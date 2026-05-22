// Copyright (c) 2011 Vince Durham
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Copyright (c) 2013-2026 The BlakeBitcoin Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <arith_uint256.h>
#include <chain.h>
#include <chainparams.h>
#include <consensus/amount.h>
#include <consensus/merkle.h>
#include <consensus/validation.h>
#include <core_io.h>
#include <key_io.h>
#include <net.h>
#include <node/context.h>
#include <node/miner.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <rpc/protocol.h>
#include <rpc/server.h>
#include <rpc/server_util.h>
#include <rpc/util.h>
#include <script/script.h>
#include <script/standard.h>
#include <streams.h>
#include <sync.h>
#include <timedata.h>
#include <txmempool.h>
#include <uint256.h>
#include <univalue.h>
#include <util/strencodings.h>
#include <util/system.h>
#include <validation.h>
#include <validationinterface.h>

#include <cstdint>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using node::BlockAssembler;
using node::NodeContext;

static constexpr size_t MAX_AUXPOW_BLOCK_CACHE{128};

static Mutex g_auxpow_mutex;
static std::map<uint256, std::shared_ptr<CBlock>> g_auxpow_blocks GUARDED_BY(g_auxpow_mutex);
static uint256 g_auxpow_prev_hash GUARDED_BY(g_auxpow_mutex);
static unsigned int g_auxpow_extra_nonce GUARDED_BY(g_auxpow_mutex){0};

static CScript ScriptForAuxpowAddress(const std::string& address)
{
    const CTxDestination destination{DecodeDestination(address)};
    if (!IsValidDestination(destination)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid BlakeBitcoin address");
    }
    return GetScriptForDestination(destination);
}

static CScript ConfiguredAuxpowMinerScript(const JSONRPCRequest& request)
{
    const ArgsManager& args{EnsureAnyArgsman(request.context)};
    const std::vector<std::string> addresses{args.GetArgs("-auxpowmineraddress")};
    if (addresses.empty()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "getauxblock without parameters requires -auxpowmineraddress=<address>; use createauxblock <address> for an explicit payout address");
    }
    if (addresses.size() > 1) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Only one -auxpowmineraddress may be configured");
    }
    return ScriptForAuxpowAddress(addresses.front());
}

static void AuxMiningCheck(const NodeContext& node, ChainstateManager& chainman)
{
    const CChainParams& chainparams{chainman.GetParams()};
    if (!chainparams.MineBlocksOnDemand()) {
        const CConnman& connman{EnsureConnman(node)};
        if (connman.GetNodeCount(ConnectionDirection::Both) == 0) {
            throw JSONRPCError(RPC_CLIENT_NOT_CONNECTED, "BlakeBitcoin is not connected!");
        }
    }

    LOCK(cs_main);
    if (!chainparams.MineBlocksOnDemand() && chainman.ActiveChainstate().IsInitialBlockDownload()) {
        throw JSONRPCError(RPC_CLIENT_IN_INITIAL_DOWNLOAD, "BlakeBitcoin is in initial sync and waiting for blocks...");
    }

    if (chainman.ActiveHeight() + 1 < chainparams.GetConsensus().nAuxpowStartHeight) {
        throw JSONRPCError(RPC_MISC_ERROR, "AuxPoW is not yet active on this chain");
    }
}

// NOTE: Assumes a conclusive result; if result is inconclusive, it must be handled by caller.
static UniValue BIP22ValidationResult(const BlockValidationState& state)
{
    if (state.IsValid()) {
        return UniValue::VNULL;
    }

    if (state.IsError()) {
        throw JSONRPCError(RPC_VERIFY_ERROR, state.ToString());
    }
    if (state.IsInvalid()) {
        const std::string reject_reason{state.GetRejectReason()};
        if (reject_reason.empty()) {
            return "rejected";
        }
        return reject_reason;
    }
    return "valid?";
}

static void IncrementAuxpowExtraNonce(CBlock& block, int height, unsigned int extra_nonce)
{
    if (block.vtx.empty() || block.vtx.front()->vin.empty()) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "AuxPoW block template is missing coinbase data");
    }

    CMutableTransaction coinbase{*block.vtx.front()};
    coinbase.vin.front().scriptSig = CScript() << height << CScriptNum(extra_nonce);
    block.vtx.front() = MakeTransactionRef(std::move(coinbase));
    block.hashMerkleRoot = BlockMerkleRoot(block);
}

static std::string TargetToLegacyHex(unsigned int nBits)
{
    arith_uint256 target;
    bool negative{false};
    bool overflow{false};
    target.SetCompact(nBits, &negative, &overflow);
    if (negative || overflow || target == 0) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "invalid difficulty bits in block");
    }

    const uint256 target_blob{ArithToUint256(target)};
    const std::vector<unsigned char> target_bytes{target_blob.begin(), target_blob.end()};
    return HexStr(target_bytes);
}

static UniValue AuxpowBlockToJSON(const CBlock& block, int height)
{
    UniValue result{UniValue::VOBJ};
    result.pushKV("hash", block.GetHash().GetHex());
    result.pushKV("chainid", block.GetChainId());
    result.pushKV("version", block.nVersion);
    result.pushKV("versionHex", strprintf("%08x", static_cast<uint32_t>(block.nVersion)));
    result.pushKV("previousblockhash", block.hashPrevBlock.GetHex());
    result.pushKV("coinbasevalue", static_cast<int64_t>(block.vtx.front()->vout.front().nValue));
    result.pushKV("bits", strprintf("%08x", block.nBits));
    result.pushKV("height", static_cast<int64_t>(height));
    result.pushKV("target", TargetToLegacyHex(block.nBits));
    return result;
}

static UniValue AuxMiningCreateBlock(const JSONRPCRequest& request, const CScript& script_pub_key)
{
    NodeContext& node{EnsureAnyNodeContext(request.context)};
    CTxMemPool& mempool{EnsureMemPool(node)};
    ChainstateManager& chainman{EnsureChainman(node)};
    AuxMiningCheck(node, chainman);

    // AuxPoW templates must use the same mempool-aware block assembler as
    // getblocktemplate so activated SegWit spends and their coinbase witness
    // commitment are not stripped from merged-mined child blocks.
    std::unique_ptr<node::CBlockTemplate> block_template{BlockAssembler{chainman.ActiveChainstate(), &mempool}.CreateNewBlock(script_pub_key)};
    if (!block_template) {
        throw JSONRPCError(RPC_OUT_OF_MEMORY, "Out of memory");
    }

    CBlock block{block_template->block};
    const CBlockIndex* pindex_prev{WITH_LOCK(cs_main, return chainman.m_blockman.LookupBlockIndex(block.hashPrevBlock))};
    if (!pindex_prev) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Unable to find AuxPoW block template parent");
    }
    const int height{pindex_prev->nHeight + 1};

    {
        LOCK(g_auxpow_mutex);
        if (g_auxpow_prev_hash != block.hashPrevBlock) {
            g_auxpow_blocks.clear();
            g_auxpow_prev_hash = block.hashPrevBlock;
            g_auxpow_extra_nonce = 0;
        }

        IncrementAuxpowExtraNonce(block, height, ++g_auxpow_extra_nonce);
        block.SetAuxpowFlag(true);

        const uint256 hash{block.GetHash()};
        g_auxpow_blocks[hash] = std::make_shared<CBlock>(block);
        while (g_auxpow_blocks.size() > MAX_AUXPOW_BLOCK_CACHE) {
            g_auxpow_blocks.erase(g_auxpow_blocks.begin());
        }
    }

    return AuxpowBlockToJSON(block, height);
}

static CAuxPow DecodeAuxPow(const UniValue& value)
{
    CAuxPow auxpow;
    const std::vector<unsigned char> data{ParseHexV(value, "auxpow")};
    CDataStream stream{data, SER_NETWORK, PROTOCOL_VERSION};
    try {
        stream >> auxpow;
        if (!stream.empty()) {
            throw std::ios_base::failure("trailing data");
        }
    } catch (const std::exception& e) {
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, strprintf("AuxPoW decode failed: %s", e.what()));
    }
    return auxpow;
}

class submitauxblock_StateCatcher final : public CValidationInterface
{
public:
    uint256 hash;
    bool found{false};
    BlockValidationState state;

    explicit submitauxblock_StateCatcher(const uint256& hash_in) : hash{hash_in} {}

protected:
    void BlockChecked(const CBlock& block, const BlockValidationState& state_in) override
    {
        if (block.GetHash() != hash) {
            return;
        }
        found = true;
        state = state_in;
    }
};

static UniValue AuxMiningSubmitBlock(const JSONRPCRequest& request, const uint256& hash, const CAuxPow& auxpow)
{
    NodeContext& node{EnsureAnyNodeContext(request.context)};
    ChainstateManager& chainman{EnsureChainman(node)};
    AuxMiningCheck(node, chainman);

    std::shared_ptr<CBlock> blockptr;
    {
        LOCK(g_auxpow_mutex);
        const auto it{g_auxpow_blocks.find(hash)};
        if (it == g_auxpow_blocks.end()) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "block hash unknown");
        }
        blockptr = std::make_shared<CBlock>(*it->second);
    }

    blockptr->SetAuxpow(new CAuxPow(auxpow));
    if (blockptr->GetHash() != hash) {
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "AuxPoW mutated block header hash");
    }

    {
        LOCK(cs_main);
        const CBlockIndex* pindex{chainman.m_blockman.LookupBlockIndex(hash)};
        if (pindex) {
            if (pindex->IsValid(BLOCK_VALID_SCRIPTS)) {
                return "duplicate";
            }
            if (pindex->nStatus & BLOCK_FAILED_MASK) {
                return "duplicate-invalid";
            }
        }

        const CBlockIndex* pindex_prev{chainman.m_blockman.LookupBlockIndex(blockptr->hashPrevBlock)};
        if (pindex_prev) {
            chainman.UpdateUncommittedBlockStructures(*blockptr, pindex_prev);
        }
    }

    bool new_block{false};
    auto sc{std::make_shared<submitauxblock_StateCatcher>(hash)};
    RegisterSharedValidationInterface(sc);
    const bool accepted{chainman.ProcessNewBlock(blockptr, /*force_processing=*/true, /*min_pow_checked=*/true, /*new_block=*/&new_block)};
    UnregisterSharedValidationInterface(sc);

    if (!new_block && accepted) {
        return "duplicate";
    }
    if (!sc->found) {
        return "inconclusive";
    }

    return BIP22ValidationResult(sc->state);
}

static RPCHelpMan createauxblock()
{
    return RPCHelpMan{"createauxblock",
        "\nCreate a merge-mined AuxPoW block template for the supplied payout address.\n",
        {
            {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "The BlakeBitcoin address that receives the coinbase output"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR_HEX, "hash", "AuxPoW child block hash to commit to in the parent coinbase"},
                {RPCResult::Type::NUM, "chainid", "BlakeBitcoin AuxPoW chain ID"},
                {RPCResult::Type::NUM, "version", "Block version including versionbits, AuxPoW flag, and chain ID bits"},
                {RPCResult::Type::STR_HEX, "versionHex", "Block version formatted in hexadecimal"},
                {RPCResult::Type::STR_HEX, "previousblockhash", "Previous block hash"},
                {RPCResult::Type::NUM, "coinbasevalue", "Coinbase value in satoshis"},
                {RPCResult::Type::STR_HEX, "bits", "Compressed target bits"},
                {RPCResult::Type::NUM, "height", "Block height"},
                {RPCResult::Type::STR_HEX, "target", "Legacy little-endian target field"},
            }},
        RPCExamples{
            HelpExampleCli("createauxblock", "\"P...\"") +
            HelpExampleRpc("createauxblock", "\"P...\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            const CScript script_pub_key{ScriptForAuxpowAddress(request.params[0].get_str())};
            return AuxMiningCreateBlock(request, script_pub_key);
        },
    };
}

static RPCHelpMan submitauxblock()
{
    return RPCHelpMan{"submitauxblock",
        "\nSubmit a solved AuxPoW payload for a block previously returned by createauxblock.\n",
        {
            {"hash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The AuxPoW child block hash returned by createauxblock"},
            {"auxpow", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The serialized AuxPoW payload"},
        },
        RPCResult{RPCResult::Type::BOOL, "", "Whether the AuxPoW block was accepted"},
        RPCExamples{
            HelpExampleCli("submitauxblock", "\"hash\" \"auxpowhex\"") +
            HelpExampleRpc("submitauxblock", "\"hash\", \"auxpowhex\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            const uint256 hash{ParseHashV(request.params[0], "hash")};
            const CAuxPow auxpow{DecodeAuxPow(request.params[1])};
            const UniValue response{AuxMiningSubmitBlock(request, hash, auxpow)};
            return response.isNull();
        },
    };
}

static RPCHelpMan getauxblock()
{
    return RPCHelpMan{"getauxblock",
        "\nCreate or submit an AuxPoW block using the legacy getauxblock interface.\n"
        "With no arguments, this creates a block using -auxpowmineraddress.\n"
        "With hash and auxpow arguments, this submits a solved AuxPoW payload.\n",
        {
            {"hash", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED, "The AuxPoW child block hash returned by getauxblock/createauxblock"},
            {"auxpow", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED, "The serialized AuxPoW payload"},
        },
        {
            RPCResult{RPCResult::Type::OBJ, "", "With no arguments, an AuxPoW work object", {
                {RPCResult::Type::STR_HEX, "hash", "AuxPoW child block hash to commit to in the parent coinbase"},
                {RPCResult::Type::NUM, "chainid", "BlakeBitcoin AuxPoW chain ID"},
                {RPCResult::Type::NUM, "version", "Block version including versionbits, AuxPoW flag, and chain ID bits"},
                {RPCResult::Type::STR_HEX, "versionHex", "Block version formatted in hexadecimal"},
                {RPCResult::Type::STR_HEX, "previousblockhash", "Previous block hash"},
                {RPCResult::Type::NUM, "coinbasevalue", "Coinbase value in satoshis"},
                {RPCResult::Type::STR_HEX, "bits", "Compressed target bits"},
                {RPCResult::Type::NUM, "height", "Block height"},
                {RPCResult::Type::STR_HEX, "target", "Legacy little-endian target field"},
            }},
            RPCResult{RPCResult::Type::BOOL, "", "With hash and auxpow arguments, whether the AuxPoW block was accepted"},
        },
        RPCExamples{
            HelpExampleCli("getauxblock", "") +
            HelpExampleCli("getauxblock", "\"hash\" \"auxpowhex\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            if (request.params.empty()) {
                return AuxMiningCreateBlock(request, ConfiguredAuxpowMinerScript(request));
            }
            if (request.params.size() == 2) {
                const uint256 hash{ParseHashV(request.params[0], "hash")};
                const CAuxPow auxpow{DecodeAuxPow(request.params[1])};
                const UniValue response{AuxMiningSubmitBlock(request, hash, auxpow)};
                return response.isNull();
            }
            throw JSONRPCError(RPC_INVALID_PARAMETER, "getauxblock must be called with either no arguments or hash and auxpow");
        },
    };
}

void RegisterAuxpowMiningRPCCommands(CRPCTable& t)
{
    static const CRPCCommand commands[]{
        {"mining", &createauxblock},
        {"mining", &submitauxblock},
        {"mining", &getauxblock},
    };
    for (const auto& c : commands) {
        t.appendCommand(c.name, &c);
    }
}

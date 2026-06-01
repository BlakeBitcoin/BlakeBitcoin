# BlakeBitcoin Core 0.25.2

BlakeBitcoin Core 0.25.2 is a release of the BlakeBitcoin (BBTC) full node and
wallet, rebased onto the Bitcoin Core 25.2 codebase. Source and release
binaries:

  https://github.com/BlakeBitcoin/BlakeBitcoin

BlakeBitcoin is a Blake-256 (8-round) AuxPoW merge-mined coin in the BlakeStream
family. For network parameters and the full 0.25.2 consensus details (including
AuxPoW start and SegWit/Taproot activation status), see `README.md`.

## How to upgrade

Shut down the running wallet/node (`blakebitcoin-qt` or `blakebitcoind`) and
wait for it to stop completely, then replace the binaries (`blakebitcoind`,
`blakebitcoin-qt`, `blakebitcoin-cli`, `blakebitcoin-tx`, `blakebitcoin-wallet`)
with the 0.25.2 build. Existing `wallet.dat` and block/chain data are kept.

## Notable changes

- Rebased onto Bitcoin Core 25.2, preserving BlakeBitcoin's network magic,
  address formats, AuxPoW (chain ID `0x0005`) merge-mining, subsidy, and
  coinbase maturity rules.
- Dual wallet support: legacy Berkeley DB `wallet.dat` and descriptor SQLite
  wallets.

## Credits

BlakeBitcoin Core is built on Bitcoin Core. Thanks to the Bitcoin Core
developers and contributors, and to the BlakeBitcoin / BlakeStream
contributors.

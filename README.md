<p align="center">
  <img src="src/qt/res/icons/blakebitcoin.png" alt="BlakeBitcoin" width="95">
</p>

# BlakeBitcoin Core 0.25.2

BlakeBitcoin Core 0.25.2 is the BBTC aux-chain port of the upstream v25.2
Core codebase. It keeps BlakeBitcoin's 0.15.21 chain identity, Blake-256
proof-of-work, AuxPoW merged-mining policy, wallet identity, and monetary
metrics while adding the Taproot-era Core codebase, descriptor-wallet support,
SQLite wallet support, ZMQ, and Linux USDT tracepoints for hardened release
builds.

## Mainnet Consensus Changes In 0.25.2

**Mainnet SegWit status: BlakeBitcoin 0.25.2 inherits the recorded 0.15.21 SegWit ACTIVE height `2564352`.**

This 0.25.2 line buries SegWit at height `2564352` and does not create a
second signaling window. The other cleanup BIPs and Taproot are assigned to
the later aux-coin rollout window, with BBTC-specific heights derived from its
150-second block spacing.

Pools and miners should use the daemon-provided AuxPoW block-template version.
Do not manually rewrite version bits.

| Rule set | Mainnet policy in BlakeBitcoin 0.25.2 |
|---|---|
| SegWit (`BIP141` / `BIP143` / `BIP147`) | Buried at `SegwitHeight = 2564352`; inherited from the 0.15.21 BIP9 activation result and not re-signaled in 0.25.2. |
| `BIP34` coinbase height | Height activation at `2572228`; `BIP34Hash = uint256{}`. |
| `BIP65` / CLTV | Height activation at `2572228`; required for standard CLTV atomic-swap refunds. |
| `BIP66` / strict DER | Height activation at `2572228`. |
| Taproot (`BIP340` / `BIP341` / `BIP342`) | BIP9 deployment bit `2`, start `1782871200` (`2026-07-01 02:00:00 UTC`), timeout `1814407200` (`2027-07-01 02:00:00 UTC`), minimum activation height `2576260`. |

Only Taproot is a future BIP9-signaled deployment in 0.25.2. `BIP34`,
`BIP65`, `BIP66`, and buried SegWit are height rules.
BlakeBitcoin Core computes the correct BIP9 top bits, Taproot bit `2`, AuxPoW
flag, and BlakeBitcoin chain-ID bits in block templates.

## About BlakeBitcoin

BlakeBitcoin is an AuxPoW merged-mined Blake-256 coin in the BlakeStream
family. It is a peer-to-peer digital currency with no central authority.

- Uses the Blake-256 hashing algorithm, 8 rounds
- Based on the upstream v25.2 Core codebase
- Uses AuxPoW / merged mining with chain ID `0x0005`
- Uses the autotools build system (`./autogen.sh`, `./configure`, `make`)
- Supports legacy Berkeley DB wallets and descriptor SQLite wallets
- Keeps BlakeBitcoin txids on single SHA-256
- Uses HASH256/double SHA-256 for witness-v0 BIP143 signing
- Keeps BIP340/BIP341/BIP342 Taproot tagged hashes byte-compatible with upstream vectors

| Network Info | Value |
|---|---|
| Ticker / unit | `BBTC` |
| Algorithm | Blake-256, 8 rounds |
| Block time | 150 seconds |
| Block reward | 50 BBTC, halving every 210000 blocks |
| Maximum money | 21000000 BBTC |
| Difficulty retarget | Every 8064 blocks, 14 days |
| Coinbase maturity | 100 blocks |
| AuxPoW chain ID | `0x0005` |
| AuxPoW start height | `500000` on mainnet; active from genesis on testnet/regtest |
| Mainnet P2P port | `8356` |
| Mainnet RPC port | `8243` |
| Testnet P2P port | `18112` |
| Testnet RPC port | `1812` |
| Regtest RPC port | `18443` |
| Mainnet genesis | `000000dcb4434e2148558a0a5c71e5c06d864accef97d75ac1c031405deb3371` |
| Mainnet Bech32 HRP | `bbtc` |
| Testnet Bech32 HRP | `tbbtc` |
| Regtest Bech32 HRP | `rbbtc` |

## Quick Start

```bash
./build.sh --help
```

For most users, downloading a tested release artifact from GitHub Releases is
the simplest path. Use `build.sh` to build release artifacts locally.

The default configuration file is `blakebitcoin.conf`. Without `-conf=`,
BlakeBitcoin Core reads only `blakebitcoin.conf` in the selected data directory.

The release config example is `share/examples/blakebitcoin.conf`. For the full
runtime option list, see [config-help.md](config-help.md).

## UPnP / miniupnpc Build Profiles

UPnP is only for desktop or home-router nodes that need automatic inbound P2P
port mapping. Nodes still sync normally through outbound peers without UPnP.

Pool, explorer, server, and Docker daemon builds should disable UPnP with
`--without-miniupnpc` so the binary has no `libminiupnpc.so.*` runtime
dependency.

UPnP-enabled Ubuntu builds need `libminiupnpc-dev` at build time and the matching
`libminiupnpc` runtime package on the target host.

## Upgrade Notes

Before starting BlakeBitcoin Core 0.25.2 on an existing data directory, close
the older wallet cleanly and back up wallet files.

When syncing 0.25.2 from old 0.8/0.15.21-era chains, header presync can look
slow or restart because v25 verifies low-work header chains before storing
them. For trusted bootstrap only, use `-minimumchainwork=0` with
`-connect=<trusted-node>` and remove those options after the node catches up.

`peers.dat` is only the cached P2P address database. It is safe to remove or
rename when moving between major releases, and BlakeBitcoin will rebuild it on
the next start. If startup fails with `Invalid or corrupt peers.dat`, remove or
rename this file:

- Windows: `%APPDATA%\BlakeBitcoin\peers.dat`
- Linux: `~/.blakebitcoin/peers.dat`
- macOS: `~/Library/Application Support/BlakeBitcoin/peers.dat`

Windows PowerShell example:

```powershell
Rename-Item "$env:APPDATA\BlakeBitcoin\peers.dat" "peers.dat.bak"
```

Linux example:

```bash
mv ~/.blakebitcoin/peers.dat ~/.blakebitcoin/peers.dat.bak
```

macOS example:

```bash
mv "$HOME/Library/Application Support/BlakeBitcoin/peers.dat" \
   "$HOME/Library/Application Support/BlakeBitcoin/peers.dat.bak"
```

If the block index or chainstate database cannot be reused after an upgrade,
restart once with `-reindex` to rebuild the local block database from the
stored block files:

```bash
blakebitcoind -reindex
```

Pruning is disabled by default (`-prune=0`), so a normal BlakeBitcoin Core node
keeps full block data. Public release nodes, explorers, pools, and bridge/watch
services should run unpruned unless they have a specific reason to discard old
block data.

For first-run testing of a new 0.25.2 build, use an isolated data directory so
the test does not touch an existing 0.15.21 wallet or chainstate:

```bash
blakebitcoin-qt -datadir=/path/to/blakebitcoin-25.2-test
```

## Build Options

```bash
./build.sh [PLATFORM] [TARGET] [OPTIONS]

Platforms:
  --native          Build natively on this machine (Linux, macOS, or Windows)
  --appimage        Build portable Linux AppImage (requires Docker)
  --windows         Cross-compile for Windows from Linux (requires Docker)
  --macos           Cross-compile for macOS from Linux (requires Docker)

Targets:
  --daemon          Build daemon only (blakebitcoind + blakebitcoin-cli + blakebitcoin-tx)
  --qt              Build Qt wallet only (blakebitcoin-qt)
  --both            Build daemon and Qt wallet (default)

Docker options:
  --pull-docker     Pull prebuilt Docker images from Docker Hub
  --build-docker    Build Docker images locally from repo Dockerfiles
  --no-docker       For --native on Linux: build directly on the host

Other options:
  --hardened-release
                   Native Linux release profile: enable SQLite, ZMQ, and USDT
                   and fail the build if configure disables any of them
  --jobs N          Parallel make jobs
```

Recommended hardened Ubuntu 26 release build:

```bash
DOCKER_NATIVE=sidgrip/native-base:26.04 \
  ./build.sh --native --both --build-docker --hardened-release --jobs 5
```

The hardened Linux release profile requires:

- `USE_BDB=true`
- `USE_SQLITE=true`
- `ENABLE_ZMQ=true`
- `ENABLE_USDT_TRACEPOINTS=true`

USDT runtime attach validation is Linux/eBPF-specific. macOS and Windows builds
do not fail release acceptance because they do not expose the Linux USDT
backend.


<!-- BEGIN electrium-build -->
### Electrium Wallet

Build the BlakeBitcoin ([Electrium](https://github.com/BlueDragon747/Blakestream-Electrum)) wallet by
choosing a target (linux/windows build in an **amd64** container, so any amd64 Docker host — Linux,
Windows, or an Intel Mac — can build either; only the macOS app needs a Mac):

```bash
./build-electrum.sh linux      # Linux AppImage    (amd64 Docker host)
./build-electrum.sh windows    # Windows .exe      (amd64 Docker host)
./build-electrum.sh macos      # macOS .dmg/.app   (on a Mac)
./build-electrum.sh all        # everything buildable on this host
```

Artifacts land in `outputs/Electrium/BBTC/`, named `Electrium-BBTC-<version>`.

The wallet builder is the shared multicoin repo
**[BlueDragon747/Blakestream-Electrum](https://github.com/BlueDragon747/Blakestream-Electrum)** — it also builds
all six BlakeStream wallets at once (`build-single-wallets.sh`) and the ElectrumX **server** Docker
image (`build-electrumx.sh`). `build-electrum.sh` auto-clones it when no local checkout is found.
<!-- END electrium-build -->

## Platform Build Instructions

### Native Linux

```bash
./build.sh --native --both --no-docker
```

- Supported validation lanes: Ubuntu 20.04, 22.04, 24.04, and 26.04
- Public Linux release lane: Ubuntu 26.04
- Native Linux outputs are written under `outputs/Ubuntu-XX/`
- `--both` refreshes daemon, command-line tools, wallet tool, util tool, and Qt
  wallet files for the detected Ubuntu lane
- `--daemon` refreshes `blakebitcoind`, `blakebitcoin-cli`,
  `blakebitcoin-tx`, `blakebitcoin-wallet`, and `blakebitcoin-util`
- `--qt` refreshes `blakebitcoin-qt`
- Native Ubuntu outputs are bare same-Ubuntu binaries that rely on host runtime
  packages installed by the generated `install-deps.sh`
- Berkeley DB 4.8 is bootstrapped into the repo cache for legacy wallet
  compatibility
- Dual-wallet builds enable both Berkeley DB and SQLite

### Native Linux With Docker

Use `--pull-docker` to pull prebuilt images from Docker Hub, or
`--build-docker` to build the selected base image locally from the Dockerfiles
in `docker/`.

```bash
./build.sh --native --both --pull-docker
./build.sh --native --qt --pull-docker
./build.sh --native --daemon --pull-docker
DOCKER_NATIVE=sidgrip/native-base:26.04 \
  ./build.sh --native --both --build-docker --hardened-release --jobs 5
```

### AppImage

```bash
./build.sh --appimage --pull-docker
```

- Uses `sidgrip/appimage-base:22.04`
- Produces `BlakeBitcoin-0.25.2-x86_64.AppImage` in `outputs/AppImage/`
- Intended for Ubuntu 22.04 and newer
- Direct launch on Ubuntu 22.04.5 needs `sudo apt install libfuse2`
- Direct launch on Ubuntu 24.04.4 and 26.04 needs `sudo apt install libfuse2t64`
- If the host lacks the FUSE package, launch with `--appimage-extract-and-run`

### Windows

```bash
./build.sh --windows --both --pull-docker
```

- Runs on Linux with Docker using `sidgrip/mxe-base:latest`
- Writes standalone `.exe` outputs plus `build-info.txt` to
  `outputs/Windows/`
- Produces daemon, CLI, TX utility, wallet tool, util tool, and Qt wallet
  executables
- Native Windows builds are diagnostic only; the release lane is the Linux MXE
  cross-compile because it packages the expected bundled runtime layout

### macOS

There are two macOS build paths:

#### Native macOS Release Build

```bash
./build.sh --native --both
```

- Uses Homebrew on the Mac host
- Installs missing Homebrew dependencies automatically
- Public macOS release artifacts should come from the native Mac build lane
- Outputs are written under `outputs/Macosx/`

#### osxcross Container Validation Build

```bash
./build.sh --macos --both --pull-docker
```

- Runs on Linux with Docker using `sidgrip/osxcross-base:sdk-26.2`
- Uses the depends plus `CONFIG_SITE` flow inside the container
- Produces validation artifacts in `outputs/Macosx/`

## Output Structure

```text
outputs/
├── AppImage/
│   ├── BlakeBitcoin-0.25.2-x86_64.AppImage
│   ├── README.md
│   └── build-info.txt
├── Macosx/
│   ├── BlakeBitcoin-Qt.app
│   ├── blakebitcoin-cli-0.25.2
│   ├── blakebitcoin-qt-0.25.2
│   ├── blakebitcoin-tx-0.25.2
│   ├── blakebitcoin-util-0.25.2
│   ├── blakebitcoin-wallet-0.25.2
│   ├── blakebitcoind-0.25.2
│   └── build-info.txt
├── Ubuntu-20/
│   ├── README.md
│   ├── blakebitcoin-256.png
│   ├── blakebitcoin-cli
│   ├── blakebitcoin.conf
│   ├── blakebitcoin.desktop
│   ├── blakebitcoin-qt
│   ├── blakebitcoin-tx
│   ├── blakebitcoin-util
│   ├── blakebitcoin-wallet
│   ├── blakebitcoind
│   └── install-deps.sh
├── Ubuntu-22/
├── Ubuntu-24/
├── Ubuntu-26/
├── Windows/
│   ├── blakebitcoin-cli-0.25.2.exe
│   ├── blakebitcoin-qt-0.25.2.exe
│   ├── blakebitcoin-tx-0.25.2.exe
│   ├── blakebitcoin-util-0.25.2.exe
│   ├── blakebitcoin-wallet-0.25.2.exe
│   ├── blakebitcoind-0.25.2.exe
│   └── build-info.txt
└── release/
    ├── BlakeBitcoin-0.25.2-Ubuntu-22-x86_64.tar.gz
    ├── BlakeBitcoin-0.25.2-Ubuntu-24-x86_64.tar.gz
    ├── BlakeBitcoin-0.25.2-Ubuntu-26-x86_64.tar.gz
    ├── BlakeBitcoin-0.25.2-Windows-x86_64.zip
    ├── BlakeBitcoin-0.25.2-macOS-x86_64.tar.gz
    ├── BlakeBitcoin-0.25.2-x86_64.AppImage
    └── SHA256SUMS
```

For Ubuntu native builds, the current host's final wallet files land in
`outputs/Ubuntu-20/`, `outputs/Ubuntu-22/`, `outputs/Ubuntu-24/`, or
`outputs/Ubuntu-26/` depending on the detected Ubuntu release. These are bare
Ubuntu-native binaries, so each Ubuntu folder gets its own `install-deps.sh`,
`README.md`, desktop launcher, icon, and `blakebitcoin.conf`. Berkeley DB 4.8
is bootstrapped into a local repo cache by the builder rather than installed
from apt.

For Windows cross-builds from Linux, the output bundle lands in
`outputs/Windows/` and contains versioned `.exe` binaries plus
`build-info.txt`.

For native macOS builds, the current host's daemon tools,
`BlakeBitcoin-Qt.app`, and the raw `blakebitcoin-qt-0.25.2` binary land in
`outputs/Macosx/`.

For AppImage builds, `outputs/AppImage/` keeps the AppImage, `README.md`, and
`build-info.txt`.

## Docker Images

When using `--pull-docker`, the build script uses these prebuilt images:

| Image | Purpose |
|---|---|
| `sidgrip/native-base:20.04` | Native Linux Ubuntu 20.04 build |
| `sidgrip/native-base:22.04` | Native Linux Ubuntu 22.04 build |
| `sidgrip/native-base:24.04` | Native Linux Ubuntu 24.04 build; default native Docker image |
| `sidgrip/native-base:26.04` | Native Linux Ubuntu 26.04 build and hardened release lane |
| `sidgrip/appimage-base:22.04` | Ubuntu 22.04+ AppImage build |
| `sidgrip/mxe-base:latest` | Windows cross-compile |
| `sidgrip/osxcross-base:sdk-26.2` | macOS cross-compile |

## Multi-Coin Builder

For coordinated BlakeStream-family wallet builds, see the
[Blakestream Installer](https://github.com/SidGrip/Blakestream-Installer).

## Tests

From a configured build tree:

```bash
make -C src -j5 test/test_blakebitcoin
src/test/test_blakebitcoin
python3 test/functional/test_runner.py feature_segwit.py feature_cltv.py feature_csv_activation.py feature_taproot.py wallet_taproot.py feature_auxpow_rpc.py feature_auxpow_segwit.py
```

Regtest and signet can be used for local feature testing. Mainnet activation
values are fixed in chain parameters and should not be changed without a
planned network release.

## License

BlakeBitcoin Core is released under the terms of the MIT license. See
[COPYING](COPYING) for more information.

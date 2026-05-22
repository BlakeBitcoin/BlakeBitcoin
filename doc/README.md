BlakeBitcoin Core
=============

Setup
---------------------
BlakeBitcoin Core is the BBTC full-node and wallet implementation. It downloads
and, by default, stores the full BlakeBitcoin chain history. Depending on the
speed of your computer and network connection, the synchronization process can
take from a few hours to longer on a fresh node.

To build BlakeBitcoin Core or use release artifacts, see the repository root
[README](/README.md).

Running
---------------------
The following are some helpful notes on how to run BlakeBitcoin Core on your native platform.

### Unix

Unpack the files into a directory and run:

- `bin/blakebitcoin-qt` (GUI) or
- `bin/blakebitcoind` (headless)

### Windows

Unpack the files into a directory, and then run blakebitcoin-qt.exe.

### macOS

Drag BlakeBitcoin Core to your applications folder, and then run BlakeBitcoin Core.

### Need Help?

Use the BlakeBitcoin project issue tracker and release notes for
project-specific support.

Building
---------------------
The following are developer notes on how to build BlakeBitcoin Core on your native platform. They are not complete guides, but include notes on the necessary libraries, compile flags, etc.

- [Dependencies](dependencies.md)
- [macOS Build Notes](build-osx.md)
- [Unix Build Notes](build-unix.md)
- [Windows Build Notes](build-windows.md)
- [FreeBSD Build Notes](build-freebsd.md)
- [OpenBSD Build Notes](build-openbsd.md)
- [NetBSD Build Notes](build-netbsd.md)
- [Android Build Notes](build-android.md)

Development
---------------------
The BlakeBitcoin repo's [root README](/README.md) contains relevant information on the development process and automated testing.

- [Developer Notes](developer-notes.md)
- [Productivity Notes](productivity.md)
- [Release Process](release-process.md)
- [Source Code Documentation (External Link)](https://doxygen.bitcoincore.org/)
- [Translation Process](translation_process.md)
- [Translation Strings Policy](translation_strings_policy.md)
- [JSON-RPC Interface](JSON-RPC-interface.md)
- [Unauthenticated REST Interface](REST-interface.md)
- [Shared Libraries](shared-libraries.md)
- [BIPS](bips.md)
- [Dnsseed Policy](dnsseed-policy.md)
- [Benchmarking](benchmarking.md)
- [Internal Design Docs](design/)

### Resources
Project-specific development resources are listed in the repository root
[README](/README.md).

### Miscellaneous
- [Assets Attribution](assets-attribution.md)
- [blakebitcoin.conf Configuration File](blakebitcoin-conf.md)
- [CJDNS Support](cjdns.md)
- [Files](files.md)
- [Fuzz-testing](fuzzing.md)
- [I2P Support](i2p.md)
- [Init Scripts (systemd/upstart/openrc)](init.md)
- [Managing Wallets](managing-wallets.md)
- [Multisig Tutorial](multisig-tutorial.md)
- [P2P bad ports definition and list](p2p-bad-ports.md)
- [PSBT support](psbt.md)
- [Reduce Memory](reduce-memory.md)
- [Reduce Traffic](reduce-traffic.md)
- [Tor Support](tor.md)
- [Transaction Relay Policy](policy/README.md)
- [ZMQ](zmq.md)

License
---------------------
Distributed under the [MIT software license](/COPYING).

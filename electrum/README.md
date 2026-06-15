# BlakeBitcoin Electrum Wallet

This folder ties the BlakeBitcoin Core repo to the matching Electrum wallet
variant. Core/Qt builds stay in `build.sh`; Electrum builds use the separate
Python wallet builder:

```bash
./build-electrum.sh wheel
./build-electrum.sh appimage
./build-electrum.sh both
```

By default the script uses a sibling `/home/sid/Blakestream-Electrum-0.25.2`
checkout when present, otherwise it can clone the Electrum source into the
user cache. Set `ELECTRUM_SOURCE=/path/to/Blakestream-Electrum-0.25.2` to use
a specific checkout.

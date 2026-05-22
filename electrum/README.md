# BlakeBitcoin Electrium Wallet

This folder ties the BlakeBitcoin Core repo to the matching Electrium wallet
variant. Core/Qt builds stay in `build.sh`; Electrium builds use the separate
Python wallet builder:

```bash
./build-electrum.sh wheel
./build-electrum.sh appimage
./build-electrum.sh both
```

By default the script uses a sibling `/home/sid/Blakestream-Electrium-0.25.2`
checkout when present, otherwise it can clone the Electrium source into the
user cache. Set `ELECTRIUM_SOURCE=/path/to/Blakestream-Electrium-0.25.2` to use
a specific checkout.

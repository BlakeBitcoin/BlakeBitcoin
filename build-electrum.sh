#!/usr/bin/env bash
# Build THIS coin's Electrum 0.25.2 wallet for an explicitly chosen target, by
# delegating to the shared multicoin builder (BlueDragon747/Blakestream-Electrum).
#
# The OS is NOT auto-detected: Linux and Windows are both built in containers, so
# any Docker host (Linux, Windows, or Intel macOS) can build either. Only the
# macOS app needs a native Mac. Pick the target with a flag.
#
# CANONICAL COPY: this file lives in the multicoin builder at
#   contrib/coin-repo/build-electrum.sh
# and is synced into every coin repo by scripts/sync-build-electrum.sh.
# Edit it there, not in the per-coin copies.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck disable=SC1091
source "$REPO_ROOT/electrum/coin.env"   # COIN_CODE, COIN_NAME

usage() {
    cat <<EOF
usage: ./build-electrum.sh <linux|windows|macos|wheel|all>

Build the ${COIN_NAME} (${COIN_CODE}) Electrum wallet. Choose the target with a
flag (the OS is NOT auto-detected — linux/windows build in an amd64 container, so
any amd64 Docker host — Linux, Windows, or an Intel Mac — can build either):

  linux | appimage   Linux AppImage           (docker; amd64 Docker host)
  windows | win      Windows .exe (+ bundle)   (docker + wine; amd64 Docker host)
  macos | mac        macOS .dmg/.app           (native; macOS host only)
  wheel              Python sdist + wheel      (local venv)
  all                every target buildable on THIS host
                     (amd64 Docker host -> linux + windows;  macOS -> + macos)

Environment:
  ELECTRUM_SOURCE         Existing BlueDragon747/Blakestream-Electrum checkout to use.
  ELECTRUM_REPO_URL       Git URL cloned if no local checkout is found.
  ELECTRUM_WORKSPACE_ROOT Generated variant workspaces.
  ELECTRUM_ARTIFACT_ROOT  Output artifact root.
EOF
}

case "${1:-}" in
    -h|--help) usage; exit 0 ;;
    "")        echo "error: choose a target (the OS is not auto-detected)" >&2; usage >&2; exit 1 ;;
esac
# macOS ships bash 3.2 (no ${var,,}); use tr.
TARGET="$(printf '%s' "$1" | tr '[:upper:]' '[:lower:]')"
case "$TARGET" in
    linux|appimage|windows|win|macos|mac|wheel|all) ;;
    *) echo "unknown target: $1" >&2; usage >&2; exit 1 ;;
esac

have_docker()   { command -v docker >/dev/null 2>&1; }
is_mac()        { [ "$(uname -s)" = "Darwin" ]; }
# linux/windows build inside an amd64 container — native on any amd64 host
# (incl. Intel Macs). On Apple Silicon it would run under slow/flaky amd64
# emulation, so `all` won't pick it automatically there (explicit still works).
amd64_native()  { case "$(uname -m)" in x86_64|amd64) return 0 ;; *) return 1 ;; esac; }
container_host() { have_docker && amd64_native; }

find_electrum_source() {
    if [ -n "${ELECTRUM_SOURCE:-}" ]; then printf '%s\n' "$ELECTRUM_SOURCE"; return; fi
    local candidate
    for candidate in \
        "$REPO_ROOT"/../Blakestream-Electrum \
        "$REPO_ROOT"/../Blakestream-Electrum-0.25.2 \
        /mnt/ram-build/Blakestream-Electrum-0.25.2 \
        /home/sid/Blakestream-Electrum-0.25.2 \
        /home/sid/Blakestream-Electrum ; do
        if [ -x "$candidate/scripts/build_wallet_variant.sh" ]; then printf '%s\n' "$candidate"; return; fi
    done
    local cache_root="${XDG_CACHE_HOME:-$HOME/.cache}/blakestream"
    local source_root="$cache_root/Blakestream-Electrum"
    local repo_url="${ELECTRUM_REPO_URL:-https://github.com/BlueDragon747/Blakestream-Electrum.git}"
    if [ ! -x "$source_root/scripts/build_wallet_variant.sh" ]; then
        mkdir -p "$cache_root"
        git clone --depth 1 "$repo_url" "$source_root"
    fi
    printf '%s\n' "$source_root"
}

ELECTRUM_ROOT="$(find_electrum_source)"
[ -x "$ELECTRUM_ROOT/scripts/build_wallet_variant.sh" ] || {
    echo "ERROR: missing Electrum builder at $ELECTRUM_ROOT/scripts/build_wallet_variant.sh" >&2; exit 1; }

WORKSPACE_ROOT="${ELECTRUM_WORKSPACE_ROOT:-$REPO_ROOT/outputs/Electrum/workspaces}"
ARTIFACT_ROOT="${ELECTRUM_ARTIFACT_ROOT:-$REPO_ROOT/outputs/Electrum}"

# Build one target after checking this host can do it.
run_one() {
    local t="$1"
    case "$t" in
        macos|mac)
            is_mac || { echo "macos must be built ON macOS (no cross-compile). From a Docker host, SSH-dispatch:" >&2
                        echo "  $ELECTRUM_ROOT/scripts/build-single-wallets.sh macos $COIN_CODE" >&2; return 2; } ;;
        linux|appimage|windows|win)
            have_docker || { echo "$t needs Docker on this host." >&2; return 2; }
            amd64_native || echo "  warning: $t builds an amd64 container; on this $(uname -m) host it runs under slow/flaky emulation." >&2 ;;
    esac
    "$ELECTRUM_ROOT/scripts/build_wallet_variant.sh" "$COIN_CODE" "$t" "$WORKSPACE_ROOT" "$ARTIFACT_ROOT"
}

if [ "$TARGET" = "all" ]; then
    built=0
    if container_host; then run_one appimage; run_one windows; built=1; fi   # amd64 Docker host
    if is_mac;         then run_one macos;                     built=1; fi   # native mac app
    [ "$built" = "1" ] || { echo "nothing buildable automatically here: need an amd64 Docker host (linux/windows) or macOS (mac app). Pass an explicit target to force." >&2; exit 1; }
else
    run_one "$TARGET"
fi

if [ -d "$ARTIFACT_ROOT/$COIN_CODE" ]; then
    ( cd "$ARTIFACT_ROOT/$COIN_CODE" \
        && find . -type f ! -name SHA256SUMS -print0 | sort -z | xargs -0 -r sha256sum > SHA256SUMS )
fi
echo "Electrum ${COIN_CODE} artifacts: $ARTIFACT_ROOT/$COIN_CODE"

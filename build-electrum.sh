#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$REPO_ROOT/electrum/coin.env"

usage() {
    cat <<EOF
usage: ./build-electrum.sh [wheel|appimage|both]

Build the ${COIN_NAME} Electrium wallet variant.

Environment:
  ELECTRIUM_SOURCE        Existing Blakestream-Electrium-0.25.2 checkout.
  ELECTRIUM_REPO_URL      Git URL used if no local source checkout exists.
  ELECTRIUM_WORKSPACE_ROOT Generated variant workspaces.
  ELECTRIUM_ARTIFACT_ROOT  Output artifact root.
EOF
}

TARGET="${1:-wheel}"
case "$TARGET" in
    wheel|appimage|both) ;;
    -h|--help) usage; exit 0 ;;
    *) usage >&2; exit 1 ;;
esac

find_electrium_source() {
    if [[ -n "${ELECTRIUM_SOURCE:-}" ]]; then
        printf '%s\n' "$ELECTRIUM_SOURCE"
        return
    fi

    local candidate
    for candidate in \
        "$REPO_ROOT/../Blakestream-Electrium-0.25.2" \
        "/home/sid/Blakestream-Electrium-0.25.2"
    do
        if [[ -x "$candidate/scripts/build_wallet_variant.sh" ]]; then
            printf '%s\n' "$candidate"
            return
        fi
    done

    local cache_root="${XDG_CACHE_HOME:-$HOME/.cache}/blakestream"
    local source_root="$cache_root/Blakestream-Electrium-0.25.2"
    local repo_url="${ELECTRIUM_REPO_URL:-https://github.com/SidGrip/Blakestream-Electrium-0.25.2.git}"
    if [[ ! -x "$source_root/scripts/build_wallet_variant.sh" ]]; then
        mkdir -p "$cache_root"
        git clone "$repo_url" "$source_root"
    fi
    printf '%s\n' "$source_root"
}

ELECTRIUM_ROOT="$(find_electrium_source)"
if [[ ! -x "$ELECTRIUM_ROOT/scripts/build_wallet_variant.sh" ]]; then
    echo "ERROR: missing Electrium builder at $ELECTRIUM_ROOT/scripts/build_wallet_variant.sh" >&2
    exit 1
fi

WORKSPACE_ROOT="${ELECTRIUM_WORKSPACE_ROOT:-$REPO_ROOT/outputs/Electrium/workspaces}"
ARTIFACT_ROOT="${ELECTRIUM_ARTIFACT_ROOT:-$REPO_ROOT/outputs/Electrium}"

"$ELECTRIUM_ROOT/scripts/build_wallet_variant.sh" "$COIN_CODE" "$TARGET" "$WORKSPACE_ROOT" "$ARTIFACT_ROOT"

if [[ -d "$ARTIFACT_ROOT/$COIN_CODE" ]]; then
    (
        cd "$ARTIFACT_ROOT/$COIN_CODE"
        find . -type f ! -name SHA256SUMS -print0 | sort -z | xargs -0 -r sha256sum > SHA256SUMS
    )
fi

echo "Electrium ${COIN_CODE} artifacts: $ARTIFACT_ROOT/$COIN_CODE"

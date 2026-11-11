#!/usr/bin/env bash
# Install Ubuntu packages on GitHub-hosted CI, bypassing a broken Azure
# mirror+file source. Keep the original system apt configuration untouched.
set -euo pipefail

if (( $# == 0 )); then
    echo "Usage: bash $0 <apt-package> [...]" >&2
    exit 2
fi

# GitHub-hosted runners may intermittently lose DNS access to
# azure.archive.ubuntu.com. Specify ordinary Ubuntu mirrors explicitly,
# without relying on the runner image's mirror+file mapping.
. /etc/os-release
if [[ "${ID:-}" != ubuntu || -z "${VERSION_CODENAME:-}" ]]; then
    echo "CI package helper requires an Ubuntu runner" >&2
    exit 1
fi

source_list="$(mktemp)"
trap 'rm -f "$source_list"' EXIT
for mirror in https://archive.ubuntu.com/ubuntu https://mirrors.edge.kernel.org/ubuntu; do
    printf 'deb [signed-by=/usr/share/keyrings/ubuntu-archive-keyring.gpg] %s %s main restricted universe multiverse\n' \
        "$mirror" "$VERSION_CODENAME" > "$source_list"
    printf 'deb [signed-by=/usr/share/keyrings/ubuntu-archive-keyring.gpg] %s %s-updates main restricted universe multiverse\n' \
        "$mirror" "$VERSION_CODENAME" >> "$source_list"
    printf 'deb [signed-by=/usr/share/keyrings/ubuntu-archive-keyring.gpg] %s %s-security main restricted universe multiverse\n' \
        "https://security.ubuntu.com/ubuntu" "$VERSION_CODENAME" >> "$source_list"

    # Only use this source file for both update and install. This prevents
    # package downloads from falling back to the runner's broken Azure mirror.
    apt_options=(
        -o "Dir::Etc::sourcelist=$source_list"
        -o "Dir::Etc::sourceparts=-"
        -o "Acquire::Retries=2"
        -o "Acquire::http::Timeout=20"
        -o "Acquire::https::Timeout=20"
    )
    for attempt in 1 2; do
        echo "Installing CI packages through $mirror, attempt $attempt/2: $*"
        if sudo apt-get "${apt_options[@]}" update &&
           sudo DEBIAN_FRONTEND=noninteractive apt-get "${apt_options[@]}" install \
               -y --no-install-recommends "$@"; then
            exit 0
        fi
        sleep $((attempt * 5))
    done
done

echo "All Ubuntu mirror attempts failed. No APK build was attempted." >&2
exit 1

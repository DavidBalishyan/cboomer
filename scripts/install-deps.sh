#!/usr/bin/env bash

## There is an .envforge.yaml in the repo root, if you can use it (https://github.com/DavidBalishyan/envforge)
## It's recommended to use envforge instead of this script
set -euo pipefail

if [ "${1:-}" = "help" ]; then
    echo "Usage: $0"
    echo "Install build dependencies for cboomer."
    echo ""
    echo "Detects the distro and installs the required packages"
    echo "(build-essential, libx11-dev, libxrandr-dev, libxext-dev, libgl-dev,"
    echo "zlib1g-dev or equivalent) via the native package manager."
    echo ""
    echo "Supported distros: Debian, Ubuntu, Linux Mint, Pop!_OS, Fedora,"
    echo "RHEL, CentOS, Rocky Linux, AlmaLinux, Arch, Manjaro, EndeavourOS,"
    echo "Alpine, openSUSE"
    exit 0
fi

if [ "${EUID:-$(id -u)}" -ne 0 ]; then
    exec sudo "$0" "$@"
fi

distro=
if [ -f /etc/os-release ]; then
    . /etc/os-release
    distro="$ID"
elif command -v lsb_release >/dev/null 2>&1; then
    distro="$(lsb_release -si)"
fi

case "${distro,,}" in
    debian|ubuntu|linuxmint|pop)
        apt-get update
        apt-get install -y build-essential libx11-dev libxrandr-dev libxext-dev libgl-dev zlib1g-dev
        ;;
    fedora|rhel|centos|rocky|almalinux)
        dnf install -y gcc make libX11-devel libXrandr-devel libXext-devel mesa-libGL-devel zlib-devel
        ;;
    arch|manjaro|endeavouros)
        pacman -S --needed base-devel libx11 libxrandr libxext mesa zlib
        ;;
    alpine)
        apk add build-base libx11-dev libxrandr-dev libxext-dev mesa-dev zlib-dev
        ;;
    opensuse*|suse*)
        zypper install -y gcc make libX11-devel libXrandr-devel libXext-devel Mesa-libGL-devel zlib-devel
        ;;
    void)
        xbps-install -S base-devel libX11-devel libXrandr-devel libXext-devel mesa-dri zlib-devel
        ;;
    *)
        echo "Unsupported distro: $distro"
        echo "Please install build-essential (or equivalent), libx11-dev, libxrandr-dev, libxext-dev, libgl-dev, and zlib1g-dev manually."
        exit 1
        ;;
esac

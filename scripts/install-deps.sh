#!/usr/bin/env bash
set -euo pipefail

if [ "${1:-}" = "help" ]; then
    echo "Usage: $0"
    echo "Install build dependencies for cboomer."
    echo ""
    echo "Detects the distro and installs the required packages"
    echo "(build-essential, libx11-dev, libxrandr-dev, libxext-dev, libgl-dev"
    echo "or equivalent) via the native package manager."
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
        apt-get install -y build-essential libx11-dev libxrandr-dev libxext-dev libgl-dev
        ;;
    fedora|rhel|centos|rocky|almalinux)
        dnf install -y gcc make libX11-devel libXrandr-devel libXext-devel mesa-libGL-devel
        ;;
    arch|manjaro|endeavouros)
        pacman -S --needed base-devel libx11 libxrandr libxext mesa
        ;;
    alpine)
        apk add build-base libx11-dev libxrandr-dev libxext-dev mesa-dev
        ;;
    opensuse*|suse*)
        zypper install -y gcc make libX11-devel libXrandr-devel libXext-devel Mesa-libGL-devel
        ;;
    *)
        echo "Unsupported distro: $distro"
        echo "Please install build-essential (or equivalent), libx11-dev, libxrandr-dev, libxext-dev, and libgl-dev manually."
        exit 1
        ;;
esac

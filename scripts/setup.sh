#!/bin/sh

set -eu

require_sudo_if_needed() {
  if [ "$(id -u)" -eq 0 ]; then
    echo ""
    return 0
  fi

  if command -v sudo >/dev/null 2>&1; then
    echo "sudo"
    return 0
  fi

  echo "This installer requires root privileges or sudo."
  exit 1
}

install_with_apt() {
  SUDO="$(require_sudo_if_needed)"
  echo "Detected package manager: apt-get"
  $SUDO apt-get update
  $SUDO apt-get install -y \
    build-essential \
    pkg-config \
    libncurses-dev \
    libcurl4-openssl-dev \
    libcjson-dev
}

install_with_dnf() {
  SUDO="$(require_sudo_if_needed)"
  echo "Detected package manager: dnf"
  if ! $SUDO dnf install -y \
    gcc \
    make \
    pkgconf-pkg-config \
    ncurses-devel \
    libcurl-devel \
    libcjson-devel; then
    echo "Retrying with cjson-devel fallback..."
    $SUDO dnf install -y \
      gcc \
      make \
      pkgconf-pkg-config \
      ncurses-devel \
      libcurl-devel \
      cjson-devel
  fi
}

install_with_pacman() {
  SUDO="$(require_sudo_if_needed)"
  echo "Detected package manager: pacman"
  $SUDO pacman -Sy --noconfirm --needed \
    base-devel \
    pkgconf \
    ncurses \
    curl \
    cjson
}

install_with_zypper() {
  SUDO="$(require_sudo_if_needed)"
  echo "Detected package manager: zypper"
  $SUDO zypper --non-interactive install \
    gcc \
    make \
    pkg-config \
    ncurses-devel \
    libcurl-devel \
    libcjson-devel
}

install_with_brew() {
  echo "Detected package manager: brew"
  brew update
  brew install \
    gcc \
    make \
    pkg-config \
    ncurses \
    curl \
    cjson
}

validate_environment() {
  missing=0

  for cmd in gcc make pkg-config; do
    if ! command -v "$cmd" >/dev/null 2>&1; then
      echo "Missing required command: $cmd"
      missing=1
    fi
  done

  for pc in ncurses libcurl libcjson; do
    if [ "$pc" = "libcjson" ]; then
      if ! pkg-config --exists libcjson && ! pkg-config --exists cjson; then
        echo "Missing pkg-config dependency: libcjson (or cjson)"
        missing=1
      fi
    else
      if ! pkg-config --exists "$pc"; then
        echo "Missing pkg-config dependency: $pc"
        missing=1
      fi
    fi
  done

  if [ "$missing" -ne 0 ]; then
    echo "Environment check failed."
    exit 1
  fi

  echo "Environment OK"
}

main() {
  if command -v apt-get >/dev/null 2>&1; then
    install_with_apt
  elif command -v dnf >/dev/null 2>&1; then
    install_with_dnf
  elif command -v pacman >/dev/null 2>&1; then
    install_with_pacman
  elif command -v zypper >/dev/null 2>&1; then
    install_with_zypper
  elif command -v brew >/dev/null 2>&1; then
    install_with_brew
  else
    cat <<'EOF'
Unsupported package manager.
Please install these dependencies manually:
- gcc
- make
- pkg-config
- ncurses (development package)
- libcurl (development package)
- cJSON (development package)
EOF
    exit 1
  fi

  validate_environment
}

main "$@"

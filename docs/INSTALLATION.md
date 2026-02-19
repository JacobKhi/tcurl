# Installation

## Dependencies

Build and runtime dependencies:

- `gcc`
- `make`
- `pkg-config`
- `ncurses` (or `ncursesw`)
- `libcurl`
- `cjson`
- `pthread` (usually included with gcc)

> **Note:** Use the latest versions available in your package manager. The build system automatically detects and uses the correct library variants (e.g., `libcjson` or `cjson`).

## Quick Install (Recommended)

### Automatic Dependency Installation

The project includes a setup script that automatically detects your package manager and installs all required dependencies:

```bash
make deps
```

Supported package managers:
- **apt-get** (Ubuntu/Debian)
- **dnf** (Fedora/RHEL/CentOS)
- **pacman** (Arch Linux)
- **zypper** (openSUSE)
- **brew** (macOS)

The script will:
1. Detect your system's package manager
2. Install all build dependencies
3. Validate the environment after installation

### Check Dependencies

To verify all dependencies are installed correctly:

```bash
make check-deps
```

This will check for:
- Required commands (`gcc`, `make`, `pkg-config`)
- Required libraries (`ncurses`, `libcurl`, `cjson`)

## Manual Dependency Installation

### Ubuntu/Debian

```bash
sudo apt-get update
sudo apt-get install build-essential pkg-config libncursesw5-dev libcurl4-openssl-dev libcjson-dev
```

### Fedora/RHEL/CentOS

```bash
sudo dnf install gcc make pkg-config ncurses-devel libcurl-devel cjson-devel
```

### Arch Linux

```bash
sudo pacman -S base-devel ncurses curl cjson
```

## Building from Source

1. Clone the repository:
```bash
git clone https://github.com/yourusername/tcurl.git
cd tcurl
```

2. Install dependencies (if not already installed):
```bash
make deps
```

3. Build the project:
```bash
make
```

4. Run the binary:
```bash
./tcurl
```

## User Installation (Recommended)

Install `tcurl` to your local user directory without requiring root privileges:

```bash
make install-user
```

This will:
- Install the `tcurl` binary to `~/.local/bin/tcurl`
- Copy default configuration files to `~/.config/tcurl/` (if they don't exist)
- Make the `tcurl` command available in your terminal (without `./`)
- Check if `~/.local/bin` is in your `PATH`

**Important:** If `~/.local/bin` is not in your PATH, the installer will show instructions to add it:

```bash
# Add to your shell config (~/.bashrc, ~/.zshrc, etc.)
export PATH="$HOME/.local/bin:$PATH"
```

Then restart your shell or source the config file:

```bash
source ~/.bashrc  # or ~/.zshrc
```

After installation, you can run:
```bash
tcurl  # instead of ./tcurl
```

### Uninstall

To remove the user installation:

```bash
make uninstall-user
```

> **Note:** This removes the binary but keeps your configuration files at `~/.config/tcurl`.

## Configuration

Configuration files are created in:
- Linux/macOS: `~/.config/tcurl/`
- Custom location: Set `XDG_CONFIG_HOME` environment variable

Default configuration files:
- `layout.conf` - UI layout and language settings
- `themes.conf` - Color themes
- `keymap.conf` - Keyboard bindings
- `envs.json` - Environment variables
- `headers.txt` - Header autocomplete list
- `history.conf` - History settings

> **Note:** Running `make install-user` automatically copies default configuration files to `~/.config/tcurl/` if they don't already exist. If you skip user installation, config files will be created on first run.

## Testing

Run the test suite:
```bash
make test
```

Run tests with AddressSanitizer (for development):
```bash
make test-asan
```

## Troubleshooting

### Dependencies not found

If you encounter "command not found" or "library not found" errors:

1. **Check dependencies:**
```bash
make check-deps
```

2. **Install/reinstall dependencies:**
```bash
make deps
```

3. **Verify pkg-config can find libraries:**
```bash
pkg-config --libs ncurses
pkg-config --libs libcurl
pkg-config --libs libcjson  # or cjson
```

### Missing cJSON library

If you get errors about missing `cjson.h`, you may need to install it manually:

```bash
git clone https://github.com/DaveGamble/cJSON.git
cd cJSON
mkdir build && cd build
cmake ..
make
sudo make install
```

### ncursesw not found

Ensure you have the wide-character version of ncurses installed:
```bash
pkg-config --libs ncursesw
```

If not found, install `libncursesw5-dev` (Debian/Ubuntu) or `ncurses-devel` (Fedora/RHEL).

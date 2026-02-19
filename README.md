# tcurl

A terminal HTTP client with a keyboard-driven interface inspired by vim.

## Quick Start

```bash
make
./tcurl
```

Inside tcurl:
- `i` to enter insert mode and edit URL/body/headers
- `Escape` to return to normal mode
- `Enter` to send request
- `:h` for help
- `:q` to quit

## Installation

### Quick Install

```bash
# Install dependencies automatically
make deps

# Build the project
make

# Install to ~/.local/bin
make install-user
```

See [docs/INSTALLATION.md](docs/INSTALLATION.md) for detailed installation instructions.

## Features

- Vim-like modal interface (normal, insert, command, search)
- Multiple layout profiles (classic, quad, focus_editor)
- HTTP methods: GET, POST, PUT, DELETE
- Environment variables and templating
- Request history with persistence
- Export to curl or JSON
- Multiple color themes
- Internationalization (English/Portuguese)

See [docs/FEATURES.md](docs/FEATURES.md) for complete feature list.

## Documentation

- [Installation Guide](docs/INSTALLATION.md)
- [Usage Guide](docs/USAGE.md)
- [Configuration](docs/CONFIGURATION.md)
- [Commands Reference](docs/COMMANDS.md)
- [Features](docs/FEATURES.md)

## Quick Reference

### Modes
- **Normal**: Navigate and send requests
- **Insert**: Edit URL, headers, and body
- **Command**: Execute commands (`:`)
- **Search**: Find in history or response (`/`)

### Key Bindings (Normal Mode)
- `h`/`l` or arrows: Focus panels
- `j`/`k` or arrows: Scroll/navigate
- `i`: Enter insert mode
- `m`: Cycle HTTP method
- `e`: Cycle environment
- `r`: Send request
- `:`: Command mode
- `/`: Search mode

### Essential Commands
```
:h              Show help
:q              Quit
:lang list      List languages
:layout list    List layouts
:theme list     List themes
:export curl    Export as curl
:auth bearer    Set bearer token
:clear!         Clear history
```

See [docs/COMMANDS.md](docs/COMMANDS.md) for all commands.

## Configuration

Configuration directory: `~/.config/tcurl/`

Files:
- `layout.conf` - UI layout and language
- `themes.conf` - Color themes
- `keymap.conf` - Keyboard bindings
- `envs.json` - Environment variables
- `headers.txt` - Header autocomplete
- `history.conf` - History settings

See [docs/CONFIGURATION.md](docs/CONFIGURATION.md) for details.

## Usage Examples

### Basic Request
1. Start tcurl: `./tcurl`
2. Press `i` to edit URL
3. Type: `https://httpbin.org/get`
4. Press `Escape` then `Enter`

### POST with Body
1. Press `i` and enter URL
2. Press `Tab` to switch to BODY
3. Type JSON: `{"name": "test"}`
4. Press `m` to change method to POST
5. Press `Escape` then `Enter`

### Using Environments
1. Edit `~/.config/tcurl/envs.json`:
```json
{
  "dev": {"URL": "http://localhost:3000"}
}
```
2. Use in request: `{{URL}}/api/users`
3. Press `e` to cycle environments

See [docs/USAGE.md](docs/USAGE.md) for more examples.

## Testing

```bash
make test
```

## License

[MIT](LICENSE.md)

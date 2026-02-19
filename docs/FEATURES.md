# Features

## Core Features

### HTTP Client
- Full HTTP/HTTPS support via libcurl
- Methods: GET, POST, PUT, DELETE
- Custom headers support
- Request body editing
- Response viewing with metadata

### Terminal UI
- Ncurses-based TUI
- Modal interface (vim-inspired)
- Three-panel layout (configurable)
- Responsive design
- Color themes

### Modes
- **Normal**: Navigation and commands
- **Insert**: Text editing
- **Command**: Execute commands (`:`)
- **Search**: Find in history/response (`/`)

### Keyboard-Driven
- Vim-like bindings (hjkl navigation)
- Fully customizable keymap
- Arrow key support
- No mouse required

## Advanced Features

### Layout Profiles

#### Classic Layout
- History panel: Left (30% width)
- Editor panel: Top-right (50% height)
- Response panel: Bottom-right (50% height)
- Split editor (URL, BODY, HEADERS panels)

#### Quad Layout
- 2x2 grid system
- Customizable slot assignments
- Adjustable split positions
- Flexible panel arrangement

#### Focus Editor Layout
- Large editor on top (60% height)
- History and Response at bottom
- Tabbed editor interface (URL | BODY | HEADERS)
- Optimized for large payloads

### Environments

- Multiple environment definitions
- JSON-based configuration
- Variable substitution with `{{VAR}}`
- Quick switching with `e` key
- URL, headers, and body support

### History

- Persistent storage (JSONL format)
- Request and response capture
- Load previous requests
- Replay functionality
- Configurable max entries
- Corrupted-line tolerance

### Search

- Contextual search (history or response)
- Case-insensitive matching
- Next/previous navigation
- Configurable search target
- Immediate search with `:find`

### Export

- cURL command generation
- JSON export format
- Copy-paste ready output
- Includes all headers and body
- Environment variables expanded

### Authentication

- Bearer token support
- Basic authentication
- Automatic header formatting
- Base64 encoding (for basic auth)

### Themes

- Multiple color presets
- Custom theme creation
- Runtime theme switching
- Persistent theme saving
- `themes.conf` configuration

### Internationalization

- English (en)
- Portuguese (pt)
- Auto-detection from LANG env
- Runtime language switching
- Full UI translation

### Request Metadata

- HTTP status codes
- Response time (milliseconds)
- Payload size (KB)
- JSON detection and pretty-print
- Scroll position tracking

### Async Requests

- Non-blocking HTTP requests
- Worker thread execution
- UI remains responsive
- Mutex-protected state
- In-flight status indicator

### Configuration

- User config directory support
- XDG Base Directory compliance
- Multiple config files:
  - `layout.conf` - UI settings
  - `themes.conf` - Color themes
  - `keymap.conf` - Key bindings
  - `envs.json` - Environments
  - `headers.txt` - Autocomplete
  - `history.conf` - History settings

### Autocomplete

- Header name autocomplete
- Based on `headers.txt`
- Common headers included
- Extensible via config

### State Management

- Persistent history
- Configuration loading
- Session state
- Error recovery
- Graceful degradation

## Developer Features

### Testing
- Comprehensive test suite
- 8 test modules
- AddressSanitizer support
- Automated testing

### Code Quality
- Modular architecture
- Inline documentation
- Clean code structure
- No code duplication
- Consistent style

### Internationalization Support
- Easy to add new languages
- Translation framework
- String management
- UTF-8 compatible

### Extensibility
- Command system
- Pluggable actions
- Theme system
- Layout engine
- Export formats

## Platform Support

### Operating Systems
- Linux (primary)
- macOS (compatible)
- BSD variants (compatible)

### Terminal Compatibility
- Any terminal with ncurses support
- UTF-8 terminal recommended
- 256-color support optional
- Minimum 80x24 recommended

## Planned Features

- More HTTP methods (PATCH, HEAD, OPTIONS)
- Request collections/workspaces
- Proxy support
- SSL certificate options
- Request timing breakdown
- Response headers viewing
- Cookie management
- Multi-tab interface
- Request chaining
- GraphQL support
- WebSocket support (future)

See [USAGE.md](USAGE.md) for usage instructions and [CONFIGURATION.md](CONFIGURATION.md) for configuration details.

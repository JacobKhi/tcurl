# tcurl

`tcurl` is a terminal HTTP client (TUI) written in C with `ncurses`.
It follows a keyboard-driven workflow inspired by vim while using real HTTP requests through `libcurl`.

## Current Features

- Configurable layout profiles:
  - `classic` (History left, Editor top-right, Response bottom-right)
  - `quad` (2x2 quadrant placement via config)
  - `focus_editor` (large editor on top, History/Response at bottom)
- Configurable panel sizing via `layout.conf` and theme presets via `themes.conf` in active config dir
- Mode-based keyboard workflow (`normal`, `insert`, `command`, `search`)
- Vim-like command line (`:`) and search (`/`) in the bottom bar
- Command history navigation in `:` mode (`Up` / `Down`)
- Request editing:
  - URL field
  - dedicated BODY + HEADERS split editor (`classic` / `quad`)
  - tabbed editor (`URL | BODY | HEADERS`) in `focus_editor`
  - multiline body
  - multiline headers
  - headers autocomplete from `headers.txt` in active config dir
- HTTP methods: `GET`, `POST`, `PUT`, `DELETE`
- Environments and variables from `envs.json` in active config dir
- Template substitution for `{{VAR}}` in URL, BODY, and HEADERS
- Authorization helpers (`:auth bearer`, `:auth basic`)
- Async requests (worker thread) so UI stays responsive
- State synchronization with mutex guards between UI and request thread
- Response metadata:
  - status code
  - elapsed time
  - payload size
  - JSON indicator
- JSON pretty-print (fallback to raw text)
- Persistent request history (JSONL) with load-back into editor/response
- History load stats for corrupted-line tolerance + incremental append persistence
- Replay from history snapshots
- Contextual search with next/prev navigation (`n` / `N`)
- Request export commands (`:export curl`, `:export json`)

## Dependencies

Build/runtime dependencies:

- `gcc`
- `make`
- `pkg-config`
- `ncurses`
- `libcurl`
- `cJSON`

Notes:

- `pthread` is used and linked, typically provided by the system toolchain on Linux/macOS.
- Use `make deps` to install dependencies automatically on supported package managers.
- History is stored at `$HOME/.config/tcurl/history.jsonl`.

## Quickstart

```sh
make deps
make
./tcurl
```

Optional dependency check:

```sh
make check-deps
```

## Run from Anywhere

Install user-local (no sudo):

```sh
make install-user
```

Then run from any directory:

```sh
tcurl
```

If `tcurl` is not found, add `~/.local/bin` to your `PATH`.

Configuration directory resolution order:

1. `TCURL_CONFIG_DIR`
2. `$XDG_CONFIG_HOME/tcurl`
3. `$HOME/.config/tcurl`
4. Fallback read-only defaults from `<executable_dir>/config`

Notes:

- Config reads try user config first, then fallback defaults.
- Config writes (for example `:theme <name> -s`) always target the user config directory.

## Keybindings (Default)

From `keymap.conf` in the active config directory:

- `h` / `l` or `Left` / `Right`: focus left/right panel
- `j` / `k` or `Up` / `Down`: move down/up (history selection or response scroll)
- `i`: enter insert mode
- `esc`: return to normal mode
- `tab`: cycle editor field (`URL` -> `BODY` -> `HEADERS`)
- `m`: cycle HTTP method
- `E`: cycle active environment (`dev` -> `prod` -> ...)
- `r`: send request
- `/`: open search prompt (searches History or Response based on focus)
- `n`: next search match
- `N`: previous search match
- `:`: open command prompt
- `enter` (in `Editor` panel, normal mode): enter insert mode
- `enter` (in `History` panel): load selected history item
- `Shift+Enter` (in `History` panel): replay selected history item
- `R` (in `History` panel): replay fallback when terminal does not emit `Shift+Enter`
- `tab` (in insert mode): insert indentation (4 spaces)
- `Shift+Tab` (while editing HEADERS in insert mode): autocomplete header name

Search behavior:

- Focus `History`: `/` searches history entries (`METHOD URL`)
- Focus `Response`: `/` searches response body
- Focus `Editor`: `/` searches response body
- `n` / `N` wrap around matches

Command mode:

- `:q` or `:quit`: quit app
- `:h` or `:help`: print commands + loaded keybindings in `Response` panel
- Bottom-right quick hint shows `:h`, `:q`, and movement (`h/j/k/l` + arrows)
- `:theme list`: list available theme presets
- `:theme <name>`: apply theme preset for current session
- `:theme <name> -s` or `:theme <name> --save`: apply and persist active preset to active user `layout.conf`
- `:export curl` or `:export json`: export current request
- `:auth bearer <token>`: set/update `Authorization: Bearer <token>`
- `:auth basic <user>:<pass>`: set/update `Authorization: Basic <base64>`
- `:find <term>`: run contextual search immediately
- `:set`: show runtime settings
- `:set search_target auto|history|response`: override `/` default target
- `:set max_entries <n>`: update history retention for current session
- `:clear!` or `:ch!`: clear history from memory and persisted storage

## History Configuration

Persistent history retention is configurable in `history.conf` in the active config directory:

```ini
max_entries = 500
```

The app keeps only the newest `max_entries` requests.

## Environments

Environments are loaded from `envs.json` in the active config directory:

```json
{
  "dev": {
    "BASE_URL": "https://httpbin.org",
    "TOKEN": "dev-token"
  },
  "prod": {
    "BASE_URL": "https://api.example.com",
    "TOKEN": "prod-token"
  }
}
```

Use templates in request fields:

- URL: `{{BASE_URL}}/anything`
- Header: `Authorization: Bearer {{TOKEN}}`
- Body: `{"token":"{{TOKEN}}"}`

If a template variable is missing in the active environment, request execution fails with a clear error message.

## Header Autocomplete

Header suggestions come from `headers.txt` in the active config directory (one header per line, `#` comments allowed).
In insert mode with HEADERS active, press `Shift+Tab` to complete/cycle matching header names.

## Layout Profiles

Layout is loaded at startup from `layout.conf` in the active config directory:

```ini
profile = classic
theme_preset = vivid
show_footer_hint = true
quad_history_slot = tl
quad_editor_slot = tr
quad_response_slot = br
classic_history_width_pct = 33
classic_editor_height_pct = 50
focus_editor_height_pct = 66
quad_split_x_pct = 50
quad_split_y_pct = 50
use_colors = true
focus_fg = cyan
focus_bg = default
```

Valid `profile` values:

- `classic`
- `quad`
- `focus_editor`

Valid `quad_*_slot` values:

- `tl`
- `tr`
- `bl`
- `br`

Notes:

- Missing/invalid config falls back to `classic` with default slots.
- In `quad`, panel slots must be unique; duplicated slots fall back to defaults.
- `focus_editor` ignores `quad_*` slot settings.
- `theme_preset` points to a preset name from `themes.conf` in the active config directory.
- `show_footer_hint` toggles the lower-right quick help hint (`:h`, `:q`, movement).
- Percentage controls:
  - `classic_history_width_pct`: `20..70`
  - `classic_editor_height_pct`: `25..75`
  - `focus_editor_height_pct`: `40..85`
  - `quad_split_x_pct` / `quad_split_y_pct`: `30..70`
- `use_colors` accepts: `true/false`, `yes/no`, `on/off`, `1/0`
- Color keys (`*_fg`, `*_bg`) accept:
  - `default`, `black`, `red`, `green`, `yellow`, `blue`, `magenta`, `cyan`, `white`

## Runtime Theme Commands

Theme can be switched at runtime (no restart required):

- `:theme list`
- `:theme <name>`

Persist current preset to config:

- `:theme <name> -s`
- `:theme <name> --save`

Notes:

- Without `-s`/`--save`, change is session-only.
- With `-s`/`--save`, the active user `layout.conf` is rewritten in canonical format and updates `theme_preset`.

## Theme Presets File

Theme presets are loaded from `themes.conf` in the active config directory using INI-style sections:

```ini
[mono]
use_colors = false

[vivid]
use_colors = true
focus_fg = yellow
focus_bg = blue
```

You can create new presets by adding new sections (for example `[solarized]`) with any supported theme keys.

## Typing Area Improvements

- Dedicated split editor with BODY and HEADERS side by side (`classic` / `quad`)
- Tabbed editor mode in `focus_editor` for larger typing area
- Active editing line highlight
- Line number gutter in BODY and HEADERS
- Vertical scrolling that follows cursor position for long content

## Testing

Run test suite:

```sh
make test
```

Run tests with sanitizers (ASan/UBSan):

```sh
make test-sanitizers
```

CI runs build + tests on Linux/macOS and sanitizer tests on Linux.

## Project Structure

- `src/core/`: core logic (http, dispatch, keymap, history, text buffer)
- `src/ui/`: ncurses drawing and input handling
- `include/`: public/internal headers
- `config/`: keymap configuration
- `scripts/`: setup helpers

Uninstall user-local binary:

```sh
make uninstall-user
```

Build release tarball:

```sh
make release
# optional version tag:
sh scripts/release.sh 0.1.0
```

## Current Limitations / Roadmap

- `command` and `search` modes exist, but advanced command history/interactive UX is still minimal
- History compaction strategy is intentionally simple and may still be optimized for very large datasets

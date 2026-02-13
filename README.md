# tcurl

`tcurl` is a terminal HTTP client (TUI) written in C with `ncurses`.
It follows a keyboard-driven workflow inspired by vim while using real HTTP requests through `libcurl`.

## Current Features

- Configurable layout profiles:
  - `classic` (History left, Editor top-right, Response bottom-right)
  - `quad` (2x2 quadrant placement via config)
  - `focus_editor` (large editor on top, History/Response at bottom)
- Configurable panel sizing via `config/layout.conf` and theme presets via `config/themes.conf`
- Mode-based keyboard workflow (`normal`, `insert`, `command`, `search`)
- Vim-like command line (`:`) and search (`/`) in the bottom bar
- Request editing:
  - URL field
  - dedicated BODY + HEADERS split editor (`classic` / `quad`)
  - tabbed editor (`URL | BODY | HEADERS`) in `focus_editor`
  - multiline body
  - multiline headers
  - headers autocomplete from `config/headers.txt`
- HTTP methods: `GET`, `POST`, `PUT`, `DELETE`
- Environments and variables from `config/envs.json`
- Template substitution for `{{VAR}}` in URL, BODY, and HEADERS
- Async requests (worker thread) so UI stays responsive
- Response metadata:
  - status code
  - elapsed time
  - payload size
  - JSON indicator
- JSON pretty-print (fallback to raw text)
- Persistent request history (JSONL) with load-back into editor/response
- Replay from history snapshots
- Contextual search with next/prev navigation (`n` / `N`)

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

## Keybindings (Default)

From `config/keymap.conf`:

- `h` / `l`: focus left/right panel
- `j` / `k`: move down/up (history selection or response scroll)
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
- `:theme list`: list available theme presets
- `:theme <name>`: apply theme preset for current session
- `:theme <name> -s` or `:theme <name> --save`: apply and persist active preset to `config/layout.conf`
- `:clear!` or `:ch!`: clear history from memory and persisted storage

## History Configuration

Persistent history retention is configurable in `config/history.conf`:

```ini
max_entries = 500
```

The app keeps only the newest `max_entries` requests.

## Environments

Environments are loaded from `config/envs.json`:

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

Header suggestions come from `config/headers.txt` (one header per line, `#` comments allowed).
In insert mode with HEADERS active, press `Shift+Tab` to complete/cycle matching header names.

## Layout Profiles

Layout is loaded at startup from `config/layout.conf`:

```ini
profile = classic
theme_preset = vivid
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
- `theme_preset` points to a preset name from `config/themes.conf`.
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
- With `-s`/`--save`, `config/layout.conf` is rewritten in canonical format and updates `theme_preset`.

## Theme Presets File

Theme presets are loaded from `config/themes.conf` using INI-style sections:

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

## Project Structure

- `src/core/`: core logic (http, dispatch, keymap, history, text buffer)
- `src/ui/`: ncurses drawing and input handling
- `include/`: public/internal headers
- `config/`: keymap configuration
- `scripts/`: setup helpers

## Current Limitations / Roadmap

- `command` and `search` modes exist, but advanced behaviors are still minimal
- No request export yet (`curl`/JSON)
- No advanced auth helpers yet

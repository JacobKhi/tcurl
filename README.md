# tcurl

`tcurl` is a terminal HTTP client (TUI) written in C with `ncurses`.
It follows a keyboard-driven workflow inspired by vim while using real HTTP requests through `libcurl`.

## Current Features

- 3-panel layout:
  - `History` (left)
  - `Editor` (top-right)
  - `Response` (bottom-right)
- Mode-based keyboard workflow (`normal`, `insert`, `command`, `search`)
- Request editing:
  - URL field
  - multiline body
  - multiline headers
- HTTP methods: `GET`, `POST`, `PUT`, `DELETE`
- Async requests (worker thread) so UI stays responsive
- Response metadata:
  - status code
  - elapsed time
  - payload size
  - JSON indicator
- JSON pretty-print (fallback to raw text)
- Persistent request history (JSONL) with load-back into editor/response
- Replay from history snapshots

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

- `q`: quit
- `h` / `l`: focus left/right panel
- `j` / `k`: move down/up (history selection or response scroll)
- `i`: enter insert mode
- `esc`: return to normal mode
- `tab`: cycle editor field (`URL` -> `BODY` -> `HEADERS`)
- `m`: cycle HTTP method
- `r`: send request
- `enter` (in `History` panel): load selected history item
- `Shift+Enter` (in `History` panel): replay selected history item
- `R` (in `History` panel): replay fallback when terminal does not emit `Shift+Enter`

## History Configuration

Persistent history retention is configurable in `config/history.conf`:

```ini
max_entries = 500
```

The app keeps only the newest `max_entries` requests.

## Project Structure

- `src/core/`: core logic (http, dispatch, keymap, history, text buffer)
- `src/ui/`: ncurses drawing and input handling
- `include/`: public/internal headers
- `config/`: keymap configuration
- `scripts/`: setup helpers

## Current Limitations / Roadmap

- `command` and `search` modes exist, but advanced behaviors are still minimal
- No environment variables templating yet (e.g. `{{BASE_URL}}`)
- No request export yet (`curl`/JSON)
- No advanced auth helpers yet

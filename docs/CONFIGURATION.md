# Configuration Guide

Configuration files are located in `~/.config/tcurl/` (or `$XDG_CONFIG_HOME/tcurl/`).

## Configuration Files

### layout.conf

Controls UI layout, themes, and language settings.

```conf
# Layout profile: classic, quad, or focus_editor
layout = classic

# UI language: auto, en, or pt
language = auto

# Show footer hint line
show_footer_hint = 1

# Active theme preset
theme_preset = default

# Panel sizing (percentages)
classic_history_width_pct = 30
classic_editor_height_pct = 50
focus_editor_height_pct = 60
quad_split_x_pct = 50
quad_split_y_pct = 50

# Quad layout slot assignments
quad_history_slot = tl
quad_editor_slot = tr
quad_response_slot = bl
```

### themes.conf

Define custom color themes.

```conf
[default]
use_colors = 1
focus_fg = yellow
focus_bg = default
tab_active_fg = black
tab_active_bg = cyan
gutter_fg = cyan
gutter_bg = default
warn_fg = yellow
warn_bg = default
error_fg = red
error_bg = default

[dracula]
use_colors = 1
focus_fg = magenta
focus_bg = default
# ... more colors
```

Available color names:
- `default` (terminal default)
- `black`, `red`, `green`, `yellow`
- `blue`, `magenta`, `cyan`, `white`

### keymap.conf

Customize keyboard bindings for each mode.

```conf
[normal]
q = ACT_QUIT
h = ACT_FOCUS_LEFT
l = ACT_FOCUS_RIGHT
k = ACT_MOVE_UP
j = ACT_MOVE_DOWN
i = ACT_ENTER_INSERT
: = ACT_ENTER_COMMAND
/ = ACT_ENTER_SEARCH
Enter = ACT_SEND_REQUEST
Tab = ACT_TOGGLE_EDITOR_FIELD
m = ACT_CYCLE_METHOD
e = ACT_CYCLE_ENV
o = ACT_HISTORY_LOAD
r = ACT_HISTORY_REPLAY
n = ACT_SEARCH_NEXT
N = ACT_SEARCH_PREV

[insert]
Escape = ACT_ENTER_NORMAL

[command]
Escape = ACT_ENTER_NORMAL

[search]
Escape = ACT_ENTER_NORMAL
```

Available actions:
- `ACT_QUIT` - Exit application
- `ACT_MOVE_UP` - Move up in panel
- `ACT_MOVE_DOWN` - Move down in panel
- `ACT_FOCUS_LEFT` - Focus left panel
- `ACT_FOCUS_RIGHT` - Focus right panel
- `ACT_ENTER_INSERT` - Enter insert mode
- `ACT_ENTER_NORMAL` - Return to normal mode
- `ACT_ENTER_COMMAND` - Open command prompt
- `ACT_ENTER_SEARCH` - Open search prompt
- `ACT_SEND_REQUEST` - Send HTTP request
- `ACT_TOGGLE_EDITOR_FIELD` - Cycle editor fields
- `ACT_CYCLE_METHOD` - Cycle HTTP methods
- `ACT_CYCLE_ENV` - Cycle environments
- `ACT_HISTORY_LOAD` - Load history item
- `ACT_HISTORY_REPLAY` - Replay history request
- `ACT_SEARCH_NEXT` - Next search result
- `ACT_SEARCH_PREV` - Previous search result

### envs.json

Define environment variables for request templates.

```json
{
  "dev": {
    "API_URL": "http://localhost:3000",
    "API_KEY": "dev-key-123"
  },
  "staging": {
    "API_URL": "https://staging.api.example.com",
    "API_KEY": "staging-key-456"
  },
  "prod": {
    "API_URL": "https://api.example.com",
    "API_KEY": "prod-key-789"
  }
}
```

Use variables in requests with `{{VAR_NAME}}` syntax:
- URL: `{{API_URL}}/users`
- Headers: `Authorization: Bearer {{API_KEY}}`
- Body: `{"endpoint": "{{API_URL}}"}`

### headers.txt

Common headers for autocomplete.

```
Content-Type: application/json
Authorization: Bearer 
Accept: application/json
User-Agent: tcurl/1.0
X-API-Key: 
X-Request-ID: 
```

### history.conf

History persistence settings.

```conf
# Maximum entries to keep in memory
max_entries = 100

# History file location (relative to config dir)
history_file = history.jsonl
```

## Layout Profiles

### classic
- History panel on left
- Editor panel on top-right
- Response panel on bottom-right
- Good for standard workflow

### quad
- 2x2 grid layout
- Customizable slot assignments
- Flexible panel positioning
- Configurable split positions

### focus_editor
- Large editor on top (60% height default)
- History and Response panels at bottom
- Tabbed editor (URL | BODY | HEADERS)
- Good for editing large payloads

## Runtime Commands

Change settings at runtime using commands:

### Language
```
:lang list               # List available languages
:lang auto               # Auto-detect from LANG env
:lang en                 # Set to English
:lang pt                 # Set to Portuguese
```

### Layout
```
:layout list             # List available layouts
:layout classic          # Switch to classic layout
:layout quad             # Switch to quad layout
:layout focus_editor     # Switch to focus_editor layout
```

### Theme
```
:theme list              # List available themes
:theme dracula           # Apply dracula theme (session only)
:theme monokai --save    # Apply and save to config
```

### Settings
```
:set                           # Show current settings
:set search_target auto        # Auto search context
:set search_target history     # Always search history
:set search_target response    # Always search response
:set max_entries 500           # Increase history limit
```

All runtime changes (except when using `--save`) apply to the current session only.

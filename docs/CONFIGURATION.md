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
h = focus_left
l = focus_right
j = move_down
k = move_up
left = focus_left
right = focus_right
down = move_down
up = move_up
i = enter_insert
":" = enter_command
"/" = enter_search
tab = toggle_editor_field
r = send_request
m = cycle_method
E = cycle_environment
n = search_next
N = search_prev
enter = history_load
s-enter = history_replay
R = history_replay

[insert]
esc = enter_normal

[command]
esc = enter_normal

[search]
esc = enter_normal
```

Available actions:
- `quit` - Exit application
- `move_up` - Move up in panel
- `move_down` - Move down in panel
- `focus_left` - Focus left panel
- `focus_right` - Focus right panel
- `enter_insert` - Enter insert mode
- `enter_normal` - Return to normal mode
- `enter_command` - Open command prompt
- `enter_search` - Open search prompt
- `send_request` - Send HTTP request
- `toggle_editor_field` - Cycle editor fields (URL, BODY, HEADERS)
- `cycle_method` - Cycle HTTP methods
- `cycle_environment` - Cycle environments
- `history_load` - Load history item into editor
- `history_replay` - Replay history request immediately
- `search_next` - Next search result
- `search_prev` - Previous search result
- `toggle_response_view` - Toggle between response body and headers

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

Header names for autocomplete in the HEADERS editor field.

```
Authorization
Content-Type
Accept
User-Agent
X-API-Key
X-Request-ID
```

### history.conf

History persistence settings.

```conf
# Maximum entries to keep in memory
max_entries = 500
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

### Cookies
```
:cookies list            # List stored cookies
:cookies clear           # Clear all cookies
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

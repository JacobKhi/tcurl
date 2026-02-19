# tcurl Command Reference

Complete reference for all commands available in tcurl. Commands are entered in command mode (press `:` in normal mode).

---

## BASIC COMMANDS

### :q | :quit
Exit the application.

**Usage:**
```
:q
:quit
```

**Example:**
```
:q
```

---

### :h | :help
Display comprehensive help text including all commands, navigation hints, and key bindings.

**Usage:**
```
:h
:help
```

**Example:**
```
:help
```

---

## LANGUAGE

### :lang list
List all available UI languages.

**Usage:**
```
:lang list
```

**Output:**
```
Available languages:
- auto (detect from LANG env)
- en (English)
- pt (Portuguese)
```

---

### :lang <auto|en|pt>
Set the UI language for the current session.

**Usage:**
```
:lang <language>
```

**Parameters:**
- `auto`: Auto-detect from LANG environment variable
- `en`: English
- `pt`: Portuguese (Português)

**Example:**
```
:lang en
:lang pt
:lang auto
```

**Notes:**
- Changes apply to current session only
- Restart required for persistent language change (edit layout.conf)
- Auto mode detects from LANG environment variable

---

## LAYOUT

### :layout list
List all available layout profiles.

**Usage:**
```
:layout list
```

**Output:**
```
Available layouts:
- classic (History left, Editor/Response right)
- quad (2x2 quadrant)
- focus_editor (Large editor on top)
```

---

### :layout <name>
Set the layout profile for the current session.

**Usage:**
```
:layout <profile>
```

**Parameters:**
- `classic`: Traditional layout (History left, Editor/Response right)
- `quad`: 2x2 grid layout with configurable slots
- `focus_editor`: Large editor on top, History/Response at bottom

**Example:**
```
:layout classic
:layout quad
:layout focus_editor
```

**Notes:**
- Changes apply to current session only
- Restart required for persistent layout change (edit layout.conf)
- Panel sizes configured in layout.conf still apply

---

## THEME MANAGEMENT

### :theme list
List all available theme presets.

**Usage:**
```
:theme list
```

**Output:**
Displays a formatted list of all theme presets available in the theme catalog.

---

### :theme <name>
Apply a theme preset for the current session only. Changes will not persist after restart.

**Usage:**
```
:theme <preset_name>
```

**Available Presets:**
- default
- dracula
- monokai
- gruvbox
- nord
- solarized

**Example:**
```
:theme dracula
```

---

### :theme <name> -s | --save
Apply a theme preset and save it as the active theme. Changes will persist across sessions.

**Usage:**
```
:theme <preset_name> -s
:theme <preset_name> --save
```

**Example:**
```
:theme monokai --save
```

**Notes:**
- Theme configuration is saved to the user's config file
- Both `-s` and `--save` flags are equivalent

---

## EXPORT

### :export curl
Export the current HTTP request as a curl command that can be copied and executed in a terminal.

**Usage:**
```
:export curl
```

**Output:**
Displays a fully formatted curl command including:
- HTTP method
- URL with environment variable interpolation
- Headers
- Request body (if applicable)
- Authentication headers

**Example Output:**
```
curl -X POST 'https://api.example.com/users' \
  -H 'Content-Type: application/json' \
  -H 'Authorization: Bearer token123' \
  -d '{"name": "John"}'
```

---

### :export json
Export the current HTTP request as a JSON object suitable for programmatic use or documentation.

**Usage:**
```
:export json
```

**Output:**
Displays a JSON representation of the request including:
- method
- url
- headers (as object)
- body

**Example Output:**
```json
{
  "method": "POST",
  "url": "https://api.example.com/users",
  "headers": {
    "Content-Type": "application/json",
    "Authorization": "Bearer token123"
  },
  "body": "{\"name\": \"John\"}"
}
```

---

## AUTHENTICATION

### :auth bearer <token>
Set or update the Authorization header with a bearer token. Automatically formats the header as `Authorization: Bearer <token>`.

**Usage:**
```
:auth bearer <token>
```

**Parameters:**
- `<token>`: The bearer token value (without "Bearer" prefix)

**Example:**
```
:auth bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxMjM0NTY3ODkwIn0
```

**Result:**
Adds or updates the Authorization header:
```
Authorization: Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxMjM0NTY3ODkwIn0
```

---

### :auth basic <user>:<pass>
Set or update the Authorization header with basic authentication. Automatically encodes credentials in base64 and formats the header as `Authorization: Basic <encoded>`.

**Usage:**
```
:auth basic <username>:<password>
```

**Parameters:**
- `<username>:<password>`: Credentials separated by colon

**Example:**
```
:auth basic admin:secret123
```

**Result:**
Adds or updates the Authorization header:
```
Authorization: Basic YWRtaW46c2VjcmV0MTIz
```

**Notes:**
- Credentials are base64 encoded automatically
- The colon separator is required
- Use caution with sensitive credentials

---

## SEARCH

### :find <term>
Execute a contextual search immediately. The search context depends on the currently focused panel or the `search_target` setting.

**Usage:**
```
:find <search_term>
```

**Parameters:**
- `<search_term>`: Text to search for (case-insensitive)

**Search Context:**
- **history panel focused:** Searches in request history
- **response panel focused:** Searches in response body
- **search_target override:** Follows configured target

**Example:**
```
:find error
:find user_id
:find 200
```

**Notes:**
- Search is case-insensitive
- Results are highlighted in the target panel
- Use `n` and `N` key bindings to navigate matches (if configured)

---

## HISTORY

### :clear! | :ch!
Clear all request history from both memory and persistent storage. The exclamation mark is required to confirm the destructive operation.

**Usage:**
```
:clear!
:ch!
```

**Example:**
```
:clear!
```

**Result:**
- All history entries are removed from memory
- History storage file is cleared
- Cannot be undone

**Notes:**
- The `!` is mandatory to prevent accidental deletion
- Command will fail if a request is currently in flight
- Both `:clear!` and `:ch!` are equivalent

---

## SETTINGS

### :set
Display current runtime settings and their values.

**Usage:**
```
:set
```

**Output:**
```
Settings:
- search_target=auto
- history_max_entries=100
```

---

### :set search_target <value>
Configure where search operations look for matches.

**Usage:**
```
:set search_target <value>
```

**Parameters:**
- `auto`: Automatically detect based on focused panel (default)
- `history`: Always search in request history
- `response`: Always search in response body

**Example:**
```
:set search_target auto
:set search_target history
:set search_target response
```

**Notes:**
- Setting applies to current session only
- Default is `auto` which respects panel focus

---

### :set max_entries <number>
Configure the maximum number of history entries to retain in memory.

**Usage:**
```
:set max_entries <number>
```

**Parameters:**
- `<number>`: Positive integer for max history entries

**Example:**
```
:set max_entries 500
:set max_entries 50
```

**Notes:**
- Setting applies to current session only
- If the new limit is lower than current entries, oldest entries are trimmed
- Does not affect persistent storage limit

---

## NAVIGATION

In normal mode, arrow keys mirror vim-style navigation:
- **Left/Right**: Same as `h`/`l` - Focus left/right panels
- **Up/Down**: Same as `k`/`j` - Scroll or navigate items

---

## KEY BINDINGS

Key bindings vary by mode. Use `:help` to see all configured bindings for:
- **normal mode**: Default state for navigation and commands
- **insert mode**: Text editing in URL, headers, and body
- **command mode**: Entering commands (after pressing `:`)
- **search mode**: Entering search terms (after pressing `/` or configured key)

See the output of `:help` command for a complete list of all key bindings configured in your keymap.

---

## NOTES

### Command Parsing
- Commands are case-sensitive
- Leading/trailing whitespace is ignored
- The `:` prefix is optional in command mode
- Invalid commands show an error message

### Environment Variables
- URL and header fields support environment variable interpolation
- Format: `${VAR_NAME}` or `$VAR_NAME`
- Variables are resolved from the active environment

### Multiple Environments
- Use configured key bindings to cycle through environments
- Current environment is shown in the top bar
- Each environment can have different variable definitions

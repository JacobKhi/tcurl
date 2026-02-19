# Usage Guide

## Basic Workflow

1. Start tcurl: `./tcurl`
2. Enter insert mode: `i`
3. Type URL and edit body/headers
4. Return to normal mode: `Escape`
5. Send request: `Enter`
6. View response in right panel

## Modes

### Normal Mode
- Navigate with `hjkl` or arrow keys
- Focus panels with `h` (left) and `l` (right)
- Scroll with `j` (down) and `k` (up)
- Enter commands with `:`
- Search with `/`

### Insert Mode
- Edit URL, body, or headers
- Type normally
- Return to normal with `Escape`

### Command Mode
- Execute commands (`:command`)
- Command history with Up/Down arrows
- Exit with `Escape` or `Enter`

### Search Mode
- Enter search term
- Navigate results with `n` (next) and `N` (previous)
- Exit with `Escape`

## HTTP Methods

Cycle through methods with `m` in normal mode:
- GET
- POST
- PUT
- DELETE

## Environments

Define variables in `envs.json`, cycle with `e` in normal mode.

Use variables in requests:
- `{{VAR_NAME}}` in URL, headers, or body
- Variables are replaced before sending request

Example:
```
URL: {{API_URL}}/users
Header: Authorization: Bearer {{API_TOKEN}}
Body: {"environment": "{{ENV_NAME}}"}
```

## History

### Loading from History
1. Focus history panel (`h` from normal mode)
2. Navigate with `j`/`k`
3. Load request with `o`
4. Replay immediately with `r`

### Clearing History
```
:clear!    # Clear all history (requires !)
```

## Search

### Contextual Search
- In history panel: Searches request URLs and metadata
- In response panel: Searches response body
- Override with `:set search_target`

### Search Commands
```
/search_term     # Open search mode
n                # Next match
N                # Previous match
:find term       # Search immediately
```

## Export

Export current request to clipboard or file:

```
:export curl     # Generate curl command
:export json     # Generate JSON representation
```

## Authentication

### Bearer Token
```
:auth bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...
```
Adds: `Authorization: Bearer <token>`

### Basic Auth
```
:auth basic username:password
```
Adds: `Authorization: Basic <base64(username:password)>`

## Layout Switching

Change layout on the fly:

```
:layout classic         # Traditional layout
:layout quad            # 2x2 grid
:layout focus_editor    # Large editor
```

## Language Switching

Change UI language:

```
:lang en    # English
:lang pt    # Portuguese
:lang auto  # Auto-detect
```

## Tips and Tricks

### Quick Request Editing
1. Load from history (`o`)
2. Enter insert mode (`i`)
3. Modify URL/body/headers
4. Send (`Escape` then `Enter`)

### Template Workflow
1. Create request with variables
2. Save to history (automatic on send)
3. Switch environments with `e`
4. Replay with same request, different vars

### Search Workflow
1. Send request
2. `/` to search response
3. `n`/`N` to navigate matches
4. Useful for finding errors in large responses

### Multi-line Bodies
- Press `Enter` in insert mode for new lines
- JSON payloads can be formatted across lines
- Headers: one per line

### Panel Focus Flow
- `h` - Focus history (left)
- `l` - Focus editor (middle/right)
- `l` again - Focus response (far right)
- Depends on current layout

### Keyboard Efficiency
- Stay in normal mode most of the time
- Use `i` for quick edits
- Use commands (`:`) for complex operations
- Bind custom keys in `keymap.conf`

## Common Workflows

### API Testing
1. Set up environments in `envs.json`
2. Create requests with variable placeholders
3. Cycle environments with `e` <result>
4. Review responses
5. Export working requests with `:export curl`

### Debugging
1. Send request
2. Check status code and response time
3. Search response with `/` for errors
4. Modify request and resend
5. Compare with history

### Documentation
1. Build request collection in history
2. Export each with `:export json`
3. Use exports for API documentation
4. Share curl commands with team

## Troubleshooting

### Request Not Sending
- Check URL format (include `http://` or `https://`)
- Verify environment variables are defined
- Check for variable placeholder typos

### Response Not Showing
- Wait for request to complete
- Check for errors in response panel
- Review request in history for clues

### Search Not Working
- Verify search target with `:set`
- Check if search term is spelled correctly
- Search is case-insensitive

### Layout Issues
- Reset with `:layout classic`
- Check `layout.conf` for corrupt settings
- Try different terminal size

See [COMMANDS.md](../COMMANDS.md) for complete command reference.

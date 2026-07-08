# Editor Behaviour

## Cursor movement

- Arrow keys move by character/line.
- Cursor column clamps to line length when moving vertically.
- Later: preserve preferred visual column when moving through shorter lines.

## Modes

- Existing files always open in read mode.
- Read mode permits navigation and viewing only.
- Pressing `w` or `W` enters write mode.
- Missing file paths open as new unsaved documents in write mode.
- The status bar shows READ or WRITE.
- Editing commands are ignored in read mode and should show a short status message.

## Editing

MVP actions:

- insert UTF-8 text without corrupting existing non-ASCII bytes
- backspace
- delete
- enter/newline
- save
- quit

Later actions:

- cut line
- duplicate line
- move line up/down
- join lines
- auto-indent
- trim trailing whitespace

Tabs insert literal tab characters by default. Later settings may change insertion to exactly one of three options: literal tab, 2 spaces, or 4 spaces.

## Status bar

Show:

- filename
- modified marker
- line number
- column number
- encoding
- line ending style
- READ/WRITE mode
- short status message

## Confirmation prompts

Required before:

- replacing all matches
- deleting a file
- discarding external changes

`Ctrl+Q` and macOS `Option+Q` quit immediately. They do not prompt for unsaved changes.

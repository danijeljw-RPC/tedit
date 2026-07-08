# File Navigation

## MVP

- accept file path from command line
- save current file
- existing files open in read mode
- missing command-line paths create a new unsaved document in write mode
- save materializes a missing-path document at the requested path
- save existing files atomically and preserve permissions where practical
- preserve original LF or CRLF line endings on save

## Near-term

- open file prompt
- save as prompt
- recent files list
- basic file browser pane or dialog

## File browser

Minimum useful file browser:

- current directory display
- parent directory entry
- directories first
- files second
- enter opens directory or file
- escape closes browser

## Destructive actions

Deleting files must always require confirmation.

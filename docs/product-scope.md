# Product Scope

## MVP

- open one text file
- display one document
- raw terminal mode
- macOS, Linux, and Windows CMake configure/build/test parity
- platform input and terminal abstractions for POSIX and Windows
- read mode for existing files
- write mode entered with `w` / `W`
- cursor movement
- vertical scrolling
- per-line horizontal scrolling for the active line
- basic insert/delete/newline editing in write mode
- create missing command-line path as a new unsaved document in write mode
- atomic save
- dirty indicator
- terminal resize handling
- basic status bar
- UTF-8 byte preservation where practical
- LF/CRLF line-ending preservation

## Near-term

- undo/redo
- find
- find next/previous
- highlight matches
- replace current
- replace all
- selection with Shift+arrows
- cut/copy/paste internal clipboard
- cut line
- save as
- open file prompt
- basic file browser
- line numbers
- visible whitespace toggle
- tab insertion setting: literal tab, 2 spaces, or 4 spaces

## Later

- tabs / multiple open files
- desktop-style menu bar
- config file
- themes
- nano syntax file import
- native syntax format
- spelling
- OS clipboard
- mouse support
- recent files
- session restore
- syntax highlighting themes
- Tree-sitter adapter
- recovery files
- external modification detection

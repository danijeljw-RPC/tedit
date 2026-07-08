# Architecture

The editor must be model-driven, not print-driven.

The core editor state owns the document, cursor, selection, viewports, undo stack, search state, and dirty state. The renderer only paints that state.

The architecture must remain cross-platform from the first milestone. macOS can be the primary manual development platform, but platform boundaries must be real enough that macOS, Linux, and Windows builds stay green at the end of every milestone.

## Layers

```text
src/core
  document model
  text buffer and line ending model
  editor state
  read/write mode state
  cursor movement
  selection model
  search/replace
  undo/redo
  file operations

src/ui
  editor view
  menu model
  dialogs
  status bar
  file browser model

src/renderer
  screen buffer
  ANSI renderer
  dirty-cell/diff renderer

src/platform
  POSIX terminal raw mode
  Windows console mode
  terminal size
  input events
  clipboard later
```

## Dependency direction

Allowed:

- `main` depends on all layers
- `ui` depends on `core`
- `renderer` depends on `core` state and platform write calls
- `platform` depends on OS APIs
- `core` depends only on the C standard library where possible

Avoid:

- core calling terminal APIs
- document code emitting ANSI
- platform code mutating editor state directly
- renderer code performing editor actions
- POSIX assumptions leaking into core or UI code
- Windows support being deferred behind placeholder behavior for completed milestone features

## Main loop

```text
init platform
load document
enter raw terminal mode
loop:
  read terminal size
  process queued input
  update editor state
  update viewport
  render frame
leave raw terminal mode
cleanup
```

## State objects

Minimum state objects:

- `Document`
- `TextLine`
- `Editor`
- `Cursor`
- `Viewport`
- `EditMode`
- `LineEnding`
- `Selection`
- `UndoStack`
- `SearchState`
- `ScreenBuffer`
- `Platform`

## Save model

Saves must be crash-safe from MVP-001:

1. write to temporary file in same directory
2. flush
3. atomically rename over original
4. preserve permissions where practical

Existing files open in read mode. The editor only mutates the document after write mode is enabled with `w` or `W`. Missing command-line paths create a new in-memory document, enter write mode immediately, and materialize the file on save.

Loaded files should preserve their original line endings on save unless the user changes settings later. Text handling should be UTF-8 oriented and byte-preserving where practical so non-ASCII content is not corrupted, even before full grapheme and display-width support exists.

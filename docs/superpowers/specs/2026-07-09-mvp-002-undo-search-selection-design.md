# MVP-002 Undo, Search, And Selection Design

## Goal

MVP-002 makes the editor safer and more useful for real editing by adding undo/redo, search/replace, visible search highlights, selection, and an internal clipboard. The implementation must preserve MVP-001 behavior, keep macOS/Linux/Windows CMake builds viable, and keep editor behavior model-driven rather than terminal-driven.

## Current Context

MVP-001 is locally verified on macOS with CMake configure, build, tests, and a PTY smoke test. The current editor already has a line-based `Document`, `Editor` cursor and viewport state, raw terminal input adapters, and immediate ANSI rendering. `UndoStack` and `SearchState` currently exist only as stubs, and no selection or clipboard state exists yet.

## Recommended Approach

Use a core-first incremental implementation.

Add real core state for undo, search, selection, and clipboard, then wire platform key mappings and ANSI rendering to that state. Document editing should gain range-based helpers so selection deletion, paste, replace current, replace all, and undo replay share the same mutation path where practical.

This avoids a broad text-buffer rewrite while still creating testable seams for MVP-002 behavior.

## Keybindings

Default MVP-002 bindings:

- `Ctrl+Z`: undo
- `Ctrl+Y`: redo
- `Ctrl+F`: begin find prompt
- `Ctrl+G`: find next
- `Ctrl+P`: find previous
- `Ctrl+R`: replace current
- `Ctrl+A`: replace all with confirmation
- `Shift+Left/Right/Up/Down`: extend selection
- `Ctrl+X`: cut selection
- `Ctrl+C`: copy selection
- `Ctrl+V`: paste internal clipboard
- `Ctrl+K`: cut current line when no selection exists

All mutating commands remain blocked in read mode. Read mode permits search navigation and copy, but blocks replace, paste, cut, cut line, undo, redo, and selected-text deletion.

## Core State

Extend `Editor` with:

- `UndoStack undo`: stores undo and redo groups.
- `SearchState search`: stores query text, current match, match count, and prompt state.
- `Selection selection`: stores anchor and active cursor positions plus an active flag.
- `char *clipboard`: stores the internal clipboard contents as bytes.

Selection endpoints are document coordinates. A normalized selection range should be derived when editing or rendering, so dragging backward and forward produces identical delete/copy behavior.

## Document Operations

Keep the line-based document model. Add focused helpers instead of replacing the text buffer:

- insert bytes at a document position
- delete a normalized range
- extract a normalized range into an allocated byte string
- replace a normalized range with bytes
- compare and normalize document positions

Range text uses `\n` between document lines internally. Saving continues to preserve the document's loaded LF/CRLF line-ending style.

## Undo And Redo

Undo entries should store enough information to reverse and replay a document mutation:

- before range
- deleted bytes
- inserted bytes
- cursor before and after
- group id

Grouped printable insertion should undo as one unit while contiguous text is typed at adjacent positions. Operations such as newline, backspace, delete, paste, cut, replace current, and replace all should close the current insert group.

Replace all must be one undo group, so a single `Ctrl+Z` restores the document to its state before replace all.

Redo is cleared when a new edit is recorded after undo.

## Search And Replace

Search is literal, case-sensitive byte search for MVP-002. Later toggles can add case-insensitive, whole-word, and regex behavior.

The find prompt stores a query in `SearchState`. While the prompt is active, printable input edits the query, backspace removes bytes from the query, Enter accepts the query, and Escape cancels the prompt. A parallel replace prompt may be used for replace current and replace all, but the implementation should keep prompt state in core rather than platform code.

Find next and previous move the cursor to the next match, wrapping through the document. Visible matches are highlighted during render. The status message should show no-match feedback or match position/count when available.

Replace current replaces the current search match if the editor is in write mode. Replace all asks for confirmation before mutating and records the whole operation as one undo group.

## Selection And Clipboard

Shift+arrow movement extends the active selection from the original anchor to the moved cursor position. Plain cursor movement clears the selection unless the movement is part of a selection-extending command.

Typing while a selection exists replaces the selected range in write mode. Backspace and Delete delete the selected range in write mode.

Copy stores selected text in the internal clipboard without mutating the document and is allowed in read mode. Cut stores selected text and deletes it in write mode. Paste inserts the clipboard at the cursor or replaces the current selection in write mode.

`Ctrl+K` cuts the current line when there is no active selection. If a selection exists, `Ctrl+K` should behave like cut selection to avoid two competing cut semantics.

Selection is stored in document coordinates, so it survives viewport movement and scrolling.

## Rendering

The ANSI renderer reads search and selection state from `Editor`.

Search highlights apply to all visible matches. Selection highlighting applies to the selected visible ranges. When a selected byte range overlaps a search match, selection highlighting wins because it communicates the active edit target.

Rendering remains immediate ANSI output for MVP-002. Dirty-cell diff rendering remains outside this milestone.

## Platform Input

Add platform key constants for the MVP-002 control keys and Shift+arrow variants. POSIX escape parsing should recognize common Shift+arrow forms such as `ESC [ 1 ; 2 A/B/C/D`. Windows input should map Shift+arrow using `dwControlKeyState`.

Prompt editing, undo, selection, search, and clipboard behavior remain in `src/core`, not in platform code.

## Tests

Add focused C tests for:

- undo and redo of insert/delete/newline edits
- grouped printable insertion undo
- redo clearing after a new edit
- replace all as one undo group
- find next and previous with wrapping
- visible-match state for renderer consumption
- selection deletion
- typing over selection
- selection surviving viewport movement
- cut, copy, paste, and cut line with internal clipboard
- read-mode blocking for mutating MVP-002 commands
- Shift+arrow escape/key mapping on POSIX-compatible parser paths

Local verification remains:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
git diff --check
```

## Out Of Scope

- OS clipboard integration
- mouse selection
- rectangular/block selection
- regex search
- case-insensitive search toggle
- whole-word search toggle
- syntax highlighting
- dirty-cell diff rendering
- unsaved-change quit prompts

## Acceptance Criteria

- Text edits can be undone and redone.
- Grouped printable insertion undoes as one edit.
- Replace all is one undo group.
- Search highlights visible matches.
- Find next and previous wrap through the document.
- Selection survives viewport movement.
- Deleting selected text is tested.
- Cut, copy, paste, and cut line use the internal clipboard.
- Read mode continues to block mutating actions.
- CMake configure, build, tests, and `git diff --check` pass locally.
- The implementation remains portable for macOS, Linux, and Windows CMake builds.

# MVP-002 Undo, Search, And Selection Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Complete MVP-002 by adding undo/redo, search/replace, visible highlights, selection, and internal clipboard behavior while preserving MVP-001 editor behavior.

**Architecture:** Keep the editor model-driven. Core state owns document ranges, undo/redo, search, selection, clipboard, and prompt state; platform code only maps terminal input to key constants; renderer only paints state.

**Tech Stack:** C17, CMake 3.20+, existing assert-based C tests, POSIX terminal escape parsing, Windows Console API key records, immediate ANSI renderer.

## Global Constraints

- Preserve MVP-001 behavior and local CMake configure/build/test flow.
- Keep macOS, Linux, and Windows CMake builds viable.
- Keep platform-specific code under `src/platform`.
- Keep editor state and behavior under `src/core`.
- Keep rendering under `src/renderer`.
- Do not add dependencies, ncurses, GUI, plugins, Tree-sitter, LSP, or network functionality.
- Preserve UTF-8 bytes and LF/CRLF line endings where practical.
- All mutating MVP-002 commands remain blocked in read mode.
- Read mode permits navigation, search navigation, and copy.
- Replace all must be one undo group.
- Selection must be stored in document coordinates so it survives viewport movement.

---

## File Structure

- Modify `src/core/document.h` and `src/core/document.c`: add document positions, ranges, text extraction, range delete, and range replace helpers.
- Modify `src/core/undo.h` and `src/core/undo.c`: replace stubs with undo/redo operation storage and grouped edit APIs.
- Modify `src/core/search.h` and `src/core/search.c`: replace stubs with literal byte-search state and navigation helpers.
- Modify `src/core/editor.h` and `src/core/editor.c`: own selection, clipboard, prompts, undo/search integration, read/write gating, and key handling.
- Modify `src/platform/platform.h`, `src/platform/terminal_posix.c`, and `src/platform/terminal_win32.c`: add MVP-002 key constants and input mappings.
- Modify `src/renderer/ansi_renderer.c`: paint visible selection and search highlights from `Editor` state.
- Modify `tests/test_main.c`: add focused tests before implementation for each behavior slice.
- Modify `README.md` and `docs/plans/MVP-002.md`: document completed MVP-002 behavior and default keybindings after implementation.

---

### Task 1: Document Range Primitives

**Files:**
- Modify: `src/core/document.h`
- Modify: `src/core/document.c`
- Test: `tests/test_main.c`

**Interfaces:**
- Produces: `DocumentPosition`, `DocumentRange`, `document_position_compare`, `document_range_normalize`, `document_extract_range`, `document_delete_range`, `document_replace_range`, `document_insert_bytes`.
- Later tasks consume these helpers for selection, paste, replace, and undo replay.

- [x] **Step 1: Add failing document range tests**

Add tests to `tests/test_main.c` that:

- Build a two-line document containing `abc` and `def`.
- Extract range `(0,1)..(1,2)` and assert it returns `bc\nde`.
- Delete the same range and assert the document becomes one line `af`.
- Replace range `(0,1)..(0,2)` with `XY\nZ` and assert lines become `aXY` and `Zf`.

Run: `cmake --build build && ctest --test-dir build --output-on-failure`

Expected: compile failure for missing range types/functions.

- [x] **Step 2: Implement range primitives**

Add public range types to `document.h`:

```c
typedef struct {
    size_t row;
    size_t col;
} DocumentPosition;

typedef struct {
    DocumentPosition start;
    DocumentPosition end;
} DocumentRange;
```

Add helpers that clamp positions to valid document coordinates, normalize backward ranges, preserve `doc->dirty` on mutation, and allocate extracted text with `malloc`.

- [x] **Step 3: Verify Task 1**

Run: `cmake --build build && ctest --test-dir build --output-on-failure`

Expected: all tests pass.

---

### Task 2: Undo/Redo Core

**Files:**
- Modify: `src/core/undo.h`
- Modify: `src/core/undo.c`
- Modify: `src/core/editor.h`
- Modify: `src/core/editor.c`
- Test: `tests/test_main.c`

**Interfaces:**
- Consumes: document range primitives from Task 1.
- Produces: editor-level undo/redo behavior for insert, delete, newline, replace, paste, and cut operations.

- [x] **Step 1: Add failing undo/redo tests**

Add tests that:

- Insert `abc` through `editor_handle_key`, undo once, and assert the line is empty.
- Redo once and assert the line is `abc`.
- Insert `abc`, undo, type `Z`, then redo and assert redo does not restore `abc`.
- Type `A`, newline, `B`, undo newline group, and assert the document is one line `A`.

Run: `cmake --build build && ctest --test-dir build --output-on-failure`

Expected: failures because undo/redo keys are unmapped or not implemented.

- [x] **Step 2: Implement undo stack and editor recording**

Implement an undo stack of edit records containing deleted text, inserted text, start position, cursor before, cursor after, and group id. Add editor helpers that mutate through `document_replace_range`, record inverse data, clear redo after new edits, and group contiguous printable insertion.

- [x] **Step 3: Verify Task 2**

Run: `cmake --build build && ctest --test-dir build --output-on-failure`

Expected: all tests pass.

---

### Task 3: Search And Replace

**Files:**
- Modify: `src/core/search.h`
- Modify: `src/core/search.c`
- Modify: `src/core/editor.h`
- Modify: `src/core/editor.c`
- Test: `tests/test_main.c`

**Interfaces:**
- Consumes: document range primitives and undo-recorded replace operations.
- Produces: literal case-sensitive search state, next/previous navigation, replace current, and replace all as one undo group.

- [x] **Step 1: Add failing search/replace tests**

Add tests that:

- Set query `one`, find next from the top, and assert cursor moves to the first match.
- Find previous with wrapping and assert cursor moves to the last match.
- Replace current `one` with `two`, then undo and assert original text returns.
- Replace all `one` with `two`, assert all matches change, undo once, and assert all original matches return.
- In read mode, replace current and replace all do not mutate.

Run: `cmake --build build && ctest --test-dir build --output-on-failure`

Expected: compile failure or behavior failure for missing search APIs.

- [x] **Step 2: Implement search state and editor APIs**

Add `search_set_query`, `search_clear`, `search_find_next`, `search_find_previous`, `search_count_matches`, and `search_visible_match_at`-style helpers. Add editor-level helpers used by tests and key handling: `editor_search_set_query`, `editor_find_next`, `editor_find_previous`, `editor_replace_current`, and `editor_replace_all_confirmed`.

- [x] **Step 3: Verify Task 3**

Run: `cmake --build build && ctest --test-dir build --output-on-failure`

Expected: all tests pass.

---

### Task 4: Selection And Clipboard

**Files:**
- Modify: `src/core/editor.h`
- Modify: `src/core/editor.c`
- Test: `tests/test_main.c`

**Interfaces:**
- Consumes: document range primitives and undo-recorded replace operations.
- Produces: selection state, Shift+arrow selection handling, selected text delete/replace, internal cut/copy/paste, and cut line.

- [x] **Step 1: Add failing selection/clipboard tests**

Add tests that:

- Select `bc` from `abc`, press Delete, and assert the line is `a`.
- Select `b`, type `X`, and assert the line is `aXc`.
- Select text, move viewport, and assert the selection is still active.
- Copy selection in read mode and assert clipboard contents are set.
- Cut selection in write mode and assert clipboard plus document mutation.
- Paste clipboard over a selection and assert replacement.
- Press `Ctrl+K` with no selection and assert current line is cut.

Run: `cmake --build build && ctest --test-dir build --output-on-failure`

Expected: failures because selection and clipboard are missing.

- [x] **Step 2: Implement selection and clipboard behavior**

Add `Selection` to `Editor`, normalize selection ranges when needed, clear selection on plain movement, extend selection on Shift+arrow movement, allow copy in read mode, and block mutating clipboard commands in read mode. Use undo-recorded document replacement for cut, paste, selection delete, typing over selection, and cut line.

- [x] **Step 3: Verify Task 4**

Run: `cmake --build build && ctest --test-dir build --output-on-failure`

Expected: all tests pass.

---

### Task 5: Platform Key Mappings

**Files:**
- Modify: `src/platform/platform.h`
- Modify: `src/platform/terminal_posix.c`
- Modify: `src/platform/terminal_win32.c`
- Test: `tests/test_main.c`

**Interfaces:**
- Produces: constants and mappings for `Ctrl+Z`, `Ctrl+Y`, `Ctrl+F`, `Ctrl+G`, `Ctrl+P`, `Ctrl+R`, `Ctrl+A`, `Ctrl+X`, `Ctrl+C`, `Ctrl+V`, `Ctrl+K`, and Shift+arrows.
- Consumed by: editor key handling.

- [x] **Step 1: Add failing key mapping tests**

Add tests that assert POSIX parser maps `"[1;2A"`, `"[1;2B"`, `"[1;2C"`, and `"[1;2D"` to Shift+arrow constants.

Run: `cmake --build build && ctest --test-dir build --output-on-failure`

Expected: failures because Shift+arrow constants and mappings are missing.

- [x] **Step 2: Implement POSIX and Windows key mappings**

Add constants in `platform.h`. Map POSIX Shift+arrow escape sequences. Map Windows Shift+arrow by checking `SHIFT_PRESSED` in `KEY_EVENT_RECORD.dwControlKeyState`.

- [x] **Step 3: Verify Task 5**

Run: `cmake --build build && ctest --test-dir build --output-on-failure`

Expected: all tests pass.

---

### Task 6: Renderer Highlights

**Files:**
- Modify: `src/renderer/ansi_renderer.c`
- Test: `tests/test_main.c`

**Interfaces:**
- Consumes: selection and search state from `Editor`.
- Produces: visible ANSI highlighting for search matches and selected ranges.

- [x] **Step 1: Add failing renderer-facing tests**

Add tests against pure helper functions or exported editor/search state that prove:

- A visible search match at a row/column is detected.
- A selected visible byte at a row/column is detected.
- Selection wins when selection overlaps search.

Run: `cmake --build build && ctest --test-dir build --output-on-failure`

Expected: failures for missing helper behavior.

- [x] **Step 2: Implement highlight decisions and ANSI paint**

Render line bytes one at a time when either search or selection highlighting is present on the visible row. Use reverse video for selection and a separate highlight style for search. Reset ANSI attributes at style boundaries and at end of line.

- [x] **Step 3: Verify Task 6**

Run: `cmake --build build && ctest --test-dir build --output-on-failure`

Expected: all tests pass.

---

### Task 7: Documentation And Closeout

**Files:**
- Modify: `README.md`
- Modify: `docs/plans/MVP-002.md`
- Review: `docs/superpowers/specs/2026-07-09-mvp-002-undo-search-selection-design.md`

**Interfaces:**
- Consumes: completed behavior from Tasks 1-6.
- Produces: user-facing docs and milestone closeout notes.

- [x] **Step 1: Update docs**

Update README with MVP-002 keybindings and update `docs/plans/MVP-002.md` to mark the milestone as implemented locally with any known limitations.

- [x] **Step 2: Full verification**

Run:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
git diff --check
```

Expected: all commands exit 0.

- [x] **Step 3: Final review**

Inspect:

```sh
git diff --stat
git diff -- src tests docs README.md CMakeLists.txt
```

Expected: changes are limited to MVP-002 implementation, tests, docs, and the removal of `fix.txt`.

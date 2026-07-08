# MVP-001 Core Editor Slice Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build the testable core of MVP-001: read/write mode, missing-file semantics, newline/delete editing, LF/CRLF preservation, atomic save, and active-line horizontal scroll reset.

**Architecture:** Keep document storage and file I/O in `src/core/document.*`, editor mode and viewport rules in `src/core/editor.*`, and terminal rendering as a consumer of editor state. This slice avoids broad terminal automation and proves behavior through core tests first.

**Tech Stack:** C17, CMake 3.20+, standard C library, existing in-repo test executable using `assert`.

## Global Constraints

- Existing files open in read mode.
- Missing command-line paths open as new unsaved documents in write mode.
- Editing is blocked in read mode.
- Saves must be atomic from MVP-001.
- LF and CRLF line endings are preserved on save.
- Text handling should preserve UTF-8 bytes where practical.
- macOS, Linux, and Windows CMake configure/build/test paths must remain viable.
- No ncurses/PDCurses, GUI, plugins, Tree-sitter, LSP, or network functionality.

---

### Task 1: Document Line Operations And Line Endings

**Files:**
- Modify: `src/core/document.h`
- Modify: `src/core/document.c`
- Modify: `tests/test_main.c`

**Interfaces:**
- Produces: `typedef enum DocumentLineEnding { DOCUMENT_LINE_ENDING_LF, DOCUMENT_LINE_ENDING_CRLF } DocumentLineEnding`
- Produces: `bool document_load_path(Document *doc, const char *path, bool *created_new)`
- Produces: `bool document_save(Document *doc)`
- Produces: `bool document_save_as(Document *doc, const char *path)`
- Produces: `void document_insert_char(Document *doc, size_t row, size_t col, char ch)`
- Produces: `void document_delete_char_before(Document *doc, size_t row, size_t col, size_t *out_row, size_t *out_col)`
- Produces: `void document_delete_char_at(Document *doc, size_t row, size_t col)`
- Produces: `void document_insert_newline(Document *doc, size_t row, size_t col, size_t *out_row, size_t *out_col)`

- [ ] **Step 1: Write failing document tests**

Add tests in `tests/test_main.c` for:

```c
static void test_insert_newline_splits_line(void) {
    Document doc;
    document_init(&doc);
    document_insert_char(&doc, 0, 0, 'A');
    document_insert_char(&doc, 0, 1, 'B');
    document_insert_char(&doc, 0, 2, 'C');
    size_t row = 0;
    size_t col = 0;
    document_insert_newline(&doc, 0, 1, &row, &col);
    assert(doc.line_count == 2);
    assert(strcmp(doc.lines[0].data, "A") == 0);
    assert(strcmp(doc.lines[1].data, "BC") == 0);
    assert(row == 1);
    assert(col == 0);
    document_destroy(&doc);
}
```

Also add tests that backspace at column zero joins with the previous line, delete at end joins with the next line, CRLF input saves as CRLF, and a missing path reports `created_new == true`.

- [ ] **Step 2: Run test to verify it fails**

Run: `cmake --build build && ctest --test-dir build --output-on-failure`

Expected: FAIL or build failure because the new signatures and behavior are not implemented yet.

- [ ] **Step 3: Implement minimal document behavior**

Update `Document` with a `line_ending` field and add helper functions that:

```c
static const char *document_line_separator(const Document *doc);
static bool document_set_path(Document *doc, const char *path);
static bool replace_file_with_temp(const char *temp_path, const char *path);
```

Implement newline splitting, previous-line joins on backspace, next-line joins on delete, CRLF detection while loading, missing-path creation in memory, and temp-file-then-rename save.

- [ ] **Step 4: Run test to verify it passes**

Run: `cmake --build build && ctest --test-dir build --output-on-failure`

Expected: all tests pass.

---

### Task 2: Editor Read/Write Mode And Core Editing Actions

**Files:**
- Modify: `src/core/editor.h`
- Modify: `src/core/editor.c`
- Modify: `CMakeLists.txt`
- Modify: `tests/test_main.c`

**Interfaces:**
- Produces: `typedef enum EditorMode { EDITOR_MODE_READ, EDITOR_MODE_WRITE } EditorMode`
- Produces: `void editor_enter_write_mode(Editor *editor)`
- Produces: `void editor_handle_key(Editor *editor, int key)`
- Consumes: document editing interfaces from Task 1.

- [ ] **Step 1: Write failing editor mode tests**

Add tests in `tests/test_main.c` for:

```c
static void test_read_mode_blocks_printable_insert(void) {
    Editor editor;
    editor_init(&editor);
    editor.mode = EDITOR_MODE_READ;
    editor_handle_key(&editor, 'A');
    assert(editor.document.lines[0].length == 0);
    assert(editor.document.dirty == false);
    editor_destroy(&editor);
}
```

Also add tests that `w` enters write mode, printable input edits only in write mode, backspace/delete/newline update cursor positions, and `Ctrl+Q` sets `should_quit`.

- [ ] **Step 2: Run test to verify it fails**

Run: `cmake -S . -B build && cmake --build build && ctest --test-dir build --output-on-failure`

Expected: FAIL or build failure because editor mode and public key handling are not implemented yet.

- [ ] **Step 3: Implement minimal editor behavior**

Move current private key handling into public `editor_handle_key(Editor *editor, int key)`. Keep platform calls outside this function. Add `mode` to `Editor`, initialize to READ, enter WRITE on `w` / `W`, block editing commands in READ, and route editing commands through the document APIs. Add `src/core/editor.c`, `src/renderer/ansi_renderer.c`, `src/platform/platform.c`, and platform terminal sources to `tedit_tests` so editor tests link on all configured platforms.

- [ ] **Step 4: Run test to verify it passes**

Run: `cmake -S . -B build && cmake --build build && ctest --test-dir build --output-on-failure`

Expected: all tests pass.

---

### Task 3: Open Semantics And Status Rendering Data

**Files:**
- Modify: `src/core/editor.c`
- Modify: `src/renderer/ansi_renderer.c`
- Modify: `tests/test_main.c`

**Interfaces:**
- Consumes: `document_load_path(Document *doc, const char *path, bool *created_new)`
- Produces: existing files open with `EDITOR_MODE_READ`; missing files open with `EDITOR_MODE_WRITE`.

- [ ] **Step 1: Write failing open-mode tests**

Add tests in `tests/test_main.c` that create a temporary existing file, open it with `editor_open`, and assert `editor.mode == EDITOR_MODE_READ`. Add a second test that opens a missing path and asserts `editor.mode == EDITOR_MODE_WRITE`, `document.path` is set, and `document.dirty == false`.

- [ ] **Step 2: Run test to verify it fails**

Run: `cmake --build build && ctest --test-dir build --output-on-failure`

Expected: FAIL because existing/missing open semantics are not yet wired through the editor.

- [ ] **Step 3: Implement open semantics and status data**

Update `editor_open` to call `document_load_path`. Set READ mode for existing files and WRITE mode for missing files. Update the renderer status line to include `READ` or `WRITE`, `UTF-8`, and `LF` or `CRLF`.

- [ ] **Step 4: Run test to verify it passes**

Run: `cmake --build build && ctest --test-dir build --output-on-failure`

Expected: all tests pass.

---

### Task 4: Active-Line Horizontal Scroll Reset

**Files:**
- Modify: `src/core/editor.h`
- Modify: `src/core/editor.c`
- Modify: `tests/test_main.c`

**Interfaces:**
- Produces: `void editor_update_viewport(Editor *editor)` for testable viewport rules.

- [ ] **Step 1: Write failing viewport reset test**

Add a test in `tests/test_main.c` that creates two lines, sets a small screen width, moves the cursor far right on line 0, calls `editor_update_viewport`, verifies `active_line_left_col > 0`, then moves to line 1, calls `editor_update_viewport`, and verifies `active_line_left_col == 0`.

- [ ] **Step 2: Run test to verify it fails**

Run: `cmake --build build && ctest --test-dir build --output-on-failure`

Expected: FAIL because viewport update is private and does not reset when switching lines.

- [ ] **Step 3: Implement reset**

Track the last active row in `Viewport`, expose `editor_update_viewport`, and reset `active_line_left_col` to zero whenever `cursor.row` changes.

- [ ] **Step 4: Run test to verify it passes**

Run: `cmake --build build && ctest --test-dir build --output-on-failure`

Expected: all tests pass.

---

### Task 5: Verification And Commit

**Files:**
- Review: `docs/plans/MVP-001.md`
- Review: `git diff`

- [ ] **Step 1: Run full local verification**

Run:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
git diff --check
```

Expected: all commands exit 0.

- [ ] **Step 2: Inspect final diff**

Run: `git diff --stat`

Expected: changes are limited to the planned core source, tests, renderer status line, and this plan.

- [ ] **Step 3: Commit**

Run:

```sh
git add CMakeLists.txt docs/superpowers/plans/2026-07-09-mvp-001-core-editor-slice.md src/core/document.h src/core/document.c src/core/editor.h src/core/editor.c src/platform/platform.h src/platform/terminal_posix.c src/renderer/ansi_renderer.c tests/test_main.c
git commit -m "Start MVP-001 core editor behavior"
```

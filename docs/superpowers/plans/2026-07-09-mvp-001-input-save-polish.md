# MVP-001 Input And Save Polish Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Finish key MVP-001 editor-loop polish: `Ctrl+S` saves missing-path documents, Tab inserts a literal tab in write mode, and macOS Option-as-Meta `Option+Q` maps to immediate quit.

**Architecture:** Keep save and edit semantics in `src/core/editor.c`, with tests calling `editor_handle_key` directly. Keep terminal-specific Option/Meta input parsing in `src/platform/terminal_posix.c` through the existing escape-sequence mapper.

**Tech Stack:** C17, CMake 3.20+, standard C library, POSIX terminal escape parsing, existing in-repo `assert` tests.

## Global Constraints

- Existing files open in read mode.
- Missing command-line paths open as new unsaved documents in write mode.
- Editing is blocked in read mode.
- Tabs insert literal tab characters by default.
- `Ctrl+S` saves.
- `Ctrl+Q` exits immediately.
- macOS `Option+Q` exits immediately where the terminal sends a distinguishable input sequence.
- macOS, Linux, and Windows CMake configure/build/test paths must remain viable.
- No ncurses/PDCurses, GUI, plugins, Tree-sitter, LSP, or network functionality.

---

### Task 1: Editor Ctrl+S Saves Missing-Path Documents

**Files:**
- Modify: `tests/test_main.c`
- Modify: `src/core/editor.c`

**Interfaces:**
- Consumes: `int editor_open(Editor *editor, const char *path)`
- Consumes: `void editor_handle_key(Editor *editor, int key)`
- Produces: `Ctrl+S` writes a missing-path WRITE-mode document to disk and clears dirty state.

- [ ] **Step 1: Write the failing test**

Add this test in `tests/test_main.c`:

```c
static void test_ctrl_s_saves_missing_path_document(void) {
    const char *path = "tedit_test_ctrl_s_missing.tmp";
    char buffer[32];
    size_t length = 0;
    remove(path);

    Editor editor;
    editor_init(&editor);
    assert(editor_open(&editor, path) == 1);
    assert(editor.mode == EDITOR_MODE_WRITE);
    editor_handle_key(&editor, 'O');
    editor_handle_key(&editor, 'K');
    assert(editor.document.dirty == true);

    editor_handle_key(&editor, TEDIT_KEY_CTRL_S);

    assert(editor.document.dirty == false);
    read_file_bytes(path, buffer, sizeof(buffer), &length);
    assert(length == 2);
    assert(memcmp(buffer, "OK", 2) == 0);
    editor_destroy(&editor);
    remove(path);
}
```

- [ ] **Step 2: Run test to verify it fails**

Run: `cmake --build build && ctest --test-dir build --output-on-failure`

Expected: If existing behavior already passes, adjust the test to assert a missing observed requirement such as status text `Saved`; otherwise it fails because save behavior is incomplete.

Observed during execution: existing behavior already satisfied this path, including `Saved` status. The test was retained as regression coverage and no production change was needed for this task.

- [ ] **Step 3: Implement minimal save polish**

Update `editor_handle_key` if needed so `TEDIT_KEY_CTRL_S` saves through `document_save`, sets status to `Saved`, and leaves the document not dirty after successful save.

- [ ] **Step 4: Run test to verify it passes**

Run: `cmake --build build && ctest --test-dir build --output-on-failure`

Expected: all tests pass.

---

### Task 2: Literal Tab Insertion In Write Mode

**Files:**
- Modify: `tests/test_main.c`
- Modify: `src/core/editor.c`

**Interfaces:**
- Consumes: `void editor_handle_key(Editor *editor, int key)`
- Produces: pressing `'\t'` in WRITE mode inserts one literal tab byte and advances the cursor by one model column.

- [ ] **Step 1: Write the failing test**

Add this test in `tests/test_main.c`:

```c
static void test_tab_inserts_literal_tab_in_write_mode(void) {
    Editor editor;
    editor_init(&editor);
    editor_enter_write_mode(&editor);

    editor_handle_key(&editor, '\t');

    assert(editor.document.lines[0].length == 1);
    assert(editor.document.lines[0].data[0] == '\t');
    assert(editor.cursor.col == 1);
    editor_destroy(&editor);
}
```

- [ ] **Step 2: Run test to verify it fails**

Run: `cmake --build build && ctest --test-dir build --output-on-failure`

Expected: FAIL because tab is not currently treated as an editing key.

- [ ] **Step 3: Implement minimal tab insertion**

Update `editor_is_editing_key` and `editor_handle_key` so `'\t'` is blocked in READ mode and inserts `'\t'` in WRITE mode.

- [ ] **Step 4: Run test to verify it passes**

Run: `cmake --build build && ctest --test-dir build --output-on-failure`

Expected: all tests pass.

---

### Task 3: Option+Q / Meta-Q Quit Mapping

**Files:**
- Modify: `tests/test_main.c`
- Modify: `src/platform/terminal_posix.c`

**Interfaces:**
- Consumes: `int platform_key_from_escape_sequence(const char *sequence)`
- Produces: escape sequences `"q"` and `"Q"` map to `TEDIT_KEY_CTRL_Q`.

- [ ] **Step 1: Write the failing mapper test**

Extend `test_escape_sequence_navigation_mapping` in `tests/test_main.c`:

```c
assert(platform_key_from_escape_sequence("q") == TEDIT_KEY_CTRL_Q);
assert(platform_key_from_escape_sequence("Q") == TEDIT_KEY_CTRL_Q);
```

- [ ] **Step 2: Run test to verify it fails**

Run: `cmake --build build && ctest --test-dir build --output-on-failure`

Expected: FAIL because the mapper currently returns escape for `"q"` and `"Q"`.

- [ ] **Step 3: Implement minimal mapper behavior**

Update `platform_key_from_escape_sequence` in `src/platform/terminal_posix.c` to return `TEDIT_KEY_CTRL_Q` for `"q"` and `"Q"`. Update the POSIX read loop to stop after a lowercase ASCII letter too, so `ESC q` is mapped without waiting for timeout.

- [ ] **Step 4: Run test to verify it passes**

Run: `cmake --build build && ctest --test-dir build --output-on-failure`

Expected: all tests pass.

---

### Task 4: Verification And Commit

**Files:**
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

Expected: changes are limited to this plan, editor save/input behavior, POSIX Meta-Q mapping, and tests.

- [ ] **Step 3: Commit**

Run:

```sh
git add docs/superpowers/plans/2026-07-09-mvp-001-input-save-polish.md src/core/editor.c src/platform/terminal_posix.c tests/test_main.c
git commit -m "Polish MVP-001 save and input handling"
```

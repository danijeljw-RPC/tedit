# MVP-001 Save And Navigation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Complete the next MVP-001 slice by preserving existing file permissions during atomic save and adding core navigation behavior for Home, End, Page Up, Page Down, Ctrl+Home, and Ctrl+End.

**Architecture:** Keep permission preservation inside `src/core/document.c` because save policy belongs with file I/O. Keep navigation semantics inside `src/core/editor.c` so platform-specific code only translates terminal input into portable key constants.

**Tech Stack:** C17, CMake 3.20+, standard C library, POSIX `stat`/`chmod` where available, existing in-repo `assert` tests.

## Global Constraints

- Existing files open in read mode.
- Missing command-line paths open as new unsaved documents in write mode.
- Editing is blocked in read mode.
- Saves must be atomic from MVP-001.
- Save existing files atomically and preserve permissions where practical.
- LF and CRLF line endings are preserved on save.
- macOS, Linux, and Windows CMake configure/build/test paths must remain viable.
- No ncurses/PDCurses, GUI, plugins, Tree-sitter, LSP, or network functionality.

---

### Task 1: Preserve Existing File Permissions On Save

**Files:**
- Modify: `src/core/document.c`
- Modify: `tests/test_main.c`

**Interfaces:**
- Consumes: `bool document_save(Document *doc)`
- Produces: POSIX existing-file saves preserve `st_mode & 0777` after temp-file replacement.

- [ ] **Step 1: Write the failing test**

Add a POSIX-only test in `tests/test_main.c`:

```c
#ifndef _WIN32
static void test_save_preserves_existing_file_permissions(void) {
    const char *path = "tedit_test_permissions.tmp";
    write_file_bytes(path, "alpha\n", 6);
    assert(chmod(path, 0600) == 0);

    Document doc;
    bool created_new = true;
    struct stat st;
    document_init(&doc);
    assert(document_load_path(&doc, path, &created_new));
    assert(created_new == false);
    document_insert_char(&doc, 0, 5, '!');
    assert(document_save(&doc));
    assert(stat(path, &st) == 0);
    assert((st.st_mode & 0777) == 0600);
    document_destroy(&doc);
    remove(path);
}
#endif
```

Call it from `main` under `#ifndef _WIN32`.

- [ ] **Step 2: Run test to verify it fails**

Run: `cmake --build build && ctest --test-dir build --output-on-failure`

Expected: FAIL on POSIX because the atomic temp file replacement uses the process default permissions rather than the existing file mode.

- [ ] **Step 3: Implement minimal permission preservation**

In `src/core/document.c`, include POSIX headers under `#ifndef _WIN32`:

```c
#include <sys/stat.h>
```

Before opening the temp file, read the target mode with `stat(path, &st)`. After `fclose(file)` and before `rename`, call `chmod(temp_path, st.st_mode & 0777)` when the target existed.

- [ ] **Step 4: Run test to verify it passes**

Run: `cmake --build build && ctest --test-dir build --output-on-failure`

Expected: all tests pass.

---

### Task 2: Core Navigation Keys

**Files:**
- Modify: `src/platform/platform.h`
- Modify: `src/core/editor.c`
- Modify: `tests/test_main.c`

**Interfaces:**
- Produces: `TEDIT_KEY_HOME`
- Produces: `TEDIT_KEY_END`
- Produces: `TEDIT_KEY_PAGE_UP`
- Produces: `TEDIT_KEY_PAGE_DOWN`
- Produces: `TEDIT_KEY_CTRL_HOME`
- Produces: `TEDIT_KEY_CTRL_END`
- Consumes: `void editor_handle_key(Editor *editor, int key)`

- [ ] **Step 1: Write failing navigation tests**

Add tests in `tests/test_main.c` for:

```c
static void test_home_end_move_within_line(void) {
    Editor editor;
    editor_init(&editor);
    editor_enter_write_mode(&editor);
    editor_handle_key(&editor, 'A');
    editor_handle_key(&editor, 'B');
    editor_handle_key(&editor, 'C');

    editor_handle_key(&editor, TEDIT_KEY_HOME);
    assert(editor.cursor.col == 0);
    editor_handle_key(&editor, TEDIT_KEY_END);
    assert(editor.cursor.col == 3);
    editor_destroy(&editor);
}
```

Also add a multi-line test proving Page Down moves down by visible rows, Page Up moves back up, Ctrl+End moves to the last line end, and Ctrl+Home moves to row 0 col 0.

- [ ] **Step 2: Run test to verify it fails**

Run: `cmake --build build && ctest --test-dir build --output-on-failure`

Expected: FAIL or build failure because the new key constants are not defined and editor behavior is missing.

- [ ] **Step 3: Implement minimal editor navigation**

Add key constants in `src/platform/platform.h` after `TEDIT_KEY_DELETE`. Add cases in `move_cursor`:

```c
case TEDIT_KEY_HOME: editor->cursor.col = 0; break;
case TEDIT_KEY_END: editor->cursor.col = current_line_length; break;
case TEDIT_KEY_PAGE_UP: move up by usable rows; break;
case TEDIT_KEY_PAGE_DOWN: move down by usable rows; break;
case TEDIT_KEY_CTRL_HOME: row = 0, col = 0; break;
case TEDIT_KEY_CTRL_END: row = last, col = last_line_length; break;
```

Keep `clamp_cursor(editor)` at the end of movement.

- [ ] **Step 4: Run test to verify it passes**

Run: `cmake --build build && ctest --test-dir build --output-on-failure`

Expected: all tests pass.

---

### Task 3: POSIX Input Mapping For Navigation Keys

**Files:**
- Modify: `src/platform/terminal_posix.c`

**Interfaces:**
- Consumes: key constants from Task 2.
- Produces: POSIX escape sequence mapping for Home, End, Page Up, Page Down, Ctrl+Home, and Ctrl+End.

- [ ] **Step 1: Write the parser-oriented failing test by extracting a pure mapper**

Add this declaration to `src/platform/platform.h`:

```c
int platform_key_from_escape_sequence(const char *sequence);
```

Add tests in `tests/test_main.c`:

```c
static void test_escape_sequence_navigation_mapping(void) {
    assert(platform_key_from_escape_sequence("[H") == TEDIT_KEY_HOME);
    assert(platform_key_from_escape_sequence("[F") == TEDIT_KEY_END);
    assert(platform_key_from_escape_sequence("[5~") == TEDIT_KEY_PAGE_UP);
    assert(platform_key_from_escape_sequence("[6~") == TEDIT_KEY_PAGE_DOWN);
    assert(platform_key_from_escape_sequence("[1;5H") == TEDIT_KEY_CTRL_HOME);
    assert(platform_key_from_escape_sequence("[1;5F") == TEDIT_KEY_CTRL_END);
}
```

- [ ] **Step 2: Run test to verify it fails**

Run: `cmake --build build && ctest --test-dir build --output-on-failure`

Expected: FAIL or build failure because the mapper does not exist.

- [ ] **Step 3: Implement mapper and use it in POSIX input**

In `src/platform/terminal_posix.c`, implement `platform_key_from_escape_sequence` under the POSIX block. It should return known key constants or `'\x1b'` for unknown sequences. Update `platform_read_key` to read up to seven bytes after escape into a null-terminated buffer and call the mapper.

Add a Windows-compatible fallback implementation in `src/platform/terminal_win32.c`:

```c
int platform_key_from_escape_sequence(const char *sequence) {
    (void)sequence;
    return '\x1b';
}
```

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

Expected: changes are limited to this plan, file save permissions, navigation key constants, POSIX key mapping, editor movement behavior, and tests.

- [ ] **Step 3: Commit**

Run:

```sh
git add docs/superpowers/plans/2026-07-09-mvp-001-save-and-navigation.md src/core/document.c src/core/editor.c src/platform/platform.h src/platform/terminal_posix.c src/platform/terminal_win32.c tests/test_main.c
git commit -m "Expand MVP-001 save and navigation behavior"
```

# MVP-003 File UI And Syntax Foundation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [x]`) syntax for tracking.

**Goal:** Complete MVP-003 by adding menu models, open/save-as prompts, a file browser model, view/editing settings, native syntax tokens, built-in highlighters, and a Tier 1 nano syntax importer.

**Architecture:** Keep the editor model-driven. Core owns editor settings, prompt state, file operations, and syntax token models; UI owns menu and file browser models; renderer only paints model state; platform only maps terminal input/output.

**Tech Stack:** C17, CMake 3.20+, existing assert-based C tests, POSIX directory APIs, Windows file enumeration APIs, immediate ANSI renderer.

## Global Constraints

- Preserve MVP-001 and MVP-002 behavior, including read/write mode rules, undo/search/selection, line endings, and byte-preserving text behavior.
- Keep macOS, Linux, and Windows CMake builds viable.
- Keep platform-specific code under `src/platform`.
- Keep editor state and behavior under `src/core`.
- Keep menu and file browser models under `src/ui`.
- Keep rendering under `src/renderer`.
- Do not add dependencies, ncurses, GUI, plugins, full regex, Tree-sitter, LSP, or network functionality.
- Menu, prompt, file browser, syntax, and settings behavior must be testable without a terminal.
- Selection highlight priority remains higher than search, and search remains higher than syntax.

---

## File Structure

- Create `src/ui/menu.h` and `src/ui/menu.c`: static menu bar model, active menu state, Alt shortcut lookup, and navigation.
- Create `src/ui/file_browser.h` and `src/ui/file_browser.c`: portable directory listing model with parent entry, directories-first sorting, file entries, selection, and activation results.
- Create `src/core/syntax.h` and `src/core/syntax.c`: native syntax token types, token arrays, built-in C-like highlighter, and Tier 1 nano importer.
- Modify `src/core/editor.h` and `src/core/editor.c`: add settings, open/save-as prompt modes, menu state, file browser state, syntax set, tab insertion behavior, and command helpers.
- Modify `src/platform/platform.h`, `src/platform/terminal_posix.c`, and `src/platform/terminal_win32.c`: add Alt key constants and mappings for menu shortcuts.
- Modify `src/renderer/ansi_renderer.c`: paint menu bar, active menu, optional line numbers, whitespace markers, and syntax styles while preserving existing search/selection highlights.
- Modify `CMakeLists.txt`: compile new UI/core files into `tedit` and `tedit_tests`.
- Modify `tests/test_main.c`: add TDD tests for every MVP-003 behavior.
- Modify `README.md` and `docs/plans/MVP-003.md`: document completed behavior, keys, closeout notes, and known limitations.

---

### Task 1: Menu Model And Alt Shortcuts

**Files:**
- Create: `src/ui/menu.h`
- Create: `src/ui/menu.c`
- Modify: `src/platform/platform.h`
- Modify: `src/platform/terminal_posix.c`
- Modify: `src/platform/terminal_win32.c`
- Modify: `src/core/editor.h`
- Modify: `src/core/editor.c`
- Modify: `CMakeLists.txt`
- Test: `tests/test_main.c`

**Interfaces:**
- Produces: `MenuBar`, `MenuId`, `MenuCommandId`, `menu_bar_init`, `menu_bar_open_shortcut`, `menu_bar_active_menu`, `menu_bar_active_item`, `menu_bar_handle_key`, `editor_menu_is_open`, `editor_active_menu`.
- Consumed by: renderer and later command routing.

- [x] **Step 1: Write failing tests**

Add tests asserting six menus exist, Alt+F/E/S/V/T/H open the expected menu, Escape closes a menu, and Down changes the active item.

Run: `cmake --build build && ctest --test-dir build --output-on-failure`

Expected: compile failure for missing menu types and Alt key constants.

- [x] **Step 2: Implement minimal menu model and key mapping**

Add menu structures, static labels/items, Alt constants, POSIX `ESC f/e/s/v/t/h` mapping, Windows Alt letter mapping, and editor key handling that opens/closes/navigates menus.

- [x] **Step 3: Verify Task 1**

Run: `cmake --build build && ctest --test-dir build --output-on-failure`

Expected: all tests pass.

---

### Task 2: Open Prompt, Save-As Prompt, And File Browser Model

**Files:**
- Create: `src/ui/file_browser.h`
- Create: `src/ui/file_browser.c`
- Modify: `src/core/editor.h`
- Modify: `src/core/editor.c`
- Modify: `CMakeLists.txt`
- Test: `tests/test_main.c`

**Interfaces:**
- Produces: `EDITOR_PROMPT_OPEN`, `EDITOR_PROMPT_SAVE_AS`, `editor_start_open_prompt`, `editor_start_save_as_prompt`, `editor_open_path`, `editor_save_as_path`, `FileBrowser`, `file_browser_open`, `file_browser_move_selection`, `file_browser_activate_selected`.
- Consumed by: menu command routing and renderer.

- [x] **Step 1: Write failing tests**

Add tests for opening an existing file through the prompt, opening a missing path through the prompt, saving an unnamed document through save-as prompt, and file browser ordering with parent entry, directories before files, and file activation.

Run: `cmake --build build && ctest --test-dir build --output-on-failure`

Expected: compile failure for missing prompt modes and file browser APIs.

- [x] **Step 2: Implement prompts and file browser**

Extend prompt completion for open/save-as, reset editor cursor/viewport/search/selection on open, call `document_save_as` for save-as, and add portable directory listing with deterministic ordering.

- [x] **Step 3: Verify Task 2**

Run: `cmake --build build && ctest --test-dir build --output-on-failure`

Expected: all tests pass.

---

### Task 3: View Settings And Tab Insertion Modes

**Files:**
- Modify: `src/core/editor.h`
- Modify: `src/core/editor.c`
- Modify: `src/renderer/ansi_renderer.c`
- Test: `tests/test_main.c`

**Interfaces:**
- Produces: `EditorSettings`, `EditorTabMode`, `editor_toggle_line_numbers`, `editor_toggle_whitespace`, `editor_set_tab_mode`.
- Consumed by: renderer and tab key handling.

- [x] **Step 1: Write failing tests**

Add tests for default settings, toggling line numbers, toggling whitespace, literal tab insertion, 2-space tab insertion, 4-space tab insertion, and read-mode blocking of tab insertion.

Run: `cmake --build build && ctest --test-dir build --output-on-failure`

Expected: compile failure for missing settings APIs.

- [x] **Step 2: Implement settings and tab behavior**

Store settings in `Editor`, route menu commands to toggles, and replace `\t` insertion with configured bytes while preserving undo behavior and read-mode gates.

- [x] **Step 3: Verify Task 3**

Run: `cmake --build build && ctest --test-dir build --output-on-failure`

Expected: all tests pass.

---

### Task 4: Native Syntax Token Model And Built-In Highlighters

**Files:**
- Create: `src/core/syntax.h`
- Create: `src/core/syntax.c`
- Modify: `src/core/editor.h`
- Modify: `src/core/editor.c`
- Modify: `src/renderer/ansi_renderer.c`
- Modify: `CMakeLists.txt`
- Test: `tests/test_main.c`

**Interfaces:**
- Produces: `SyntaxTokenType`, `SyntaxToken`, `SyntaxTokenLine`, `SyntaxDefinition`, `syntax_definition_init_builtin_c`, `syntax_highlight_line`, `syntax_token_line_destroy`, `editor_syntax_token_at`.
- Consumed by: renderer.

- [x] **Step 1: Write failing tests**

Add tests showing the built-in C-like highlighter marks `int` as keyword, `// comment` as comment, string literals as string, numbers as number, and exposes tokens without renderer calls.

Run: `cmake --build build && ctest --test-dir build --output-on-failure`

Expected: compile failure for missing syntax APIs.

- [x] **Step 2: Implement token model and built-in highlighter**

Add token arrays with explicit ownership, a deterministic byte-scanning C-like highlighter, and renderer syntax style lookup that does not affect search/selection priority.

- [x] **Step 3: Verify Task 4**

Run: `cmake --build build && ctest --test-dir build --output-on-failure`

Expected: all tests pass.

---

### Task 5: Nano Syntax Importer Tier 1

**Files:**
- Modify: `src/core/syntax.h`
- Modify: `src/core/syntax.c`
- Test: `tests/test_main.c`

**Interfaces:**
- Produces: `syntax_definition_load_nano_file`, `syntax_definition_load_nano_text`, extension matching, simple color/icolor rules, and native token type mapping.
- Consumed by: editor syntax selection later.

- [x] **Step 1: Write failing tests**

Add tests loading nano text containing `syntax "c" "\\.c$"`, `color green "int"`, and `icolor brightred "todo"`, then assert the syntax name, extension match, case-sensitive keyword token, and case-insensitive todo token.

Run: `cmake --build build && ctest --test-dir build --output-on-failure`

Expected: compile or behavior failure for missing nano importer.

- [x] **Step 2: Implement Tier 1 importer**

Parse quoted strings from `syntax`, `color`, and `icolor` lines. Support simple literal word/punctuation matching and extension suffix patterns like `\\.c$`. Map common nano colors to native token types.

- [x] **Step 3: Verify Task 5**

Run: `cmake --build build && ctest --test-dir build --output-on-failure`

Expected: all tests pass.

---

### Task 6: Renderer Integration And Documentation Closeout

**Files:**
- Modify: `src/renderer/ansi_renderer.c`
- Modify: `README.md`
- Modify: `docs/plans/MVP-003.md`
- Review: `docs/superpowers/specs/2026-07-09-mvp-003-file-ui-syntax-foundation-design.md`

**Interfaces:**
- Consumes: menu, settings, browser, and syntax model state from earlier tasks.

- [x] **Step 1: Write/adjust renderer-facing tests**

Add or adjust tests for highlight priority and settings-driven renderer decisions where practical through pure model helpers.

Run: `cmake --build build && ctest --test-dir build --output-on-failure`

Expected: all tests pass after prior tasks, or fail only for missing renderer helper behavior added in this task.

- [x] **Step 2: Implement renderer paint and docs**

Render menu bar on the first row, active menu below it when open, editor text below menu chrome, line numbers when enabled, visible whitespace when enabled, and fixed syntax colors. Update README and MVP-003 plan with closeout notes and limitations.

- [x] **Step 3: Final verification and commit**

Run:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
git diff --check
git status --short
git log -1 --oneline
```

Expected: configure/build/tests/diff-check pass, working tree contains only MVP-003 changes before commit, then a local milestone commit is created.

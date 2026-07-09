# MVP-003 File UI And Syntax Foundation Design

## Goal

MVP-003 adds discoverable menu state, file prompts/browser state, view/settings toggles, and the first renderer-independent syntax highlighting model. The work must preserve MVP-001 and MVP-002 editing behavior and keep C17/CMake portability across macOS, Linux, and Windows.

## Current Context

The editor has a model-driven core with `Document`, `Editor`, cursor, viewport, undo, search, selection, prompt, clipboard, and read/write mode state. Rendering is immediate ANSI output from `Editor` state. The `src/ui` layer is currently placeholder-only, which gives MVP-003 room to add menu and file browser models without moving document mutations into renderer or platform code.

## Approach Options

Recommended: add small native models and wire them through the existing editor loop. `src/ui` owns menu and file browser data structures; `src/core` owns editor settings, open/save prompt modes, and syntax token generation; renderer reads those models and paints them. This keeps the milestone testable without a terminal and avoids broad rewrites.

Alternative 1: fold menu and browser state directly into `Editor`. That is faster, but it makes `Editor` larger and blurs UI-model ownership.

Alternative 2: build a more complete command palette/dialog framework. That would be cleaner long term, but it drifts into later UI work and is too large for MVP-003.

## Menu Model

Add a static menu bar model with File, Edit, Search, View, Tools, and Help menus. Each menu has an Alt shortcut and a fixed list of command identifiers. Opening a menu sets active menu state; Escape closes it; Left/Right moves between menus; Up/Down moves inside a menu. The initial command execution surface stays minimal and maps only MVP-003-relevant commands to existing editor actions and prompts.

Default Alt shortcuts:

- `Alt+F`: File
- `Alt+E`: Edit
- `Alt+S`: Search
- `Alt+V`: View
- `Alt+T`: Tools
- `Alt+H`: Help

## File Prompts And Browser

Add open and save-as prompt modes in core. The prompt buffer remains byte-oriented ASCII path input for MVP-003. Open prompt accepts a path, calls `editor_open`, resets cursor/viewport/selection/search state, and preserves existing read/write rules: existing files open read-only, missing files open write mode. Save-as prompt accepts a path, calls `document_save_as`, updates the document path, clears dirty on success, and works when the app started without a command-line path.

Add a basic file browser model in `src/ui` with current directory, parent entry, directories-first ordering, files second, selected index, and close/open behavior. The browser lists entries through portable C APIs: POSIX `opendir/readdir/stat` and Windows `FindFirstFileA/FindNextFileA`. Tests can create a temporary directory and assert sorting/selection/open decisions without raw terminal input.

## View And Editing Settings

Add editor settings for:

- line numbers: off by default, toggled by the View menu
- whitespace display: off by default, toggled by the View menu
- tab insertion: literal tab by default, or 2 spaces, or 4 spaces

Tab key insertion must use the setting in write mode and remain blocked in read mode. Whitespace display is renderer-only: tabs and spaces in the document stay byte-preserving, while rendering may paint visible placeholders. Line numbers are renderer-only and affect left gutter width without changing document coordinates.

## Syntax Model

Add a native syntax model under `src/core`. It exposes token types independent of ANSI styling:

- normal
- keyword
- type
- string
- character
- comment
- number
- function
- preprocessor
- heading
- link
- error
- todo

Built-in highlighters are simple and deterministic. MVP-003 should include at least C-like highlighting for comments, strings, character literals, preprocessor lines, numbers, and keywords, plus a plain text fallback. Token ranges are byte offsets per line. Renderer maps token types to ANSI styles but does not own tokenization.

## Nano Importer Tier 1

Add a nano syntax importer that parses a useful Tier 1 subset:

- `syntax "name" "extension-regex"...`
- `color <fg> "regex"...`
- `icolor <fg> "regex"...`

For MVP-003, simple regex rules are intentionally constrained to literal words, escaped punctuation, and extension-like patterns. The importer maps nano color names to native token types through a fixed table, stores case-sensitivity, and exposes match rules to the native syntax engine. Multiline `start=`/`end=`, comments metadata, headers, and nano compatibility quirks stay out of scope.

## Rendering

Renderer reads model state:

- menu bar and active menu from UI model
- line number and whitespace settings from `Editor`
- syntax tokens from core
- existing search and selection highlights from core

Selection remains highest priority, then search, then syntax. Rendering must not mutate editor state.

## Tests

Add assert-based tests for:

- Alt shortcut constants and menu opening/navigation
- File/Edit/Search/View/Tools/Help menu presence
- open prompt opens existing and missing files without command-line path
- save-as prompt saves unnamed documents
- file browser parent/directories/files ordering and open decisions
- line number and whitespace settings toggles
- tab insertion modes for literal tab, 2 spaces, and 4 spaces
- syntax token model independent of renderer
- built-in C-like highlighter token ranges
- nano importer Tier 1 syntax/color/icolor parsing
- read mode still blocks mutating edits including tab insertion
- MVP-001/MVP-002 regression tests remain green

## Out Of Scope

- mouse menus
- recent files
- file deletion/rename/copy
- destructive file operations
- multiline nano regions
- Tree-sitter
- LSP
- plugins
- ncurses or GUI
- syntax themes beyond fixed ANSI mapping
- full regex engine

## Acceptance Criteria

- Menus can be opened with Alt shortcuts.
- Open and save-as work when the app starts without a command-line path.
- Native highlighting model is renderer-independent.
- Nano syntax parser can load simple Tier 1 syntax rules.
- Line numbers, whitespace display, and tab insertion settings are model state with tests.
- The implementation configures, builds, and tests locally with the documented CMake commands.
- The code remains portable for macOS, Linux, and Windows CMake builds.

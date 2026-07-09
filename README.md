# TEdit Starter

TEdit is a planned cross-platform terminal text editor written in C.

The first milestone is deliberately small:

- open one text file
- render it in a raw terminal
- default to read mode when opening existing files
- enter write mode with `w` / `W`
- move the cursor
- vertically scroll
- implement per-line horizontal scrolling for the active line only
- edit text
- save atomically

MVP-002 adds safer editing actions:

- undo/redo grouped edits
- find prompt and next/previous navigation
- replace current and replace all with confirmation
- visible search and selection highlights
- Shift+arrow selection
- internal cut/copy/paste clipboard
- cut line

MVP-003 adds file UI and syntax foundations:

- model-driven File, Edit, Search, View, Tools, and Help menus
- Alt shortcuts for opening menus
- open and save-as prompts that work without a command-line path
- basic file browser model with parent entry, directories first, and files second
- line-number and whitespace-display settings
- tab insertion setting for literal tab, 2 spaces, or 4 spaces
- renderer-independent native syntax token model
- simple built-in C-like highlighter
- Tier 1 nano syntax importer for `syntax`, `color`, `icolor`, simple filename suffix patterns, and literal match rules

This repository is a starter skeleton for Claude Code CLI, Codex, or a human developer to begin implementation.
The architecture must stay cross-platform as features are added: macOS is the primary local development target, but each milestone must keep Linux and Windows compiling through CMake and must not defer Windows support into a rewrite.

## Build

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

CI runs the same configure, build, and test flow on Ubuntu, macOS, and Windows.

## Run

```sh
./build/tedit path/to/file.txt
```

On Windows, the binary will be under the CMake build output directory.
If `path/to/file.txt` does not exist, the editor starts a new unsaved document in write mode and creates the file when saved.

## Default Keys

- `w` / `W`: enter WRITE mode from READ mode
- `Ctrl+S`: save
- `Ctrl+Q`: quit immediately
- `Ctrl+Z`: undo
- `Ctrl+Y`: redo
- `Ctrl+F`: find prompt
- `Ctrl+G`: find next
- `Ctrl+P`: find previous
- `Ctrl+R`: replace current match
- `Ctrl+A`: replace all matches with confirmation
- `Shift+Arrow`: extend selection
- `Ctrl+C`: copy selection to the internal clipboard
- `Ctrl+X`: cut selection to the internal clipboard
- `Ctrl+V`: paste from the internal clipboard
- `Ctrl+K`: cut selection, or cut the current line when no selection is active
- `Alt+F`: open File menu
- `Alt+E`: open Edit menu
- `Alt+S`: open Search menu
- `Alt+V`: open View menu
- `Alt+T`: open Tools menu
- `Alt+H`: open Help menu

Within an open menu, use arrow keys to navigate, Enter to run the selected command, and Escape to close the menu.

## MVP-003 Limitations

- File browser support is a portable model with tested ordering and activation behavior; destructive file operations, recent files, rename, copy, and delete are not included.
- Nano syntax support is Tier 1 only. Multiline `start=`/`end=`, headers, comments metadata, and nano compatibility quirks are intentionally deferred.
- Syntax matching is simple byte-oriented matching, not a full regex engine.
- Whitespace display is visual-only and does not modify document bytes.

## Documentation

Start here:

- `TUI-Editor-Complete-Feature-Specification.md`
- `docs/README.md`
- `docs/project-brief.md`
- `docs/architecture.md`
- `docs/plans/MVP-001.md`
- `docs/claude-code-instructions.md`
- `docs/codex-instructions.md`

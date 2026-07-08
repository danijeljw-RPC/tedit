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

## Documentation

Start here:

- `TUI-Editor-Complete-Feature-Specification.md`
- `docs/README.md`
- `docs/project-brief.md`
- `docs/architecture.md`
- `docs/plans/MVP-001.md`
- `docs/claude-code-instructions.md`
- `docs/codex-instructions.md`

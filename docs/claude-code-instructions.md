# Claude Code Instructions

## Operating mode

Work milestone by milestone. Do not implement future milestone features while closing the current milestone.

## First task

Start with `docs/plans/MVP-001.md`.

## Constraints

- C17 only.
- CMake build must remain working.
- macOS, Linux, and Windows builds must remain viable at each milestone.
- Keep core logic terminal-independent.
- Prefer small, reviewable changes.
- Do not add ncurses/PDCurses.
- Do not introduce GPL source or bundled GPL syntax files.
- Do not implement plugins, Tree-sitter, or LSP in MVP-001.
- Do not leave Windows support as a placeholder for accepted milestone behavior.

## Required closeout

Before reporting completion:

1. run configure/build/tests
2. confirm CI coverage or equivalent build commands for macOS, Linux, and Windows
3. run the editor manually if possible
4. report files changed
5. report tests run
6. list known gaps honestly

## Design priority

The per-line horizontal scroll behaviour is a core product differentiator. Preserve it when refactoring viewport/rendering code.

Existing files open in READ mode. Editing is enabled only after `w` / `W`, while missing command-line paths start as WRITE-mode unsaved documents.

# Codex Instructions

## Goal

Implement the C terminal editor from the documented milestones.
Keep macOS, Linux, and Windows viable throughout implementation; do not defer Windows support into a later rewrite.

## Start point

Read:

- `docs/project-brief.md`
- `docs/architecture.md`
- `docs/engineering-rules.md`
- `docs/plans/MVP-001.md`

## Build commands

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

CI must run equivalent configure, build, and test steps on Ubuntu, macOS, and Windows before a milestone is considered closed.

## Implementation rules

- Keep code portable.
- Keep platform-specific code under `src/platform`.
- Keep editor state under `src/core`.
- Keep rendering under `src/renderer`.
- Keep read/write mode behavior in core state, not in platform input code.
- Preserve UTF-8 bytes and LF/CRLF line endings where practical.
- Implement Windows behavior alongside POSIX behavior for accepted milestone features.
- Add tests for core behaviour before expanding UI features.
- Update docs if behaviour changes.

## Do not do yet

- no dependencies unless explicitly justified
- no ncurses
- no GUI
- no plugin system
- no network functionality

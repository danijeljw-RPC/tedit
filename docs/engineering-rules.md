# Engineering Rules

## General

- Keep scope narrow per milestone.
- Do not add unrelated features while implementing a milestone.
- Prefer small C files with clear ownership.
- Keep core logic testable without a terminal.
- Avoid global mutable state unless unavoidable for signal/terminal cleanup.
- Keep macOS, Linux, and Windows compiling through CMake at the end of every milestone.
- Do not implement a feature in a way that requires a later Windows rewrite.
- Keep platform behavior behind explicit platform interfaces.

## C style

- Use C17.
- Use explicit ownership rules for allocated memory.
- Every `init` must have a matching `destroy`.
- Functions returning success/failure should use `bool` where practical.
- Avoid clever macros.
- Avoid hidden allocations in hot render loops where practical.

## Terminal safety

- Raw mode must always be restored on normal exit.
- Add signal/atexit cleanup before serious editing work.
- Do not leave the terminal broken after a crash where avoidable.
- Completed terminal/input features must have POSIX and Windows implementations, even when manual testing happens first on macOS.

## Testing

- Core document operations must have unit tests.
- Cursor and viewport behaviour must have tests.
- Per-line horizontal scrolling must have tests.
- Read mode and write mode transitions must have tests.
- Atomic save and line-ending preservation must have tests.
- Renderer should eventually be tested against expected screen buffers.

## No uncontrolled decisions

Create or update an ADR before changing major choices such as:

- replacing raw terminal handling with curses
- adding a scripting/plugin language
- adopting Tree-sitter as required dependency
- changing build system away from CMake
- changing the editor from single-process terminal app to client/server
- deferring Windows feature parity for an accepted milestone

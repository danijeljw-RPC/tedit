# ADR-0002 — Use Raw Terminal Rendering

## Status

Accepted

## Decision

Use raw terminal mode and ANSI rendering instead of ncurses/PDCurses for the initial architecture.

## Context

The editor requires custom rendering behaviour, especially per-line horizontal scrolling for the active line only.

## Consequences

- More implementation work in the terminal/input layer.
- More rendering control.
- Fewer external dependencies.
- Windows support requires explicit console handling as each accepted feature is implemented.

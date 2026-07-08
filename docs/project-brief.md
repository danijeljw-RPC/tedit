# Project Brief

Build a cross-platform terminal text editor in C.

The editor should feel approachable like nano, but with stronger navigation, modern selection behaviour, menus, search, highlighting, and a distinctive horizontal scrolling model.

## Product goal

Create a lightweight TUI editor for macOS, Linux, and Windows that is simple enough for casual terminal editing but structured enough to grow into a serious tool.

macOS is the initial daily-driver development platform, but Linux and Windows are first-class targets. Each feature must be designed so it can compile and run through the same CMake project on all three operating systems as it is implemented, without requiring a later Windows rewrite.

## Differentiator

Most terminal editors use a global horizontal offset. When the user moves through a long line, the whole editor view shifts horizontally.

This project should instead use per-line horizontal scrolling:

- the active line may scroll horizontally
- inactive visible lines remain at column 0
- the user keeps surrounding document context while inspecting a long line

Existing files open in read mode by default. The user explicitly enters write mode with `w` or `W` before editing. Missing file paths create a new unsaved document and enter write mode immediately.

## Initial non-goals

- no GUI
- no plugin runtime in MVP
- no LSP in MVP
- no Tree-sitter in MVP
- no full Unicode display-width perfection in MVP
- no terminal multiplexer assumptions
- no ncurses/PDCurses dependency unless a future ADR changes this

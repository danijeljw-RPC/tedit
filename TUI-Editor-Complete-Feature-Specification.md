# TUI Editor - Agreed Feature Specification

This document contains **only** the features discussed.
It describes the complete product direction, not a single MVP. Milestone
documents under `docs/plans/` define when each feature is implemented.

# Vision

A cross-platform terminal text editor written in C for macOS, Linux and
Windows using a raw ANSI renderer with platform-specific terminal
support.

# Core Design

-   C language
-   Cross-platform
-   CMake build on macOS, Linux, and Windows at the end of every stage
-   Raw ANSI rendering
-   POSIX `termios` on macOS/Linux
-   Windows console implementation developed alongside each feature
-   Editor logic separated from rendering
-   Screen buffer with dirty-region redraws
-   No flicker
-   Terminal resize handling
-   No stage may rely on a future Windows rewrite

# Interface

``` text
┌──────────────────────────────────────────────────────────────────────────────┐
│ File  Edit  Search  View  Tools  Help                         README.md   * │
├──────────────────────────────────────────────────────────────────────────────┤
│ Tabs: README.md | notes.txt | main.c                                        │
├───────────────┬──────────────────────────────────────────────────────────────┤
│ File Browser  │  1  # Heading                                               │
│ > docs        │  2                                                          │
│   src         │  3  Current line scrolls horizontally only.                 │
│   tests       │  4                                                          │
│               │  5  Other lines remain fixed.                               │
│               │                                                             │
├───────────────┴──────────────────────────────────────────────────────────────┤
│ Ln 5 Col 18 UTF-8 LF READ/WRITE Modified                                    │
└──────────────────────────────────────────────────────────────────────────────┘
```

# Modes

## Read Mode

-   Default when opening files.
-   Navigation only.
-   Existing files always open in read mode.

## Write Mode

-   Press `W` or `w` to enable editing.
-   New missing-path files start in write mode.
-   Status bar indicates WRITE.
-   Save permitted after modifications.
-   Modified indicator shown.

# File Operations

-   New
-   Open
-   Recent files
-   Save
-   Save As
-   Reload from disk
-   Rename
-   Delete file (confirmation)
-   Close file
-   Multiple open files
-   Tabs
-   Modified indicator
-   Atomic save
-   Recovery files
-   Detect external modification
-   Missing command-line paths create a new unsaved document and materialize the file on save
-   Existing file permissions are preserved where practical

# Navigation

-   Arrow keys
-   Home
-   End
-   Page Up
-   Page Down
-   Ctrl+Home
-   Ctrl+End
-   Go to line
-   Go to column
-   Breadcrumb/path
-   File browser pane

# Editing

-   Insert
-   Delete
-   Backspace
-   New line
-   Duplicate line
-   Delete line
-   Cut line
-   Join lines
-   Move line up
-   Move line down
-   Auto-indent
-   Trim trailing whitespace
-   Insert/Overwrite mode
-   Existing files are not editable until write mode is enabled
-   `Ctrl+Q` and macOS `Option+Q` quit immediately

# Selection

-   Shift+Arrow keys
-   Shift+Home
-   Shift+End
-   Ctrl+Shift+Arrow keys
-   Select word
-   Select line
-   Select paragraph
-   Select all
-   Copy
-   Cut
-   Paste
-   Internal clipboard
-   OS clipboard

# Search

-   Find
-   Incremental search
-   Find next
-   Find previous
-   Highlight all matches
-   Case-sensitive option
-   Whole-word option
-   Regular expression option
-   Replace current
-   Replace all

# Display

-   Line numbers
-   Current line highlight
-   Matching brackets
-   Show tabs/spaces
-   Configurable tab width
-   Tab insertion choices: literal tab, 2 spaces, or 4 spaces
-   Optional word wrap
-   Per-line horizontal scrolling:
    -   only the active line scrolls horizontally
    -   all other visible lines remain fixed
    -   active-line horizontal scroll resets when switching away from the line

# Syntax Highlighting

-   Built-in highlighting
-   Nano syntax file compatibility/import
-   Internal syntax model
-   Initial languages:
    -   Plain text
    -   C
    -   C#
    -   Markdown
    -   JSON
    -   YAML
    -   XML/HTML

# Spelling

-   Spell checking
-   Highlight spelling mistakes
-   Ignore once
-   Ignore always
-   Add to dictionary
-   Multiple dictionaries

# Menus

## File

-   New
-   Open
-   Recent
-   Save
-   Save As
-   Close
-   Exit

## Edit

-   Undo
-   Redo
-   Cut
-   Copy
-   Paste
-   Delete line
-   Duplicate line

## Search

-   Find
-   Replace
-   Go To

## View

-   Toggle line numbers
-   Toggle whitespace
-   Toggle wrap

## Tools

-   Spelling
-   Syntax
-   Settings

## Help

-   Shortcuts
-   About

# Undo / Redo

-   Unlimited (configurable)
-   Group typing into logical operations
-   Restore cursor and selection

# Encoding

-   UTF-8
-   LF
-   CRLF
-   Preserve loaded text bytes where practical
-   Preserve original line endings on save unless the user changes settings
-   Advanced grapheme and display-width handling can improve over time without corrupting data

# Staging

-   MVP-001 delivers the single-file editor loop, read/write modes, atomic
    save, line-ending preservation, per-line horizontal scrolling, and
    cross-platform CMake build/test parity.
-   MVP-002 adds undo, search, replace, and selection while preserving read/write
    mode rules.
-   MVP-003 adds menus, file UI, display toggles, tab insertion settings, and
    syntax highlighting foundations.
-   Later stages add tabs, multiple open files, recovery files, spelling,
    settings, themes, OS clipboard integration, and advanced syntax support.

# Configuration

-   Theme
-   Tab width
-   Autosave
-   Key bindings
-   Default encoding
-   Default line endings
-   Dictionary
-   Syntax

# Architecture

``` text
Application
│
├── Core
│   ├── Document
│   ├── Buffer
│   ├── Cursor
│   ├── Selection
│   ├── Search
│   ├── Undo
│   └── File I/O
│
├── UI
│   ├── Menu Bar
│   ├── Editor View
│   ├── Status Bar
│   ├── Dialogs
│   └── File Browser
│
├── Renderer
│   ├── Screen Buffer
│   ├── ANSI Renderer
│   └── Dirty Region Tracker
│
└── Platform
    ├── POSIX (termios)
    └── Windows Console
```

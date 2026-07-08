# Syntax Highlighting

## Direction

The editor should have its own internal syntax highlighting model, with an importer/parser for nano syntax files.

Do not make the runtime model a direct clone of nano internals.

Nano compatibility is an accepted product direction. It does not need to ship in MVP-001, but architecture and token models should avoid choices that block it.

## Native model

Suggested internal token types:

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

## Nano compatibility

Support nano syntax files in tiers.

### Tier 1

- `syntax`
- `color`
- `icolor`
- extension/file matching
- simple regex rules

### Tier 2

- `start=` / `end=` multiline regions
- comments
- header matching

### Tier 3

- nano edge cases and compatibility quirks

## Licensing note

Parsing user-provided nano syntax files is different from copying nano source files or bundling GPL syntax files. Confirm licensing before bundling any third-party definitions.

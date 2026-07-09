# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- MVP-003 file UI and syntax foundation: menu bar model, File/Edit/Search/View/Tools/Help menus, open and save-as prompts, file browser model, line-number and whitespace settings, tab insertion modes, syntax token model, built-in highlighter, and Tier 1 nano syntax importer.
- Centered About and prompt dialogs for terminal UI workflows.
- `-v` and `--version` command-line flags for printing version and copyright information.
- Centralized application metadata in `src/core/app_info.h`.

### Changed

- Improved terminal menu rendering with a white menu bar, bold mnemonic letters, bordered vertical dropdowns, and direct menu item shortcuts such as `Esc`, `s`, `f` for Search -> Find.
- `Ctrl+Q` and menu Quit now clear the terminal before exit.

### Fixed

- Standalone `Esc` followed by a menu letter now opens the expected menu.
- About dialog padding now uses display width so UTF-8 copyright text keeps the box rectangular.

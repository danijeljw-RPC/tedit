# Cross-Platform Support

## Targets

- macOS
- Linux
- Windows

## Strategy

Use CMake and isolate platform-specific terminal code.

macOS is the initial hands-on development environment, but Linux and Windows are first-class targets. Every milestone must configure, build, and run tests on all three operating systems. A feature is not complete if its Windows path is only a placeholder that would require a future rewrite.

## POSIX

Use:

- `termios` for raw mode
- `ioctl(TIOCGWINSZ)` for terminal size
- ANSI escape sequences for rendering

## Windows

Use Windows Console APIs as features are implemented:

- enable virtual terminal processing
- configure console input mode
- read key events or parse VT sequences consistently
- query console size

The Windows implementation may be smaller than the POSIX implementation while a feature is being developed, but milestone completion requires equivalent behavior for accepted milestone features.

## Clipboard later

- macOS: `pbcopy` / `pbpaste` or native APIs
- Linux: X11/Wayland tools vary
- Windows: Win32 clipboard API

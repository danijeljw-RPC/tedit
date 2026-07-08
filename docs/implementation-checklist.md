# Implementation Checklist

Before closing a milestone:

- CMake configure succeeds on macOS, Linux, and Windows
- build succeeds on macOS, Linux, and Windows
- tests pass
- terminal state restores after exit
- README or relevant docs updated
- no unrelated files changed
- known limitations documented
- no accepted milestone feature is left as a Windows-only placeholder
- save, encoding, and line-ending behavior are verified when touched

For each feature:

- define state first
- define input/action mapping
- update document/editor state
- update viewport logic
- render from state
- implement platform adapter behavior for POSIX and Windows
- add tests for core logic

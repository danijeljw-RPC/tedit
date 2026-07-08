# ADR-0001 — Use C and CMake

## Status

Accepted

## Decision

Use C17 for the editor and CMake 3.20 or newer for cross-platform builds.

## Context

The project is a terminal editor intended for macOS, Linux, and Windows. C gives direct control over terminal I/O, memory, and platform boundaries. CMake provides a practical cross-platform build layer.

## Consequences

- Manual memory management is required.
- Platform abstraction must be explicit.
- CI can build on multiple operating systems.
- Each milestone must keep macOS, Linux, and Windows configure/build/test paths working.

# ADR-0003 — Model-Driven Editor

## Status

Accepted

## Decision

Represent editor state explicitly and render from that state.

## Context

Printing directly from editing actions creates coupling and makes testing difficult.

## Consequences

- Core editing can be tested without a terminal.
- Rendering can be replaced or optimized later.
- Features like selections, menus, search highlights, and syntax tokens have clear state ownership.

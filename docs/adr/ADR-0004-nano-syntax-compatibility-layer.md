# ADR-0004 — Nano Syntax Compatibility Layer

## Status

Accepted

## Decision

Support nano syntax files through an importer/parser that maps nano rules into the editor's own internal highlighting model.

## Context

Nano syntax compatibility is useful for adoption, but the editor should not be internally constrained by nano's model.

## Consequences

- Existing nano syntax files can be used where licensing permits.
- Native syntax highlighting can evolve independently.
- Compatibility can be implemented in tiers.

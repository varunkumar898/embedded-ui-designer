# ADR-001: Unified Project Model & Serialization Schema

## Status
Accepted

## Context
Embedded UI applications require cross-platform design serialization that maps visual layouts, API endpoints, hardware pin bindings, and code generation parameters into a deterministic, version-controlled format.

## Decision
We adopted a unified JSON-based Project Model schema comprising:
1. `ProjectMetadata`: UUID, semantic version, target MCU framework, creation/modified timestamps.
2. `TypeRegistry`: Strongly-typed primitive and composite structure definitions.
3. `ApiSchema`: Endpoint definitions, parameter bindings (path/query/header/body), status code mappings.
4. `DeviceOperations`: Pin-level hardware mappings.

## Consequences
- Deterministic round-trip serialization between desktop designer UI, web schema editor, and code generators.
- Backwards compatibility guarantees via semantic schema migration hooks.

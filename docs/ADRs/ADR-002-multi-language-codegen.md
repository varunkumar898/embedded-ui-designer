# ADR-002: Multi-Language Code Generation Architecture

## Status
Accepted

## Context
Embedded systems teams write firmware across different languages depending on performance, memory safety, and platform constraints:
- C99 for ultra-low power bare-metal Cortex-M0 microcontrollers.
- C++17/20 for object-oriented firmware on ESP32 / Cortex-M4/M7.
- Rust for memory-safe embedded systems.

## Decision
Implement a single-source code generator engine (`CodegenEngine`) that ingests the `ProjectModel` and emits idiomatic, compile-ready code for C99, C++17/20, and Rust with HAL integration and serialization hooks.

## Consequences
- Eliminates manual sync errors between frontend API contracts and firmware endpoint handlers.
- 100% test coverage across all three language targets.

# Architecture Decision Record (ADR) Template

## Title: [Short title of solved problem and chosen solution]

* **Status**: [Proposed | Accepted | Superseded | Deprecated]
* **Deciders**: [List everyone involved in the decision]
* **Date**: [YYYY-MM-DD]
* **Technical Story**: [Link to issue or requirement]

---

## Context and Problem Statement

[Describe the context and problem statement in 2-3 sentences. What are we trying to achieve? What forces influence our decision?]

---

## Decision Drivers

* Performance on memory-constrained microcontrollers (e.g., STM32, ESP32, RP2040)
* Type safety and zero-allocation serialization
* Multi-language compatibility (C, C++, Rust)
* Ease of integration with modern Web frontends
* Security compliance (OWASP Top 10, TLS 1.3)

---

## Considered Options

1. Option 1: [Description]
2. Option 2: [Description]
3. Option 3: [Description]

---

## Decision Outcome

Chosen option: **Option 1**, because [justification. e.g., only Option 1 meets zero-allocation constraints while offering C++17 constexpr and Rust memory safety].

### Positive Consequences

* [e.g., Deterministic memory footprint under 64KB RAM]
* [e.g., Sub-millisecond latency for GPIO/ADC reads]
* [e.g., Compile-time schema validation across C/C++/Rust]

### Negative Consequences

* [e.g., Code generator complexity increased]
* [e.g., Requires cross-compiler toolchains in CI]

---

## Pros and Cons of the Options

### Option 1: [Chosen Option]

* Good, because [argument a]
* Good, because [argument b]
* Bad, because [argument c]

### Option 2: [Alternative Option]

* Good, because [argument a]
* Bad, because [argument b]

---

## Links

* [Project Model Specification](file:///home/cherry/embedded-ui-designer/docs/WEEK_3_4_DETAILED_PROMPTS.md)
* [Code Generation Architecture](file:///home/cherry/embedded-ui-designer/docs/WEEK_5_6_DETAILED_PROMPTS.md)
* [Security Hardening Guide](file:///home/cherry/embedded-ui-designer/docs/WEEK_9_10_DETAILED_PROMPTS.md)

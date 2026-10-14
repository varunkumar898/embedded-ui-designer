# EmbedWeb Unsupported Properties & Components Policy

## 1. Overview
EmbedWeb is an ultra-lightweight, bounded-memory native C UI runtime for microcontrollers (e.g., ESP32-S3, STM32, RP2350). Because resource-constrained MCUs lack multi-gigabyte memory and GPU pipelines, EmbedWeb intentionally does not support full web browsers, JavaScript engines, SVG paths, or video codecs.

This policy defines the deterministic behavior and graceful degradation guarantees when encountering unsupported properties or components from the Universal IR.

---

## 2. Policy Principles

1. **Deterministic Degradation**: The runtime must never crash, assert, or exhibit undefined behavior when encountering an unsupported property or component.
2. **Predictable Layout Preservation**: Unsupported components preserve their bounding geometry (`req_w`, `req_h`, `flex` weights) so surrounding layout elements are not broken or shifted unexpectedly.
3. **Zero Dynamic Allocation**: Handling unsupported properties must not trigger heap allocations, exceptions, or string parsing in the rendering loop.
4. **Developer Visibility**: In simulation or debug mode, fallback representations (e.g., placeholder outlines) clearly identify unsupported features.

---

## 3. Unsupported Components Policy

| Component Type | IR Representation | EmbedWeb Behavior | Visual Fallback |
| :--- | :--- | :--- | :--- |
| **Video** | `VideoComponent` / `video` | Ignored | Bounding box with outline and placeholder label `[Video]` (debug) or transparent spacer (release). |
| **Canvas / WebGL** | `CanvasComponent` / `<canvas>` | Ignored | Bounding box with outline and placeholder label `[Canvas]` (debug). |
| **Complex SVG Path** | `VectorComponent` (complex Bezier) | Simplified | Bounded rectangular approximation or discarded. |
| **Rich HTML / Iframe** | `IframeComponent` / `<iframe>` | Ignored | Transparent container preserving dimensions. |
| **Audio** | `<audio>` | Ignored | Zero layout impact (0x0). |

---

## 4. Unsupported Properties Policy

| CSS / IR Property | Support in Stage 4.1 | Degradation Behavior |
| :--- | :--- | :--- |
| **Multi-stop Gradients** | Unsupported | Fallback to solid color of first color stop (`gradient.stops[0].color`). |
| **Box Shadows** (`boxShadow`) | Unsupported | Ignored, or rendered as 1px solid border (`border_width = 1`) if high contrast requested. |
| **Backdrop Filter / Blur** | Unsupported | Ignored; background rendered with standard solid color. |
| **Transforms** (`rotate`, `skew`, `matrix`) | Unsupported | Identity transform (0 rotation, 0 skew). |
| **Animations / Transitions** | Unsupported in 4.1 | Rendered statically at initial state (keyframe 0%). |
| **Variable / Non-8x8 Fonts** | Unsupported in 4.1/4.2 | Rendered using embedded 8x8 bitmap ASCII font. |
| **ARGB8888 Framebuffer / Assets** | **Explicitly Deferred** | RGB565 is the native MCU frame format. 32bpp ARGB8888 is deferred because it doubles RAM usage (breaching <64 KB SRAM budget on 480x320 displays) and halves SPI display bus throughput. Transparency is supported via RGB565 Chroma-Keying and integer alpha blending (`ew_color_blend565`). |
| **Progress Bars** (`ProgressBar`) | **Supported in Stage 4.2** | Rendered with background track, filled indicator proportional to `progress_value`, and border. |
| **Image Assets** (`Image`) | **Supported in Stage 4.2** | Raw RGB565 and RGB565 Chroma-Key bitmaps supported with clipping and opacity blending. |

---

## 5. Telemetry and Diagnostic Callback

EmbedWeb exposes an optional diagnostic callback:
```c
typedef void (*ew_unsupported_property_cb)(const char* component_id, const char* property_name, void* user_data);
```
During project conversion or runtime hydration, this callback notifies the developer or automated test suite whenever an unsupported property is encountered.

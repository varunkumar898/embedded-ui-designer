# Embedded Device UI Designer – Project Brief & Technical Specification

## Project Vision
A cross-platform desktop design tool (Windows, Linux, macOS; x86 & ARM) that enables embedded systems engineers to design user interfaces visually (like Figma) and automatically generate production-ready code for microcontrollers (Qt for MCUs and µGFX).

## Core Pillars
1. **Design Canvas**: Visual drag-and-drop workspace matching target microcontroller display dimensions (e.g., 320x240, 480x320, 800x480).
2. **Component Library**: Embedded-focused UI elements (Buttons, Labels, Text Inputs, Rectangles, Sliders, Progress Bars, Images).
3. **Properties & Layers**: Real-time property inspection, alignment tools, layer ordering, and visual tree hierarchy.
4. **Code Generation Pipeline**: Exporting full, buildable Qt for MCUs (QML + C++) and µGFX (C) project suites.
5. **Project Serialization**: Human-readable `.euiproj` JSON format for version control and interchange.

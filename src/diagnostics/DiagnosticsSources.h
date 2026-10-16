#pragma once

namespace Diagnostics {

class DiagnosticsService;

namespace DiagnosticsSources {

/**
 * Registers the built-in diagnostic sources shared by the app:
 *  - "Resources": ResourceAnalysis structured warnings (Task 1 analyzer).
 *  - "Asset":     Image components that reference missing files.
 *  - "Binding":   Existing data-binding validation (GeneratorContext::validate).
 *  - "Layout":    Components outside the screen bounds (DiagnosticsEngine rule).
 *  - "BoardDisplay": Task 5 framebuffer fit + LCD-controller contradictions.
 *  - "Backend":   Task 6 component/backend compatibility for the selected
 *                 target backend (unsupported, partial or unknown support).
 */
void registerBuiltinSources(DiagnosticsService& service);

} // namespace DiagnosticsSources
} // namespace Diagnostics

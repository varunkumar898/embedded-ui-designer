#pragma once

#include <cstddef>

/// @deprecated Use ResourceEstimator::calculateFramebufferBytes instead.
/// This class is kept for backward compatibility only.
class ResourceAnalyzer {
public:
    ResourceAnalyzer() = default;

    /// Calculates exact framebuffer size accounting for dimensions, bpp, buffer count, and DMA alignment
    /// @deprecated Use ResourceEstimator::calculateFramebufferBytes instead.
    static size_t calculateFramebufferBytes(int width, int height, int bpp, int bufferCount, size_t dmaAlignment = 16);
};
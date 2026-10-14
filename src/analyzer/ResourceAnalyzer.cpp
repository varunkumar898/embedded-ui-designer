#include "ResourceAnalyzer.h"
#include "ResourceEstimator.h"

size_t ResourceAnalyzer::calculateFramebufferBytes(int width, int height, int bpp, int bufferCount, size_t dmaAlignment) {
    return ResourceEstimator::calculateFramebufferBytes(width, height, bpp, bufferCount, dmaAlignment);
}
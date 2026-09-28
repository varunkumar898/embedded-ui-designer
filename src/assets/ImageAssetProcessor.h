#pragma once

#include <QString>
#include <QImage>
#include <QByteArray>
#include <cstdint>

/**
 * @brief Target pixel format for embedded graphics displays.
 */
enum class ImageFormat {
    RGB565,      ///< 16-bit high color (5-bit red, 6-bit green, 5-bit blue)
    Monochrome,  ///< 1-bit per pixel thresholded bitmap (8 pixels packed per byte)
    RGB888       ///< 24-bit true color (8-bit red, 8-bit green, 8-bit blue)
};

/**
 * @brief C++ Asset Compilation subsystem for EmbeddedUIDesigner.
 * 
 * Converts standard QImage assets into flash-ready C byte arrays with correct
 * bitwise packing for bare-metal microcontroller framebuffers and µGFX image widgets.
 */
class ImageAssetProcessor {
public:
    ImageAssetProcessor() = delete;

    /**
     * @brief Converts a QImage to a formatted C-style const uint8_t array string.
     * @param image The input image to convert.
     * @param format The target pixel format (RGB565 or Monochrome).
     * @param arrayName The C identifier for the generated array.
     * @return Formatted C source string (e.g., "const uint8_t image_data[] = { ... };").
     */
    static QString generateCArray(const QImage& image, ImageFormat format, const QString& arrayName = "image_data");

    /**
     * @brief Converts a QImage directly to raw packed binary bytes.
     * @param image The input image.
     * @param format The target pixel format.
     * @return Packed QByteArray.
     */
    static QByteArray processToBytes(const QImage& image, ImageFormat format);
};

#ifndef BOARD_FACTORY_H
#define BOARD_FACTORY_H

#include "BoardHAL.h"
#include <memory>
#include <QString>
#include <QStringList>

/**
 * @class BoardFactory
 * @brief Factory for creating board-specific HAL instances.
 * 
 * Responsibilities:
 * 1. Auto-detect connected board via OpenOCD
 * 2. Instantiate the correct HAL
 * 3. Fallback to user selection if auto-detect fails
 */
class BoardFactory {
public:
    /**
     * @brief Auto-detect board type by querying OpenOCD.
     * Uses "targets" command to identify chip.
     * 
     * @param openocdOutput Output from "targets" OpenOCD command
     * @return Detected board name ("STM32F030R8", "ESP32", etc.)
     */
    static QString detectBoardFromOpenOCD(const QString& openocdOutput);

    /**
     * @brief Create HAL instance for a given board name.
     * 
     * @param boardName e.g., "STM32F030R8", "ESP32", "ESP32-S3"
     * @return Unique pointer to BoardHAL implementation
     */
    static std::unique_ptr<BoardHAL> createHAL(const QString& boardName);

    /**
     * @brief List all supported boards.
     * 
     * @return List of board names
     */
    static QStringList getSupportedBoards();

    /**
     * @brief Check if a board is currently supported.
     * 
     * @param boardName Board to check
     * @return true if supported
     */
    static bool isSupported(const QString& boardName);

    /**
     * @brief Get default OpenOCD configuration file path for board.
     * @param boardName Board name (e.g. "ESP32-DEVKITV1", "STM32F030R8")
     * @return Path to OpenOCD config file (resource path or empty string)
     */
    static QString getOpenOCDConfigPath(const QString& boardName);
};

#endif // BOARD_FACTORY_H

#pragma once

#include <QString>
#include <QList>
#include <QVariantMap>

struct LVGLWidgetInstance {
    QString id;
    QString type; // e.g. "Button", "Slider", "Container", etc.
    QString name;
    int x = 0;
    int y = 0;
    int width = 100;
    int height = 40;
    QVariantMap properties;
};

/**
 * @brief LVGLCodeGenerator generates complete, compilable LVGL v8/v9 C code
 * for embedded systems (STM32, ESP32, RP2040, etc.) from canvas UI widgets.
 */
class LVGLCodeGenerator {
public:
    LVGLCodeGenerator() = default;
    ~LVGLCodeGenerator() = default;

    void addWidget(const LVGLWidgetInstance& widget);
    void clearWidgets();

    QString generateHeader(const QString& targetBoard = "stm32") const;
    QString generateSource(const QString& targetBoard = "stm32") const;

    // Export both files into directory
    bool exportToDirectory(const QString& outputDir, const QString& targetBoard = "stm32") const;

private:
    QList<LVGLWidgetInstance> m_widgets;
};

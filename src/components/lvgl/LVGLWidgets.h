#pragma once

#include "models/ButtonComponent.h"
#include "models/LabelComponent.h"
#include "models/SliderComponent.h"
#include "models/SwitchComponent.h"
#include "models/GaugeComponent.h"
#include "models/RectangleComponent.h"

// LVGL Component Model Wrappers
class LVGLButton : public ButtonComponent {
public:
    explicit LVGLButton(const QString& id = "lv_button_1", QGraphicsItem* parent = nullptr)
        : ButtonComponent(id, parent) {
        setText("Click Me");
    }
    QString widgetName() const { return componentId(); }
};

class LVGLLabel : public LabelComponent {
public:
    explicit LVGLLabel(const QString& id = "lv_label_1", QGraphicsItem* parent = nullptr)
        : LabelComponent(id, parent) {
        setText("Text Label");
    }
    QString widgetName() const { return componentId(); }
};

class LVGLSlider : public SliderComponent {
public:
    explicit LVGLSlider(const QString& id = "lv_slider_1", QGraphicsItem* parent = nullptr)
        : SliderComponent(id, parent) {
    }
    QString widgetName() const { return componentId(); }
};

class LVGLSwitch : public SwitchComponent {
public:
    explicit LVGLSwitch(const QString& id = "lv_switch_1", QGraphicsItem* parent = nullptr)
        : SwitchComponent(id, parent) {
    }
    QString widgetName() const { return componentId(); }
};

class LVGLContainer : public RectangleComponent {
public:
    explicit LVGLContainer(const QString& id = "lv_container_1", QGraphicsItem* parent = nullptr)
        : RectangleComponent(id, parent) {
        setCompSize(200, 140);
        setCornerRadius(8);
    }
    QString widgetName() const { return componentId(); }
};

class LVGLGauge : public GaugeComponent {
public:
    explicit LVGLGauge(const QString& id = "lv_gauge_1", QGraphicsItem* parent = nullptr)
        : GaugeComponent(id, parent) {
    }
    QString widgetName() const { return componentId(); }
};

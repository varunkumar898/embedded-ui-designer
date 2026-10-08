# Adding New Components Guide

This guide describes how to add new first-class visual and interactive components to the **Embedded UI Designer** without architectural rewrites.

---

## 1. Architectural Overview

The component architecture follows a clean object-oriented pattern:

1. **`Component` (`src/models/Component.h`)**: Base class for all visual canvas elements, handling ID, type, geometry ($x, y, w, h$), Z-order, visibility, selection, state configuration, and base serialization.
2. **`ValueVisualizationComponent` (`src/models/ValueVisualizationComponent.h`)**: Common base for all numeric/value-driven components. Inherits from `Component` and encapsulates:
   - Engineering value, min, max, step, precision, and units.
   - Normalized ratio calculation (`normalizedValue()` $\in [0.0, 1.0]$).
   - Warning and critical threshold ranges.
   - Auto-state transitions (`"normal"`, `"warning"`, `"critical"`).
   - Data Source property binding.
3. **`ComponentPalette` (`src/panels/ComponentPalette.cpp`)**: Catalog of components grouped into `BASIC`, `INPUT`, `DISPLAY`, `NAVIGATION`, and `DATA`.
4. **`PropertiesPanel` (`src/panels/PropertiesPanel.cpp`)**: Dynamic inspector with dedicated sections per component type and universal Data Binding controls.

---

## 2. Step-by-Step Implementation Workflow

### Step 1: Create the Component Model Header & Source

Inherit from `ValueVisualizationComponent` (for numeric/dial components) or `Component` (for containers/layout controls).

#### Header (`src/models/MyNewComponent.h`):
```cpp
#pragma once
#include "ValueVisualizationComponent.h"

class MyNewComponent : public ValueVisualizationComponent {
public:
    explicit MyNewComponent(const QString& id);

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    
    // Custom properties
    int customMode() const { return m_customMode; }
    void setCustomMode(int mode) { m_customMode = mode; }

    // Serialization
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& json) override;

    // Duplication
    std::unique_ptr<Component> clone() const override;

private:
    int m_customMode = 0;
};
```

#### Implementation (`src/models/MyNewComponent.cpp`):
```cpp
#include "MyNewComponent.h"
#include <QPainter>

MyNewComponent::MyNewComponent(const QString& id)
    : ValueVisualizationComponent(id, "my_new_component") {
    setSize(QSizeF(120, 120));
    setMinimum(0.0);
    setMaximum(100.0);
    setValue(50.0);
}

void MyNewComponent::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    Q_UNUSED(option);
    Q_UNUSED(widget);
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    QRectF rect(0, 0, width(), height());
    // Draw background track
    painter->fillRect(rect, trackColor());

    // Draw value visualization using normalizedValue()
    double ratio = normalizedValue(); // 0.0 to 1.0
    QRectF fillRect(0, 0, width() * ratio, height());
    painter->fillRect(fillRect, activeColor());

    painter->restore();
}

QJsonObject MyNewComponent::toJson() const {
    QJsonObject json = ValueVisualizationComponent::toJson();
    json["customMode"] = m_customMode;
    return json;
}

void MyNewComponent::fromJson(const QJsonObject& json) {
    ValueVisualizationComponent::fromJson(json);
    if (json.contains("customMode")) {
        m_customMode = json["customMode"].toInt();
    }
}

std::unique_ptr<Component> MyNewComponent::clone() const {
    auto c = std::make_unique<MyNewComponent>(id() + "_copy");
    c->fromJson(this->toJson());
    return c;
}
```

---

### Step 2: Register in Component Factory

Update the factory in `src/project/Project.cpp` (and `src/models/Screen.cpp`):

```cpp
// In Project::createComponentInstance
if (type == "my_new_component") {
    return std::make_unique<MyNewComponent>(id);
}
```

---

### Step 3: Register in ComponentPalette

Add the component to the desired category in `src/panels/ComponentPalette.cpp`:

```cpp
// In populateCategories():
QTreeWidgetItem* cat = m_categoryItems.value("DISPLAY");
addPaletteItem(cat, "My New Component", "my_new_component", "badge_icon_name");
```

---

### Step 4: Add Inspector Controls to PropertiesPanel

Add property inspection in `src/panels/PropertiesPanel.cpp`:

1. In `updateComponentProperties(Component* comp)`:
```cpp
if (auto* myComp = dynamic_cast<MyNewComponent*>(comp)) {
    // Expose value / thresholds (inherited from ValueVisualizationComponent)
    populateValueVisualizationFields(myComp);
    
    // Add custom property field
    addSpinBoxField("Custom Mode", myComp->customMode(), [myComp, this](int val) {
        myComp->setCustomMode(val);
        emit propertyChanged(myComp, "customMode", val);
    });
}
```

---

### Step 5: Update CMakeLists.txt

Add the new `.h` and `.cpp` files to `CMakeLists.txt`:
- `src/models/MyNewComponent.h`
- `src/models/MyNewComponent.cpp`

---

### Step 6: Add Unit & Functional Tests

Add automated tests in `tests/test_all_cases.cpp` and `tests/test_functional_runner.cpp`:
1. Creation and default property inspection.
2. Value normalization and threshold auto-state transitions.
3. JSON serialization round-trip verification (`toJson` / `fromJson`).
4. Canvas rendering without memory corruption or bounds clipping.

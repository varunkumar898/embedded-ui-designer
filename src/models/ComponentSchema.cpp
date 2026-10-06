#include "ComponentSchema.h"
#include "UIComponent.h"
#include "ButtonComponent.h"
#include "LabelComponent.h"
#include "RectangleComponent.h"
#include "ProgressBarComponent.h"
#include "SliderComponent.h"
#include "SwitchComponent.h"
#include "CheckboxComponent.h"
#include "TextInputComponent.h"
#include "ImageComponent.h"
#include "CircleComponent.h"
#include "PathComponent.h"

#include <QColor>
#include <cmath>

QJsonObject PropertyDefinition::toJson() const {
    QJsonObject obj;
    obj["name"] = name;
    obj["type"] = type;
    obj["defaultValue"] = defaultValue;
    obj["editable"] = editable;
    obj["description"] = description;
    if (hasRange) {
        obj["minimum"] = minimum;
        obj["maximum"] = maximum;
    }
    if (!enumValues.isEmpty()) {
        QJsonArray arr;
        for (const QString& v : enumValues) arr.append(v);
        obj["enumValues"] = arr;
    }
    return obj;
}

PropertyDefinition PropertyDefinition::fromJson(const QJsonObject& json) {
    PropertyDefinition def;
    def.name = json["name"].toString();
    def.type = json["type"].toString();
    def.defaultValue = json["defaultValue"];
    def.editable = json.value("editable").toBool(true);
    def.description = json["description"].toString();
    if (json.contains("minimum") && json.contains("maximum")) {
        def.hasRange = true;
        def.minimum = json["minimum"].toDouble();
        def.maximum = json["maximum"].toDouble();
    }
    if (json.contains("enumValues")) {
        for (const auto& v : json["enumValues"].toArray()) {
            def.enumValues.append(v.toString());
        }
    }
    return def;
}

QJsonObject ComponentTypeSchema::toJson() const {
    QJsonObject obj;
    obj["type"] = type;
    obj["className"] = className;
    obj["displayName"] = displayName;
    obj["category"] = category;

    QJsonArray propsArr;
    for (const auto& p : properties) {
        propsArr.append(p.toJson());
    }
    obj["properties"] = propsArr;
    obj["defaults"] = defaultProperties;

    QJsonArray capArr;
    for (const auto& c : capabilities) {
        capArr.append(c);
    }
    obj["capabilities"] = capArr;
    return obj;
}

const PropertyDefinition* ComponentTypeSchema::findProperty(const QString& name) const {
    for (const auto& p : properties) {
        if (p.name.compare(name, Qt::CaseInsensitive) == 0) {
            return &p;
        }
    }
    return nullptr;
}

ComponentSchemaRegistry& ComponentSchemaRegistry::instance() {
    static ComponentSchemaRegistry s_reg;
    return s_reg;
}

ComponentSchemaRegistry::ComponentSchemaRegistry() {
    registerAllBuiltins();
}

const QList<ComponentTypeSchema>& ComponentSchemaRegistry::allSchemas() const {
    return m_schemas;
}

QString ComponentSchemaRegistry::normalizeTypeName(const QString& typeName) const {
    QString t = typeName.trimmed().toLower().replace("-", "_").replace(" ", "_");
    if (t == "text" || t == "label" || t == "labelcomponent") return "label";
    if (t == "btn" || t == "button" || t == "buttoncomponent") return "button";
    if (t == "rect" || t == "rectangle" || t == "rectanglecomponent" || t == "shape") return "rectangle";
    if (t == "progress" || t == "progressbar" || t == "progress_bar" || t == "progressbarcomponent") return "progress_bar";
    if (t == "slider" || t == "slidercomponent") return "slider";
    if (t == "switch" || t == "switchcomponent" || t == "toggle") return "switch";
    if (t == "checkbox" || t == "check_box" || t == "checkboxcomponent") return "checkbox";
    if (t == "textinput" || t == "text_input" || t == "textinputcomponent" || t == "input" || t == "textfield") return "text_input";
    if (t == "image" || t == "imagecomponent" || t == "img") return "image";
    if (t == "circle" || t == "circlecomponent") return "circle";
    if (t == "path" || t == "pathcomponent") return "path";
    return t;
}

const ComponentTypeSchema* ComponentSchemaRegistry::findSchema(const QString& typeName) const {
    QString norm = normalizeTypeName(typeName);
    auto it = m_typeMap.find(norm);
    if (it != m_typeMap.end()) {
        return &m_schemas[it.value()];
    }
    return nullptr;
}

QStringList ComponentSchemaRegistry::availableTypes() const {
    QStringList types;
    for (const auto& s : m_schemas) {
        types.append(s.type);
    }
    return types;
}

QJsonObject ComponentSchemaRegistry::fullSchemaJson() const {
    QJsonObject root;
    QJsonArray typesArr;
    for (const auto& s : m_schemas) {
        typesArr.append(s.toJson());
    }
    root["componentTypes"] = typesArr;
    return root;
}

void ComponentSchemaRegistry::registerAllBuiltins() {
    m_schemas.clear();
    m_typeMap.clear();

    // 1. PROGRESS BAR
    {
        ComponentTypeSchema s;
        s.type = "progress_bar";
        s.className = "ProgressBarComponent";
        s.displayName = "Progress Bar";
        s.category = "Controls";
        s.capabilities = {"resize", "move", "colors", "styles", "interactive_radius"};
        s.properties = {
            {"value", "double", 0.0, true, 0.0, 100.0, true, {}, "Current progress value"},
            {"minimum", "double", 0.0, true, -100000.0, 100000.0, true, {}, "Minimum value"},
            {"maximum", "double", 100.0, true, -100000.0, 100000.0, true, {}, "Maximum value"},
            {"orientation", "enum", "Horizontal", true, 0, 0, false, {"Horizontal", "Vertical"}, "Bar orientation"},
            {"barColor", "color", "#10b981", true, 0, 0, false, {}, "Progress filled bar color"},
            {"trackColor", "color", "#1c2028", true, 0, 0, false, {}, "Track background color"},
            {"borderColor", "color", "#2d3441", true, 0, 0, false, {}, "Outer border stroke color"},
            {"borderWidth", "int", 1, true, 0, 20, true, {}, "Border stroke width in pixels"},
            {"cornerRadius", "int", 8, true, 0, 50, true, {}, "Corner rounding radius"},
            {"visible", "bool", true, true, 0, 0, false, {}, "Component visibility"}
        };
        s.defaultProperties = {
            {"width", 260}, {"height", 24}, {"value", 65.0}, {"minimum", 0.0}, {"maximum", 100.0},
            {"orientation", "Horizontal"}, {"barColor", "#10b981"}, {"trackColor", "#1c2028"},
            {"borderColor", "#2d3441"}, {"borderWidth", 1}, {"cornerRadius", 8}, {"visible", true}
        };
        m_typeMap[s.type] = m_schemas.size();
        m_schemas.append(s);
    }

    // 2. BUTTON
    {
        ComponentTypeSchema s;
        s.type = "button";
        s.className = "ButtonComponent";
        s.displayName = "Button";
        s.category = "Controls";
        s.capabilities = {"resize", "move", "colors", "styles", "interactive_radius"};
        s.properties = {
            {"text", "string", "Button", true, 0, 0, false, {}, "Button label text"},
            {"backgroundColor", "color", "#2563eb", true, 0, 0, false, {}, "Button background color"},
            {"textColor", "color", "#ffffff", true, 0, 0, false, {}, "Text label color"},
            {"borderColor", "color", "#3b82f6", true, 0, 0, false, {}, "Border outline color"},
            {"borderWidth", "int", 0, true, 0, 20, true, {}, "Border width in pixels"},
            {"cornerRadius", "int", 6, true, 0, 50, true, {}, "Corner radius in pixels"},
            {"fontFamily", "enum", "Inter", true, 0, 0, false, LabelComponent::availableFonts(), "Typography font family"},
            {"pixelSize", "int", 14, true, 6, 96, true, {}, "Font size in pixels"},
            {"bold", "bool", false, true, 0, 0, false, {}, "Bold font weight"},
            {"enabled", "bool", true, true, 0, 0, false, {}, "Interactive enabled state"},
            {"onClicked", "string", "", true, 0, 0, false, {}, "MCU click callback handler name"},
            {"visible", "bool", true, true, 0, 0, false, {}, "Component visibility"}
        };
        s.defaultProperties = {
            {"width", 120}, {"height", 40}, {"text", "Button"}, {"backgroundColor", "#2563eb"},
            {"textColor", "#ffffff"}, {"borderColor", "#3b82f6"}, {"borderWidth", 0},
            {"cornerRadius", 6}, {"fontFamily", "Inter"}, {"pixelSize", 14}, {"bold", false},
            {"enabled", true}, {"visible", true}
        };
        m_typeMap[s.type] = m_schemas.size();
        m_schemas.append(s);
    }

    // 3. LABEL
    {
        ComponentTypeSchema s;
        s.type = "label";
        s.className = "LabelComponent";
        s.displayName = "Label";
        s.category = "Display";
        s.capabilities = {"resize", "move", "colors", "styles"};
        s.properties = {
            {"text", "string", "Label", true, 0, 0, false, {}, "Displayed text"},
            {"color", "color", "#ffffff", true, 0, 0, false, {}, "Text color"},
            {"backgroundColor", "color", "transparent", true, 0, 0, false, {}, "Optional background fill color"},
            {"cornerRadius", "int", 0, true, 0, 50, true, {}, "Corner radius when background is filled"},
            {"fontFamily", "enum", "Inter", true, 0, 0, false, LabelComponent::availableFonts(), "Font family"},
            {"pixelSize", "int", 16, true, 6, 96, true, {}, "Font pixel size"},
            {"bold", "bool", false, true, 0, 0, false, {}, "Bold text"},
            {"italic", "bool", false, true, 0, 0, false, {}, "Italic text"},
            {"alignment", "enum", "Left", true, 0, 0, false, {"Left", "Center", "Right"}, "Horizontal text alignment"},
            {"letterSpacing", "double", 0.0, true, -5.0, 20.0, true, {}, "Letter spacing in pixels"},
            {"lineHeight", "int", 0, true, 0, 400, true, {}, "Line height percentage (0=default)"},
            {"visible", "bool", true, true, 0, 0, false, {}, "Component visibility"}
        };
        s.defaultProperties = {
            {"width", 140}, {"height", 30}, {"text", "Label"}, {"color", "#ffffff"},
            {"backgroundColor", "transparent"}, {"cornerRadius", 0}, {"fontFamily", "Inter"},
            {"pixelSize", 16}, {"bold", false}, {"italic", false}, {"alignment", "Left"},
            {"letterSpacing", 0.0}, {"lineHeight", 0}, {"visible", true}
        };
        m_typeMap[s.type] = m_schemas.size();
        m_schemas.append(s);
    }

    // 4. RECTANGLE / SHAPE
    {
        ComponentTypeSchema s;
        s.type = "rectangle";
        s.className = "RectangleComponent";
        s.displayName = "Rectangle / Shape";
        s.category = "Shapes";
        s.capabilities = {"resize", "move", "colors", "styles", "interactive_radius"};
        s.properties = {
            {"fillColor", "color", "#1e222a", true, 0, 0, false, {}, "Inner background fill color"},
            {"strokeColor", "color", "#323846", true, 0, 0, false, {}, "Border stroke line color"},
            {"strokeWidth", "int", 1, true, 0, 20, true, {}, "Stroke width in pixels"},
            {"cornerRadius", "int", 8, true, 0, 50, true, {}, "Corner radius in pixels"},
            {"visible", "bool", true, true, 0, 0, false, {}, "Component visibility"}
        };
        s.defaultProperties = {
            {"width", 140}, {"height", 80}, {"fillColor", "#1e222a"}, {"strokeColor", "#323846"},
            {"strokeWidth", 1}, {"cornerRadius", 8}, {"visible", true}
        };
        m_typeMap[s.type] = m_schemas.size();
        m_schemas.append(s);
    }

    // 5. SLIDER
    {
        ComponentTypeSchema s;
        s.type = "slider";
        s.className = "SliderComponent";
        s.displayName = "Slider";
        s.category = "Input";
        s.capabilities = {"resize", "move", "colors", "styles"};
        s.properties = {
            {"value", "int", 50, true, -10000, 10000, true, {}, "Current slider integer value"},
            {"minimum", "int", 0, true, -10000, 10000, true, {}, "Minimum value"},
            {"maximum", "int", 100, true, -10000, 10000, true, {}, "Maximum value"},
            {"orientation", "enum", "Horizontal", true, 0, 0, false, {"Horizontal", "Vertical"}, "Orientation"},
            {"trackColor", "color", "#252830", true, 0, 0, false, {}, "Track background color"},
            {"fillColor", "color", "#10b981", true, 0, 0, false, {}, "Active filled track color"},
            {"handleColor", "color", "#ffffff", true, 0, 0, false, {}, "Thumb/handle knob color"},
            {"borderColor", "color", "#3b404e", true, 0, 0, false, {}, "Border stroke color"},
            {"borderWidth", "int", 1, true, 0, 20, true, {}, "Border width"},
            {"cornerRadius", "int", 4, true, 0, 50, true, {}, "Corner rounding radius"},
            {"visible", "bool", true, true, 0, 0, false, {}, "Component visibility"}
        };
        s.defaultProperties = {
            {"width", 180}, {"height", 32}, {"value", 50}, {"minimum", 0}, {"maximum", 100},
            {"orientation", "Horizontal"}, {"trackColor", "#252830"}, {"fillColor", "#10b981"},
            {"handleColor", "#ffffff"}, {"borderColor", "#3b404e"}, {"borderWidth", 1},
            {"cornerRadius", 4}, {"visible", true}
        };
        m_typeMap[s.type] = m_schemas.size();
        m_schemas.append(s);
    }

    // 6. SWITCH
    {
        ComponentTypeSchema s;
        s.type = "switch";
        s.className = "SwitchComponent";
        s.displayName = "Switch";
        s.category = "Input";
        s.capabilities = {"resize", "move", "colors", "styles"};
        s.properties = {
            {"checked", "bool", true, true, 0, 0, false, {}, "Toggled switch state"},
            {"onColor", "color", "#10b981", true, 0, 0, false, {}, "Background track color when ON"},
            {"offColor", "color", "#374151", true, 0, 0, false, {}, "Background track color when OFF"},
            {"thumbColor", "color", "#ffffff", true, 0, 0, false, {}, "Sliding thumb circle color"},
            {"borderColor", "color", "#4b5563", true, 0, 0, false, {}, "Outer border color"},
            {"borderWidth", "int", 1, true, 0, 20, true, {}, "Outer border width"},
            {"cornerRadius", "int", 15, true, 0, 50, true, {}, "Track corner radius"},
            {"onToggled", "string", "", true, 0, 0, false, {}, "MCU toggle callback handler"},
            {"visible", "bool", true, true, 0, 0, false, {}, "Component visibility"}
        };
        s.defaultProperties = {
            {"width", 54}, {"height", 30}, {"checked", true}, {"onColor", "#10b981"},
            {"offColor", "#374151"}, {"thumbColor", "#ffffff"}, {"borderColor", "#4b5563"},
            {"borderWidth", 1}, {"cornerRadius", 15}, {"visible", true}
        };
        m_typeMap[s.type] = m_schemas.size();
        m_schemas.append(s);
    }

    // 7. CHECKBOX
    {
        ComponentTypeSchema s;
        s.type = "checkbox";
        s.className = "CheckboxComponent";
        s.displayName = "Checkbox";
        s.category = "Input";
        s.capabilities = {"resize", "move", "colors", "styles"};
        s.properties = {
            {"text", "string", "Checkbox", true, 0, 0, false, {}, "Checkbox text label"},
            {"checked", "bool", false, true, 0, 0, false, {}, "Checked boolean state"},
            {"textColor", "color", "#ffffff", true, 0, 0, false, {}, "Label font color"},
            {"checkColor", "color", "#ffffff", true, 0, 0, false, {}, "Checkmark icon color"},
            {"boxColor", "color", "#10b981", true, 0, 0, false, {}, "Box filled color when checked"},
            {"borderColor", "color", "#4b5563", true, 0, 0, false, {}, "Box border outline color"},
            {"borderWidth", "int", 1, true, 0, 10, true, {}, "Box border width"},
            {"fontFamily", "enum", "Inter", true, 0, 0, false, LabelComponent::availableFonts(), "Typography font family"},
            {"pixelSize", "int", 13, true, 6, 96, true, {}, "Font size in pixels"},
            {"onToggled", "string", "", true, 0, 0, false, {}, "MCU callback handler"},
            {"visible", "bool", true, true, 0, 0, false, {}, "Component visibility"}
        };
        s.defaultProperties = {
            {"width", 130}, {"height", 28}, {"text", "Checkbox"}, {"checked", false},
            {"textColor", "#ffffff"}, {"checkColor", "#ffffff"}, {"boxColor", "#10b981"},
            {"borderColor", "#4b5563"}, {"borderWidth", 1}, {"fontFamily", "Inter"},
            {"pixelSize", 13}, {"visible", true}
        };
        m_typeMap[s.type] = m_schemas.size();
        m_schemas.append(s);
    }

    // 8. TEXT INPUT
    {
        ComponentTypeSchema s;
        s.type = "text_input";
        s.className = "TextInputComponent";
        s.displayName = "Text Input";
        s.category = "Input";
        s.capabilities = {"resize", "move", "colors", "styles"};
        s.properties = {
            {"text", "string", "", true, 0, 0, false, {}, "Input field content text"},
            {"placeholder", "string", "Enter text...", true, 0, 0, false, {}, "Placeholder hint text"},
            {"textColor", "color", "#ffffff", true, 0, 0, false, {}, "Entered text color"},
            {"placeholderColor", "color", "#64748b", true, 0, 0, false, {}, "Placeholder text color"},
            {"backgroundColor", "color", "#1e222a", true, 0, 0, false, {}, "Background fill color"},
            {"borderColor", "color", "#3b404e", true, 0, 0, false, {}, "Input field border color"},
            {"borderWidth", "int", 1, true, 0, 20, true, {}, "Border stroke width"},
            {"cornerRadius", "int", 6, true, 0, 50, true, {}, "Corner rounding radius"},
            {"fontFamily", "enum", "Inter", true, 0, 0, false, LabelComponent::availableFonts(), "Font family"},
            {"pixelSize", "int", 13, true, 6, 96, true, {}, "Font pixel size"},
            {"readOnly", "bool", false, true, 0, 0, false, {}, "Read-only disabled editing"},
            {"onTextChanged", "string", "", true, 0, 0, false, {}, "MCU text change handler name"},
            {"visible", "bool", true, true, 0, 0, false, {}, "Component visibility"}
        };
        s.defaultProperties = {
            {"width", 160}, {"height", 36}, {"text", ""}, {"placeholder", "Enter text..."},
            {"textColor", "#ffffff"}, {"placeholderColor", "#64748b"}, {"backgroundColor", "#1e222a"},
            {"borderColor", "#3b404e"}, {"borderWidth", 1}, {"cornerRadius", 6},
            {"fontFamily", "Inter"}, {"pixelSize", 13}, {"readOnly", false}, {"visible", true}
        };
        m_typeMap[s.type] = m_schemas.size();
        m_schemas.append(s);
    }

    // 9. IMAGE
    {
        ComponentTypeSchema s;
        s.type = "image";
        s.className = "ImageComponent";
        s.displayName = "Image";
        s.category = "Media";
        s.capabilities = {"resize", "move"};
        s.properties = {
            {"imagePath", "string", "", true, 0, 0, false, {}, "Filesystem path to image asset"},
            {"format", "enum", "RGB565", true, 0, 0, false, {"RGB565", "Monochrome"}, "Color encoding format"},
            {"opacity", "int", 100, true, 0, 100, true, {}, "Opacity percentage (0-100)"},
            {"scalingMode", "enum", "KeepAspectRatio", true, 0, 0, false, {"KeepAspectRatio", "Stretch", "Center"}, "Image scaling behavior"},
            {"visible", "bool", true, true, 0, 0, false, {}, "Component visibility"}
        };
        s.defaultProperties = {
            {"width", 80}, {"height", 80}, {"imagePath", ""}, {"format", "RGB565"},
            {"opacity", 100}, {"scalingMode", "KeepAspectRatio"}, {"visible", true}
        };
        m_typeMap[s.type] = m_schemas.size();
        m_schemas.append(s);
    }

    // 10. CIRCLE
    {
        ComponentTypeSchema s;
        s.type = "circle";
        s.className = "CircleComponent";
        s.displayName = "Circle";
        s.category = "Shapes";
        s.capabilities = {"resize", "move", "colors", "styles"};
        s.properties = {
            {"fillColor", "color", "#10b981", true, 0, 0, false, {}, "Circle interior fill color"},
            {"strokeColor", "color", "#34d399", true, 0, 0, false, {}, "Outline border stroke color"},
            {"strokeWidth", "int", 1, true, 0, 20, true, {}, "Stroke thickness in pixels"},
            {"filled", "bool", true, true, 0, 0, false, {}, "Whether interior is filled"},
            {"visible", "bool", true, true, 0, 0, false, {}, "Component visibility"}
        };
        s.defaultProperties = {
            {"width", 50}, {"height", 50}, {"fillColor", "#10b981"}, {"strokeColor", "#34d399"},
            {"strokeWidth", 1}, {"filled", true}, {"visible", true}
        };
        m_typeMap[s.type] = m_schemas.size();
        m_schemas.append(s);
    }

    // 11. PATH
    {
        ComponentTypeSchema s;
        s.type = "path";
        s.className = "PathComponent";
        s.displayName = "Vector Path";
        s.category = "Shapes";
        s.capabilities = {"resize", "move", "colors"};
        s.properties = {
            {"strokeColor", "color", "#38bdf8", true, 0, 0, false, {}, "Vector line stroke color"},
            {"strokeWidth", "int", 2, true, 1, 64, true, {}, "Vector line thickness"},
            {"opacity", "int", 100, true, 0, 100, true, {}, "Path opacity (0-100)"},
            {"visible", "bool", true, true, 0, 0, false, {}, "Component visibility"}
        };
        s.defaultProperties = {
            {"width", 60}, {"height", 60}, {"strokeColor", "#38bdf8"},
            {"strokeWidth", 2}, {"opacity", 100}, {"visible", true}
        };
        m_typeMap[s.type] = m_schemas.size();
        m_schemas.append(s);
    }
}

UIComponent* ComponentSchemaRegistry::createComponent(const QString& typeName, const QString& id) const {
    QString norm = normalizeTypeName(typeName);
    if (norm == "progress_bar") return new ProgressBarComponent(id);
    if (norm == "button") return new ButtonComponent(id);
    if (norm == "label") return new LabelComponent(id);
    if (norm == "rectangle") return new RectangleComponent(id);
    if (norm == "slider") return new SliderComponent(id);
    if (norm == "switch") return new SwitchComponent(id);
    if (norm == "checkbox") return new CheckboxComponent(id);
    if (norm == "text_input") return new TextInputComponent(id);
    if (norm == "image") return new ImageComponent(id);
    if (norm == "circle") return new CircleComponent(id);
    if (norm == "path") return new PathComponent(id);
    return nullptr;
}

QJsonObject ComponentSchemaRegistry::getComponentProperties(const UIComponent* comp) const {
    QJsonObject props;
    if (!comp) return props;

    // Common properties
    props["visible"] = comp->isComponentVisible();

    if (auto pb = dynamic_cast<const ProgressBarComponent*>(comp)) {
        props["value"] = pb->actualValue();
        props["minimum"] = pb->minimum();
        props["maximum"] = pb->maximum();
        props["orientation"] = pb->orientation();
        props["barColor"] = pb->barColor().name();
        props["trackColor"] = pb->trackColor().name();
        props["borderColor"] = pb->borderColor().name();
        props["borderWidth"] = pb->borderWidth();
        props["cornerRadius"] = pb->cornerRadius();
    } else if (auto btn = dynamic_cast<const ButtonComponent*>(comp)) {
        props["text"] = btn->text();
        props["backgroundColor"] = btn->backgroundColor().name();
        props["textColor"] = btn->textColor().name();
        props["borderColor"] = btn->borderColor().name();
        props["borderWidth"] = btn->borderWidth();
        props["cornerRadius"] = btn->cornerRadius();
        props["fontFamily"] = btn->fontFamily();
        props["pixelSize"] = btn->pixelSize();
        props["bold"] = btn->bold();
        props["enabled"] = btn->isEnabled();
        props["onClicked"] = btn->onClickedHandler();
    } else if (auto lbl = dynamic_cast<const LabelComponent*>(comp)) {
        props["text"] = lbl->text();
        props["color"] = lbl->color().name();
        props["backgroundColor"] = lbl->backgroundColor().isValid() ? lbl->backgroundColor().name() : "transparent";
        props["cornerRadius"] = lbl->cornerRadius();
        props["fontFamily"] = lbl->fontFamily();
        props["pixelSize"] = lbl->pixelSize();
        props["bold"] = lbl->bold();
        props["italic"] = lbl->italic();
        Qt::Alignment al = lbl->alignment();
        props["alignment"] = (al & Qt::AlignRight) ? "Right" : (al & Qt::AlignHCenter) ? "Center" : "Left";
        props["letterSpacing"] = lbl->letterSpacing();
        props["lineHeight"] = lbl->lineHeight();
    } else if (auto rect = dynamic_cast<const RectangleComponent*>(comp)) {
        props["fillColor"] = rect->fillColor().name();
        props["strokeColor"] = rect->strokeColor().name();
        props["strokeWidth"] = rect->strokeWidth();
        props["cornerRadius"] = rect->cornerRadius();
    } else if (auto sld = dynamic_cast<const SliderComponent*>(comp)) {
        props["value"] = sld->value();
        props["minimum"] = sld->minimum();
        props["maximum"] = sld->maximum();
        props["orientation"] = sld->orientation();
        props["trackColor"] = sld->trackColor().name();
        props["fillColor"] = sld->fillColor().name();
        props["handleColor"] = sld->handleColor().name();
        props["borderColor"] = sld->borderColor().name();
        props["borderWidth"] = sld->borderWidth();
        props["cornerRadius"] = sld->cornerRadius();
    } else if (auto sw = dynamic_cast<const SwitchComponent*>(comp)) {
        props["checked"] = sw->isChecked();
        props["onColor"] = sw->onColor().name();
        props["offColor"] = sw->offColor().name();
        props["thumbColor"] = sw->thumbColor().name();
        props["borderColor"] = sw->borderColor().name();
        props["borderWidth"] = sw->borderWidth();
        props["cornerRadius"] = sw->cornerRadius();
        props["onToggled"] = sw->onToggledHandler();
    } else if (auto chk = dynamic_cast<const CheckboxComponent*>(comp)) {
        props["text"] = chk->text();
        props["checked"] = chk->isChecked();
        props["textColor"] = chk->textColor().name();
        props["checkColor"] = chk->checkColor().name();
        props["boxColor"] = chk->boxColor().name();
        props["borderColor"] = chk->borderColor().name();
        props["borderWidth"] = chk->borderWidth();
        props["fontFamily"] = chk->fontFamily();
        props["pixelSize"] = chk->pixelSize();
        props["onToggled"] = chk->onToggledHandler();
    } else if (auto txt = dynamic_cast<const TextInputComponent*>(comp)) {
        props["text"] = txt->text();
        props["placeholder"] = txt->placeholder();
        props["textColor"] = txt->textColor().name();
        props["placeholderColor"] = txt->placeholderColor().name();
        props["backgroundColor"] = txt->backgroundColor().name();
        props["borderColor"] = txt->borderColor().name();
        props["borderWidth"] = txt->borderWidth();
        props["cornerRadius"] = txt->cornerRadius();
        props["fontFamily"] = txt->fontFamily();
        props["pixelSize"] = txt->pixelSize();
        props["readOnly"] = txt->isReadOnly();
        props["onTextChanged"] = txt->onTextChangedHandler();
    } else if (auto img = dynamic_cast<const ImageComponent*>(comp)) {
        props["imagePath"] = img->imagePath();
        props["format"] = img->format();
        props["opacity"] = img->opacityPercent();
        props["scalingMode"] = img->scalingMode();
    } else if (auto circ = dynamic_cast<const CircleComponent*>(comp)) {
        props["fillColor"] = circ->fillColor().name();
        props["strokeColor"] = circ->strokeColor().name();
        props["strokeWidth"] = circ->strokeWidth();
        props["filled"] = circ->isFilled();
    } else if (auto p = dynamic_cast<const PathComponent*>(comp)) {
        props["strokeColor"] = p->strokeColor().name();
        props["strokeWidth"] = qRound(p->strokeThickness());
        props["opacity"] = p->opacityPercent();
    }

    return props;
}

static bool parseColor(const QJsonValue& val, QColor* outColor) {
    if (!outColor) return false;
    if (val.isString()) {
        QString s = val.toString().trimmed();
        if (s.compare("transparent", Qt::CaseInsensitive) == 0) {
            *outColor = QColor(0, 0, 0, 0);
            return true;
        }
        QColor c(s);
        if (c.isValid()) {
            *outColor = c;
            return true;
        }
    }
    return false;
}

bool ComponentSchemaRegistry::setComponentProperty(UIComponent* comp, const QString& propName, const QJsonValue& val, QString* error) const {
    if (!comp) {
        if (error) *error = "Null component target";
        return false;
    }

    QString key = propName.trimmed();

    // Universal geometry/visibility properties
    if (key.compare("visible", Qt::CaseInsensitive) == 0) {
        comp->setComponentVisible(val.toBool(true));
        return true;
    }
    if (key.compare("x", Qt::CaseInsensitive) == 0) {
        comp->setCompPos(val.toDouble(), comp->compY());
        return true;
    }
    if (key.compare("y", Qt::CaseInsensitive) == 0) {
        comp->setCompPos(comp->compX(), val.toDouble());
        return true;
    }
    if (key.compare("width", Qt::CaseInsensitive) == 0) {
        comp->setCompSize(val.toDouble(), comp->compHeight());
        return true;
    }
    if (key.compare("height", Qt::CaseInsensitive) == 0) {
        comp->setCompSize(comp->compWidth(), val.toDouble());
        return true;
    }

    // Specific component dispatches
    if (auto pb = dynamic_cast<ProgressBarComponent*>(comp)) {
        if (key.compare("value", Qt::CaseInsensitive) == 0 || key.compare("progress", Qt::CaseInsensitive) == 0) {
            pb->setActualValue(val.toDouble());
            return true;
        }
        if (key.compare("minimum", Qt::CaseInsensitive) == 0 || key.compare("min", Qt::CaseInsensitive) == 0) {
            pb->setMinimum(val.toDouble());
            return true;
        }
        if (key.compare("maximum", Qt::CaseInsensitive) == 0 || key.compare("max", Qt::CaseInsensitive) == 0) {
            pb->setMaximum(val.toDouble());
            return true;
        }
        if (key.compare("orientation", Qt::CaseInsensitive) == 0) {
            QString o = val.toString();
            if (o.compare("vertical", Qt::CaseInsensitive) == 0) pb->setOrientation("Vertical");
            else pb->setOrientation("Horizontal");
            return true;
        }
        if (key.compare("barColor", Qt::CaseInsensitive) == 0 || key.compare("fillColor", Qt::CaseInsensitive) == 0) {
            QColor c;
            if (parseColor(val, &c)) { pb->setBarColor(c); return true; }
            if (error) *error = "Invalid color value: " + val.toString();
            return false;
        }
        if (key.compare("trackColor", Qt::CaseInsensitive) == 0) {
            QColor c;
            if (parseColor(val, &c)) { pb->setTrackColor(c); return true; }
            if (error) *error = "Invalid color value: " + val.toString();
            return false;
        }
        if (key.compare("borderColor", Qt::CaseInsensitive) == 0) {
            QColor c;
            if (parseColor(val, &c)) { pb->setBorderColor(c); return true; }
            if (error) *error = "Invalid color value: " + val.toString();
            return false;
        }
        if (key.compare("borderWidth", Qt::CaseInsensitive) == 0) {
            pb->setBorderWidth(val.toInt());
            return true;
        }
        if (key.compare("cornerRadius", Qt::CaseInsensitive) == 0 || key.compare("radius", Qt::CaseInsensitive) == 0) {
            pb->setCornerRadius(val.toInt());
            return true;
        }
    } else if (auto btn = dynamic_cast<ButtonComponent*>(comp)) {
        if (key.compare("text", Qt::CaseInsensitive) == 0) {
            btn->setText(val.toString());
            return true;
        }
        if (key.compare("backgroundColor", Qt::CaseInsensitive) == 0 || key.compare("bgColor", Qt::CaseInsensitive) == 0) {
            QColor c;
            if (parseColor(val, &c)) { btn->setBackgroundColor(c); return true; }
            if (error) *error = "Invalid color value";
            return false;
        }
        if (key.compare("textColor", Qt::CaseInsensitive) == 0) {
            QColor c;
            if (parseColor(val, &c)) { btn->setTextColor(c); return true; }
            if (error) *error = "Invalid color value";
            return false;
        }
        if (key.compare("borderColor", Qt::CaseInsensitive) == 0) {
            QColor c;
            if (parseColor(val, &c)) { btn->setBorderColor(c); return true; }
            if (error) *error = "Invalid color value";
            return false;
        }
        if (key.compare("borderWidth", Qt::CaseInsensitive) == 0) {
            btn->setBorderWidth(val.toInt());
            return true;
        }
        if (key.compare("cornerRadius", Qt::CaseInsensitive) == 0 || key.compare("radius", Qt::CaseInsensitive) == 0) {
            btn->setCornerRadius(val.toInt());
            return true;
        }
        if (key.compare("fontFamily", Qt::CaseInsensitive) == 0 || key.compare("font", Qt::CaseInsensitive) == 0) {
            btn->setFontFamily(val.toString());
            return true;
        }
        if (key.compare("pixelSize", Qt::CaseInsensitive) == 0 || key.compare("fontSize", Qt::CaseInsensitive) == 0) {
            btn->setPixelSize(val.toInt());
            return true;
        }
        if (key.compare("bold", Qt::CaseInsensitive) == 0) {
            btn->setBold(val.toBool());
            return true;
        }
        if (key.compare("enabled", Qt::CaseInsensitive) == 0) {
            btn->setEnabled(val.toBool(true));
            return true;
        }
        if (key.compare("onClicked", Qt::CaseInsensitive) == 0) {
            btn->setOnClickedHandler(val.toString());
            return true;
        }
    } else if (auto lbl = dynamic_cast<LabelComponent*>(comp)) {
        if (key.compare("text", Qt::CaseInsensitive) == 0) {
            lbl->setText(val.toString());
            return true;
        }
        if (key.compare("color", Qt::CaseInsensitive) == 0 || key.compare("textColor", Qt::CaseInsensitive) == 0) {
            QColor c;
            if (parseColor(val, &c)) { lbl->setColor(c); return true; }
            if (error) *error = "Invalid color value";
            return false;
        }
        if (key.compare("backgroundColor", Qt::CaseInsensitive) == 0 || key.compare("background", Qt::CaseInsensitive) == 0) {
            QColor c;
            if (parseColor(val, &c)) { lbl->setBackgroundColor(c); return true; }
            if (error) *error = "Invalid color value";
            return false;
        }
        if (key.compare("cornerRadius", Qt::CaseInsensitive) == 0 || key.compare("radius", Qt::CaseInsensitive) == 0) {
            lbl->setCornerRadius(val.toInt());
            return true;
        }
        if (key.compare("fontFamily", Qt::CaseInsensitive) == 0 || key.compare("font", Qt::CaseInsensitive) == 0) {
            lbl->setFontFamily(val.toString());
            return true;
        }
        if (key.compare("pixelSize", Qt::CaseInsensitive) == 0 || key.compare("fontSize", Qt::CaseInsensitive) == 0) {
            lbl->setPixelSize(val.toInt());
            return true;
        }
        if (key.compare("bold", Qt::CaseInsensitive) == 0) {
            lbl->setBold(val.toBool());
            return true;
        }
        if (key.compare("italic", Qt::CaseInsensitive) == 0) {
            lbl->setItalic(val.toBool());
            return true;
        }
        if (key.compare("alignment", Qt::CaseInsensitive) == 0) {
            QString a = val.toString().trimmed().toLower();
            if (a == "center") lbl->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
            else if (a == "right") lbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            else lbl->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            return true;
        }
        if (key.compare("letterSpacing", Qt::CaseInsensitive) == 0) {
            lbl->setLetterSpacing(val.toDouble());
            return true;
        }
        if (key.compare("lineHeight", Qt::CaseInsensitive) == 0) {
            lbl->setLineHeight(val.toInt());
            return true;
        }
    } else if (auto rect = dynamic_cast<RectangleComponent*>(comp)) {
        if (key.compare("fillColor", Qt::CaseInsensitive) == 0 || key.compare("fill", Qt::CaseInsensitive) == 0) {
            QColor c;
            if (parseColor(val, &c)) { rect->setFillColor(c); return true; }
            if (error) *error = "Invalid color value";
            return false;
        }
        if (key.compare("strokeColor", Qt::CaseInsensitive) == 0 || key.compare("stroke", Qt::CaseInsensitive) == 0 || key.compare("borderColor", Qt::CaseInsensitive) == 0) {
            QColor c;
            if (parseColor(val, &c)) { rect->setStrokeColor(c); return true; }
            if (error) *error = "Invalid color value";
            return false;
        }
        if (key.compare("strokeWidth", Qt::CaseInsensitive) == 0 || key.compare("borderWidth", Qt::CaseInsensitive) == 0) {
            rect->setStrokeWidth(val.toInt());
            return true;
        }
        if (key.compare("cornerRadius", Qt::CaseInsensitive) == 0 || key.compare("radius", Qt::CaseInsensitive) == 0) {
            rect->setCornerRadius(val.toInt());
            return true;
        }
    } else if (auto sld = dynamic_cast<SliderComponent*>(comp)) {
        if (key.compare("value", Qt::CaseInsensitive) == 0) {
            sld->setValue(val.toInt());
            return true;
        }
        if (key.compare("minimum", Qt::CaseInsensitive) == 0 || key.compare("min", Qt::CaseInsensitive) == 0) {
            sld->setMinimum(val.toInt());
            return true;
        }
        if (key.compare("maximum", Qt::CaseInsensitive) == 0 || key.compare("max", Qt::CaseInsensitive) == 0) {
            sld->setMaximum(val.toInt());
            return true;
        }
        if (key.compare("orientation", Qt::CaseInsensitive) == 0) {
            QString o = val.toString();
            if (o.compare("vertical", Qt::CaseInsensitive) == 0) sld->setOrientation("Vertical");
            else sld->setOrientation("Horizontal");
            return true;
        }
        if (key.compare("trackColor", Qt::CaseInsensitive) == 0) {
            QColor c;
            if (parseColor(val, &c)) { sld->setTrackColor(c); return true; }
            if (error) *error = "Invalid color value";
            return false;
        }
        if (key.compare("fillColor", Qt::CaseInsensitive) == 0) {
            QColor c;
            if (parseColor(val, &c)) { sld->setFillColor(c); return true; }
            if (error) *error = "Invalid color value";
            return false;
        }
        if (key.compare("handleColor", Qt::CaseInsensitive) == 0 || key.compare("thumbColor", Qt::CaseInsensitive) == 0) {
            QColor c;
            if (parseColor(val, &c)) { sld->setHandleColor(c); return true; }
            if (error) *error = "Invalid color value";
            return false;
        }
        if (key.compare("borderColor", Qt::CaseInsensitive) == 0) {
            QColor c;
            if (parseColor(val, &c)) { sld->setBorderColor(c); return true; }
            if (error) *error = "Invalid color value";
            return false;
        }
        if (key.compare("borderWidth", Qt::CaseInsensitive) == 0) {
            sld->setBorderWidth(val.toInt());
            return true;
        }
        if (key.compare("cornerRadius", Qt::CaseInsensitive) == 0 || key.compare("radius", Qt::CaseInsensitive) == 0) {
            sld->setCornerRadius(val.toInt());
            return true;
        }
    } else if (auto sw = dynamic_cast<SwitchComponent*>(comp)) {
        if (key.compare("checked", Qt::CaseInsensitive) == 0) {
            sw->setChecked(val.toBool());
            return true;
        }
        if (key.compare("onColor", Qt::CaseInsensitive) == 0) {
            QColor c;
            if (parseColor(val, &c)) { sw->setOnColor(c); return true; }
            if (error) *error = "Invalid color value";
            return false;
        }
        if (key.compare("offColor", Qt::CaseInsensitive) == 0) {
            QColor c;
            if (parseColor(val, &c)) { sw->setOffColor(c); return true; }
            if (error) *error = "Invalid color value";
            return false;
        }
        if (key.compare("thumbColor", Qt::CaseInsensitive) == 0) {
            QColor c;
            if (parseColor(val, &c)) { sw->setThumbColor(c); return true; }
            if (error) *error = "Invalid color value";
            return false;
        }
        if (key.compare("borderColor", Qt::CaseInsensitive) == 0) {
            QColor c;
            if (parseColor(val, &c)) { sw->setBorderColor(c); return true; }
            if (error) *error = "Invalid color value";
            return false;
        }
        if (key.compare("borderWidth", Qt::CaseInsensitive) == 0) {
            sw->setBorderWidth(val.toInt());
            return true;
        }
        if (key.compare("cornerRadius", Qt::CaseInsensitive) == 0 || key.compare("radius", Qt::CaseInsensitive) == 0) {
            sw->setCornerRadius(val.toInt());
            return true;
        }
        if (key.compare("onToggled", Qt::CaseInsensitive) == 0) {
            sw->setOnToggledHandler(val.toString());
            return true;
        }
    } else if (auto chk = dynamic_cast<CheckboxComponent*>(comp)) {
        if (key.compare("text", Qt::CaseInsensitive) == 0) {
            chk->setText(val.toString());
            return true;
        }
        if (key.compare("checked", Qt::CaseInsensitive) == 0) {
            chk->setChecked(val.toBool());
            return true;
        }
        if (key.compare("textColor", Qt::CaseInsensitive) == 0) {
            QColor c;
            if (parseColor(val, &c)) { chk->setTextColor(c); return true; }
            if (error) *error = "Invalid color value";
            return false;
        }
        if (key.compare("checkColor", Qt::CaseInsensitive) == 0) {
            QColor c;
            if (parseColor(val, &c)) { chk->setCheckColor(c); return true; }
            if (error) *error = "Invalid color value";
            return false;
        }
        if (key.compare("boxColor", Qt::CaseInsensitive) == 0) {
            QColor c;
            if (parseColor(val, &c)) { chk->setBoxColor(c); return true; }
            if (error) *error = "Invalid color value";
            return false;
        }
        if (key.compare("borderColor", Qt::CaseInsensitive) == 0) {
            QColor c;
            if (parseColor(val, &c)) { chk->setBorderColor(c); return true; }
            if (error) *error = "Invalid color value";
            return false;
        }
        if (key.compare("borderWidth", Qt::CaseInsensitive) == 0) {
            chk->setBorderWidth(val.toInt());
            return true;
        }
        if (key.compare("fontFamily", Qt::CaseInsensitive) == 0 || key.compare("font", Qt::CaseInsensitive) == 0) {
            chk->setFontFamily(val.toString());
            return true;
        }
        if (key.compare("pixelSize", Qt::CaseInsensitive) == 0 || key.compare("fontSize", Qt::CaseInsensitive) == 0) {
            chk->setPixelSize(val.toInt());
            return true;
        }
        if (key.compare("onToggled", Qt::CaseInsensitive) == 0) {
            chk->setOnToggledHandler(val.toString());
            return true;
        }
    } else if (auto txt = dynamic_cast<TextInputComponent*>(comp)) {
        if (key.compare("text", Qt::CaseInsensitive) == 0) {
            txt->setText(val.toString());
            return true;
        }
        if (key.compare("placeholder", Qt::CaseInsensitive) == 0) {
            txt->setPlaceholder(val.toString());
            return true;
        }
        if (key.compare("textColor", Qt::CaseInsensitive) == 0) {
            QColor c;
            if (parseColor(val, &c)) { txt->setTextColor(c); return true; }
            if (error) *error = "Invalid color value";
            return false;
        }
        if (key.compare("placeholderColor", Qt::CaseInsensitive) == 0) {
            QColor c;
            if (parseColor(val, &c)) { txt->setPlaceholderColor(c); return true; }
            if (error) *error = "Invalid color value";
            return false;
        }
        if (key.compare("backgroundColor", Qt::CaseInsensitive) == 0 || key.compare("background", Qt::CaseInsensitive) == 0) {
            QColor c;
            if (parseColor(val, &c)) { txt->setBackgroundColor(c); return true; }
            if (error) *error = "Invalid color value";
            return false;
        }
        if (key.compare("borderColor", Qt::CaseInsensitive) == 0) {
            QColor c;
            if (parseColor(val, &c)) { txt->setBorderColor(c); return true; }
            if (error) *error = "Invalid color value";
            return false;
        }
        if (key.compare("borderWidth", Qt::CaseInsensitive) == 0) {
            txt->setBorderWidth(val.toInt());
            return true;
        }
        if (key.compare("cornerRadius", Qt::CaseInsensitive) == 0 || key.compare("radius", Qt::CaseInsensitive) == 0) {
            txt->setCornerRadius(val.toInt());
            return true;
        }
        if (key.compare("fontFamily", Qt::CaseInsensitive) == 0 || key.compare("font", Qt::CaseInsensitive) == 0) {
            txt->setFontFamily(val.toString());
            return true;
        }
        if (key.compare("pixelSize", Qt::CaseInsensitive) == 0 || key.compare("fontSize", Qt::CaseInsensitive) == 0) {
            txt->setPixelSize(val.toInt());
            return true;
        }
        if (key.compare("readOnly", Qt::CaseInsensitive) == 0) {
            txt->setReadOnly(val.toBool());
            return true;
        }
        if (key.compare("onTextChanged", Qt::CaseInsensitive) == 0) {
            txt->setOnTextChangedHandler(val.toString());
            return true;
        }
    } else if (auto img = dynamic_cast<ImageComponent*>(comp)) {
        if (key.compare("imagePath", Qt::CaseInsensitive) == 0 || key.compare("source", Qt::CaseInsensitive) == 0) {
            img->setImagePath(val.toString());
            return true;
        }
        if (key.compare("format", Qt::CaseInsensitive) == 0) {
            img->setFormat(val.toString());
            return true;
        }
        if (key.compare("opacity", Qt::CaseInsensitive) == 0) {
            img->setOpacityPercent(val.toInt());
            return true;
        }
        if (key.compare("scalingMode", Qt::CaseInsensitive) == 0) {
            img->setScalingMode(val.toString());
            return true;
        }
    } else if (auto circ = dynamic_cast<CircleComponent*>(comp)) {
        if (key.compare("fillColor", Qt::CaseInsensitive) == 0 || key.compare("fill", Qt::CaseInsensitive) == 0) {
            QColor c;
            if (parseColor(val, &c)) { circ->setFillColor(c); return true; }
            if (error) *error = "Invalid color value";
            return false;
        }
        if (key.compare("strokeColor", Qt::CaseInsensitive) == 0 || key.compare("stroke", Qt::CaseInsensitive) == 0) {
            QColor c;
            if (parseColor(val, &c)) { circ->setStrokeColor(c); return true; }
            if (error) *error = "Invalid color value";
            return false;
        }
        if (key.compare("strokeWidth", Qt::CaseInsensitive) == 0) {
            circ->setStrokeWidth(val.toInt());
            return true;
        }
        if (key.compare("filled", Qt::CaseInsensitive) == 0) {
            circ->setFilled(val.toBool(true));
            return true;
        }
    } else if (auto p = dynamic_cast<PathComponent*>(comp)) {
        if (key.compare("strokeColor", Qt::CaseInsensitive) == 0 || key.compare("stroke", Qt::CaseInsensitive) == 0) {
            QColor c;
            if (parseColor(val, &c)) { p->setStrokeColor(c); return true; }
            if (error) *error = "Invalid color value";
            return false;
        }
        if (key.compare("strokeWidth", Qt::CaseInsensitive) == 0 || key.compare("thickness", Qt::CaseInsensitive) == 0) {
            p->setStrokeThickness(val.toDouble());
            return true;
        }
        if (key.compare("opacity", Qt::CaseInsensitive) == 0) {
            p->setOpacityPercent(val.toInt());
            return true;
        }
    }

    if (error) *error = QString("Unknown or unsupported property '%1' for component type '%2'").arg(key, comp->componentType());
    return false;
}

bool ComponentSchemaRegistry::setComponentProperties(UIComponent* comp, const QJsonObject& props, QString* error) const {
    if (!comp) {
        if (error) *error = "Null component target";
        return false;
    }
    for (auto it = props.begin(); it != props.end(); ++it) {
        if (!setComponentProperty(comp, it.key(), it.value(), error)) {
            return false;
        }
    }
    return true;
}

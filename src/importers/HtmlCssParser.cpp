#include "HtmlCssParser.h"
#include <QRegularExpression>
#include <QDebug>

namespace Importers {

HtmlCssParser::HtmlCssParser(const QDomDocument& doc, const QString& css, ImportResult* result)
    : m_doc(doc)
    , m_css(css)
    , m_result(result)
{
    parseCssRules(css);
    extractStylesFromDocument(doc);
}

void HtmlCssParser::parseCssRules(const QString& css) {
    if (css.isEmpty()) return;

    // Remove comments
    QString cleanCss = css;
    cleanCss.remove(QRegularExpression("/\\*.*?\\*/", QRegularExpression::DotMatchesEverythingOption));

    // Match rules: selector { declarations }
    QRegularExpression ruleRegex("([^{]+)\\{([^}]+)\\}");
    QRegularExpressionMatchIterator it = ruleRegex.globalMatch(cleanCss);

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString selectors = match.captured(1).trimmed();
        QString declarations = match.captured(2).trimmed();

        QMap<QString, QString> props;
        QStringList declList = declarations.split(';', Qt::SkipEmptyParts);
        for (const QString& decl : declList) {
            int colonIdx = decl.indexOf(':');
            if (colonIdx > 0) {
                QString prop = decl.left(colonIdx).trimmed().toLower();
                QString val = decl.mid(colonIdx + 1).trimmed();
                props[prop] = val;
            }
        }

        QStringList selectorList = selectors.split(',', Qt::SkipEmptyParts);
        for (const QString& sel : selectorList) {
            QString cleanSel = sel.trimmed().toLower();
            for (auto propIt = props.constBegin(); propIt != props.constEnd(); ++propIt) {
                m_cssRules[cleanSel][propIt.key()] = propIt.value();
            }
        }
    }
}

void HtmlCssParser::extractStylesFromDocument(const QDomDocument& doc) {
    QDomNodeList styleNodes = doc.elementsByTagName("style");
    for (int i = 0; i < styleNodes.size(); ++i) {
        QDomElement styleElem = styleNodes.at(i).toElement();
        parseCssRules(styleElem.text());
    }
}

QMap<QString, QString> HtmlCssParser::getElementStyles(const QDomElement& elem) {
    QMap<QString, QString> styles;

    // 1. Tag name rule (e.g. "button")
    QString tag = elem.tagName().toLower();
    if (m_cssRules.contains(tag)) {
        styles.insert(m_cssRules[tag]);
    }

    // 2. Class rules (e.g. ".button")
    if (elem.hasAttribute("class")) {
        QStringList classes = elem.attribute("class").split(' ', Qt::SkipEmptyParts);
        for (const QString& cls : classes) {
            QString sel = "." + cls.toLower();
            if (m_cssRules.contains(sel)) {
                for (auto it = m_cssRules[sel].constBegin(); it != m_cssRules[sel].constEnd(); ++it) {
                    styles[it.key()] = it.value();
                }
            }
        }
    }

    // 3. ID rule (e.g. "#submit")
    if (elem.hasAttribute("id")) {
        QString sel = "#" + elem.attribute("id").toLower();
        if (m_cssRules.contains(sel)) {
            for (auto it = m_cssRules[sel].constBegin(); it != m_cssRules[sel].constEnd(); ++it) {
                styles[it.key()] = it.value();
            }
        }
    }

    // 4. Inline style attribute (highest priority)
    if (elem.hasAttribute("style")) {
        QString inlineStyle = elem.attribute("style");
        QStringList decls = inlineStyle.split(';', Qt::SkipEmptyParts);
        for (const QString& decl : decls) {
            int colonIdx = decl.indexOf(':');
            if (colonIdx > 0) {
                QString prop = decl.left(colonIdx).trimmed().toLower();
                QString val = decl.mid(colonIdx + 1).trimmed();
                styles[prop] = val;
            }
        }
    }

    // Check for unsupported CSS properties
    static const QStringList unsupportedProps = {
        "transform", "filter", "animation", "transition", "background-image", "clip-path"
    };
    for (const QString& unprop : unsupportedProps) {
        if (styles.contains(unprop) && m_result) {
            m_result->warnings.append(QString("Unsupported CSS property '%1' in <%2>").arg(unprop).arg(elem.tagName()));
        }
    }

    return styles;
}

QString HtmlCssParser::getCssProperty(const QDomElement& elem, const QString& property) {
    QMap<QString, QString> styles = getElementStyles(elem);
    return styles.value(property.toLower());
}

bool HtmlCssParser::isCssPropertySet(const QDomElement& elem, const QString& property) {
    QMap<QString, QString> styles = getElementStyles(elem);
    return styles.contains(property.toLower());
}

QColor HtmlCssParser::extractCssColor(const QString& colorStr) {
    if (colorStr.isEmpty()) return QColor();
    QString s = colorStr.trimmed().toLower();

    if (s == "transparent") return QColor(0, 0, 0, 0);
    if (s.startsWith("#")) return QColor(s);

    if (s.startsWith("rgb")) {
        QRegularExpression rgbRegex("rgba?\\s*\\(\\s*(\\d+)\\s*,\\s*(\\d+)\\s*,\\s*(\\d+)(?:\\s*,\\s*([\\d.]+))?\\s*\\)");
        QRegularExpressionMatch match = rgbRegex.match(s);
        if (match.hasMatch()) {
            int r = match.captured(1).toInt();
            int g = match.captured(2).toInt();
            int b = match.captured(3).toInt();
            int a = 255;
            if (!match.captured(4).isEmpty()) {
                double aVal = match.captured(4).toDouble();
                a = static_cast<int>(aVal * 255.0);
            }
            return QColor(r, g, b, a);
        }
    }

    QColor c = QColor::fromString(s);
    if (c.isValid()) return c;
    return QColor(s);
}

QSizeF HtmlCssParser::extractCssSize(const QString& sizeStr) {
    if (sizeStr.isEmpty()) return QSizeF();
    QString s = sizeStr.trimmed().toLower();
    s.remove("px");
    bool ok = false;
    double val = s.toDouble(&ok);
    if (ok) return QSizeF(val, val);
    return QSizeF();
}

IR::IRLayoutType HtmlCssParser::extractLayoutType(const QDomElement& elem) {
    QString disp = getCssProperty(elem, "display").toLower();
    if (disp == "flex") return IR::IRLayoutType::Flex;
    if (disp == "grid") return IR::IRLayoutType::Grid;
    if (disp == "none") return IR::IRLayoutType::Absolute;
    return IR::IRLayoutType::Absolute;
}

void HtmlCssParser::applyFlexboxProperties(IR::IRWidget& container, const QDomElement& elem) {
    container.layout.type = IR::IRLayoutType::Flex;

    QString dir = getCssProperty(elem, "flex-direction").toLower();
    if (dir == "column") container.layout.flexDirection = IR::IRFlexDirection::Column;
    else if (dir == "column-reverse") container.layout.flexDirection = IR::IRFlexDirection::ColumnReverse;
    else if (dir == "row-reverse") container.layout.flexDirection = IR::IRFlexDirection::RowReverse;
    else container.layout.flexDirection = IR::IRFlexDirection::Row;

    QString jc = getCssProperty(elem, "justify-content").toLower();
    if (jc == "center") container.layout.justifyContent = IR::IRJustifyContent::Center;
    else if (jc == "flex-end") container.layout.justifyContent = IR::IRJustifyContent::FlexEnd;
    else if (jc == "space-between") container.layout.justifyContent = IR::IRJustifyContent::SpaceBetween;
    else if (jc == "space-around") container.layout.justifyContent = IR::IRJustifyContent::SpaceAround;
    else if (jc == "space-evenly") container.layout.justifyContent = IR::IRJustifyContent::SpaceEvenly;
    else container.layout.justifyContent = IR::IRJustifyContent::FlexStart;

    QString ai = getCssProperty(elem, "align-items").toLower();
    if (ai == "flex-start") container.layout.alignItems = IR::IRAlignItems::FlexStart;
    else if (ai == "center") container.layout.alignItems = IR::IRAlignItems::Center;
    else if (ai == "flex-end") container.layout.alignItems = IR::IRAlignItems::FlexEnd;
    else if (ai == "baseline") container.layout.alignItems = IR::IRAlignItems::Baseline;
    else container.layout.alignItems = IR::IRAlignItems::Stretch;

    QString gapStr = getCssProperty(elem, "gap");
    if (!gapStr.isEmpty()) {
        gapStr.remove("px");
        container.layout.gap = gapStr.toDouble();
    }
}

void HtmlCssParser::applyGridProperties(IR::IRWidget& container, const QDomElement& elem) {
    container.layout.type = IR::IRLayoutType::Grid;

    QString cols = getCssProperty(elem, "grid-template-columns");
    if (!cols.isEmpty()) {
        container.layout.gridTemplateColumns = cols.split(' ', Qt::SkipEmptyParts);
        // Store computed columns count in properties
        int colCount = container.layout.gridTemplateColumns.size();
        if (cols.contains("repeat(")) {
            QRegularExpression repRegex("repeat\\s*\\(\\s*(\\d+)");
            auto match = repRegex.match(cols);
            if (match.hasMatch()) colCount = match.captured(1).toInt();
        }
        container.properties["columns"] = colCount;
    }

    QString rows = getCssProperty(elem, "grid-template-rows");
    if (!rows.isEmpty()) {
        container.layout.gridTemplateRows = rows.split(' ', Qt::SkipEmptyParts);
        int rowCount = container.layout.gridTemplateRows.size();
        if (rows.contains("repeat(")) {
            QRegularExpression repRegex("repeat\\s*\\(\\s*(\\d+)");
            auto match = repRegex.match(rows);
            if (match.hasMatch()) rowCount = match.captured(1).toInt();
        }
        container.properties["rows"] = rowCount;
    }

    QString gapStr = getCssProperty(elem, "gap");
    if (!gapStr.isEmpty()) {
        gapStr.remove("px");
        container.layout.gap = gapStr.toDouble();
    }
}

IR::IREffects HtmlCssParser::extractStyle(const QDomElement& elem) {
    IR::IREffects effects;

    QString bg = getCssProperty(elem, "background-color");
    if (bg.isEmpty()) bg = getCssProperty(elem, "background");
    if (!bg.isEmpty()) {
        effects.backgroundColor.type = IR::IRColor::Type::Solid;
        effects.backgroundColor.solidColor = extractCssColor(bg);
    }

    QString borderCol = getCssProperty(elem, "border-color");
    if (!borderCol.isEmpty()) {
        effects.border.color.type = IR::IRColor::Type::Solid;
        effects.border.color.solidColor = extractCssColor(borderCol);
    }

    QString borderW = getCssProperty(elem, "border-width");
    if (!borderW.isEmpty()) {
        borderW.remove("px");
        effects.border.width = borderW.toDouble();
    }

    QString radius = getCssProperty(elem, "border-radius");
    if (!radius.isEmpty()) {
        radius.remove("px");
        effects.borderRadius = radius.toInt();
    }

    QString op = getCssProperty(elem, "opacity");
    if (!op.isEmpty()) {
        effects.opacity = op.toDouble();
    }

    return effects;
}

QList<IR::IRWidget> HtmlCssParser::convertChildren(const QDomElement& parentElem) {
    QList<IR::IRWidget> list;
    QDomNode child = parentElem.firstChild();
    while (!child.isNull()) {
        if (child.isElement()) {
            QDomElement childElem = child.toElement();
            QString tag = childElem.tagName().toLower();
            if (tag == "style" || tag == "script") {
                child = child.nextSibling();
                continue;
            }

            if (tag == "div" || tag == "section" || tag == "article" || tag == "main" || tag == "nav" || tag == "header" || tag == "footer") {
                list.append(convertContainer(childElem));
            } else {
                list.append(convertElement(childElem));
            }
        }
        child = child.nextSibling();
    }
    return list;
}

IR::IRWidget HtmlCssParser::convertContainer(const QDomElement& elem) {
    IR::IRWidget container;
    container.id = elem.hasAttribute("id") ? IR::IRNodeId::fromString(elem.attribute("id")) : IR::IRNodeId::generate();
    container.componentType = "Container";
    container.name = elem.hasAttribute("id") ? elem.attribute("id") : "Container";
    container.effects = extractStyle(elem);

    // Layout
    IR::IRLayoutType lType = extractLayoutType(elem);
    if (lType == IR::IRLayoutType::Flex) {
        applyFlexboxProperties(container, elem);
    } else if (lType == IR::IRLayoutType::Grid) {
        applyGridProperties(container, elem);
    } else {
        container.layout.type = IR::IRLayoutType::Absolute;
    }

    // Sizing
    QString wStr = getCssProperty(elem, "width");
    if (wStr.isEmpty() && elem.hasAttribute("width")) wStr = elem.attribute("width");
    if (!wStr.isEmpty()) {
        wStr.remove("px");
        container.properties["width"] = wStr.toDouble();
    }

    QString hStr = getCssProperty(elem, "height");
    if (hStr.isEmpty() && elem.hasAttribute("height")) hStr = elem.attribute("height");
    if (!hStr.isEmpty()) {
        hStr.remove("px");
        container.properties["height"] = hStr.toDouble();
    }

    container.properties["isContainer"] = true;

    // Convert children recursively
    container.children = convertChildren(elem);

    return container;
}

IR::IRWidget HtmlCssParser::convertElement(const QDomElement& elem) {
    IR::IRWidget w;
    w.id = elem.hasAttribute("id") ? IR::IRNodeId::fromString(elem.attribute("id")) : IR::IRNodeId::generate();
    w.effects = extractStyle(elem);

    QString tag = elem.tagName().toLower();

    // Sizing & position
    QString wStr = getCssProperty(elem, "width");
    if (wStr.isEmpty() && elem.hasAttribute("width")) wStr = elem.attribute("width");
    if (!wStr.isEmpty()) {
        wStr.remove("px");
        w.properties["width"] = wStr.toDouble();
    }

    QString hStr = getCssProperty(elem, "height");
    if (hStr.isEmpty() && elem.hasAttribute("height")) hStr = elem.attribute("height");
    if (!hStr.isEmpty()) {
        hStr.remove("px");
        w.properties["height"] = hStr.toDouble();
    }

    QString colStr = getCssProperty(elem, "color");
    if (!colStr.isEmpty()) {
        w.textStyle.color.type = IR::IRColor::Type::Solid;
        w.textStyle.color.solidColor = extractCssColor(colStr);
    }

    QString fontSz = getCssProperty(elem, "font-size");
    if (!fontSz.isEmpty()) {
        fontSz.remove("px");
        fontSz.remove("pt");
        w.textStyle.fontSize = fontSz.toInt();
    }

    // Mapping per tag
    if (tag == "button" || (tag == "input" && (elem.attribute("type") == "button" || elem.attribute("type") == "submit"))) {
        w.componentType = "Button";
        w.label = elem.text().trimmed();
        if (w.label.isEmpty() && elem.hasAttribute("value")) w.label = elem.attribute("value");
    } else if (tag == "input" && elem.attribute("type") == "text") {
        w.componentType = "TextInput";
        w.label = elem.attribute("placeholder");
        if (w.label.isEmpty()) w.label = elem.attribute("value");
    } else if (tag == "textarea") {
        w.componentType = "TextInput";
        w.label = elem.attribute("placeholder");
        if (w.label.isEmpty()) w.label = elem.text().trimmed();
    } else if (tag == "img") {
        w.componentType = "Image";
        QString src = elem.attribute("src");
        w.properties["imagePath"] = src;
        w.properties["filePath"] = src;
        w.label = elem.attribute("alt");
    } else if (tag == "input" && elem.attribute("type") == "checkbox") {
        w.componentType = "Checkbox";
        w.label = elem.attribute("name");
        w.properties["checked"] = elem.hasAttribute("checked");
    } else if (tag == "input" && elem.attribute("type") == "radio") {
        w.componentType = "RadioButton";
        w.label = elem.attribute("name");
        w.properties["checked"] = elem.hasAttribute("checked");
    } else if (tag == "select") {
        w.componentType = "ComboBox";
        QDomNode opt = elem.firstChild();
        QStringList options;
        while (!opt.isNull()) {
            if (opt.isElement() && opt.toElement().tagName().toLower() == "option") {
                options.append(opt.toElement().text().trimmed());
            }
            opt = opt.nextSibling();
        }
        w.properties["options"] = QJsonArray::fromStringList(options);
    } else if (tag == "table") {
        w.componentType = "Table";
    } else if (tag == "label" || tag == "p" || tag == "span" || tag.startsWith("h")) {
        w.componentType = "Label";
        w.label = elem.text().trimmed();
    } else if (tag == "canvas" || tag == "video" || tag == "audio" || tag == "iframe") {
        w.componentType = "Label";
        w.label = QString("<%1>").arg(tag);
        if (m_result) {
            m_result->warnings.append(QString("Unsupported HTML element <%1> mapped to fallback Label").arg(tag));
        }
    } else {
        // Unknown element fallback
        w.componentType = "Label";
        w.label = elem.text().trimmed();
        if (w.label.isEmpty()) w.label = elem.tagName();
    }

    return w;
}

} // namespace Importers

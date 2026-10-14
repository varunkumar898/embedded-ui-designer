#pragma once

#include "ir/IRTypes.h"
#include "HtmlCssImporter.h"
#include <QDomDocument>
#include <QDomElement>
#include <QMap>
#include <QColor>
#include <QSizeF>
#include <vector>

namespace Importers {

class HtmlCssParser {
public:
    HtmlCssParser(const QDomDocument& doc, const QString& css, ImportResult* result = nullptr);

    IR::IRWidget convertElement(const QDomElement& elem);
    IR::IRWidget convertContainer(const QDomElement& elem);
    QList<IR::IRWidget> convertChildren(const QDomElement& parentElem);

    // CSS utilities
    QString getCssProperty(const QDomElement& elem, const QString& property);
    QColor extractCssColor(const QString& colorStr);
    QSizeF extractCssSize(const QString& sizeStr);
    bool isCssPropertySet(const QDomElement& elem, const QString& property);

    // Layout utilities
    IR::IRLayoutType extractLayoutType(const QDomElement& elem);
    void applyFlexboxProperties(IR::IRWidget& container, const QDomElement& elem);
    void applyGridProperties(IR::IRWidget& container, const QDomElement& elem);

    // Style utilities
    IR::IREffects extractStyle(const QDomElement& elem);

private:
    void parseCssRules(const QString& css);
    void extractStylesFromDocument(const QDomDocument& doc);
    QMap<QString, QString> getElementStyles(const QDomElement& elem);

    QDomDocument m_doc;
    QString m_css;
    ImportResult* m_result = nullptr;

    // selector -> { property: value }
    QMap<QString, QMap<QString, QString>> m_cssRules;
};

} // namespace Importers

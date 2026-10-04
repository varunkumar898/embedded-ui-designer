#pragma once

#include <QString>
#include <QStringList>
#include <QList>
#include <QMap>

class UIComponent;

struct QmlImportResult {
    bool success = false;
    QList<UIComponent*> components;
    QStringList rejectedItems; // Naming what was not imported and why
    QString errorMessage;

    int importedCount() const { return components.size(); }
    int rejectedCount() const { return rejectedItems.size(); }
};

class QmlImporter {
public:
    static QmlImportResult importFromFile(const QString& filePath);
    static QmlImportResult importFromString(const QString& qmlContent);

    static QString stripComments(const QString& content);
    static bool isNumericLiteral(const QString& s);
    static bool isPlainLiteral(const QString& rawValue, QString* outLiteral = nullptr);
};

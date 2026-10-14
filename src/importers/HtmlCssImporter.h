#pragma once

#include "ir/IRTypes.h"
#include <QString>
#include <QStringList>
#include <QDomDocument>

namespace Importers {

struct ImportResult {
    bool success = false;
    IR::IRProject ir;
    QStringList warnings;
    QStringList errors;
};

class HtmlCssImporter {
public:
    static ImportResult importHtml(const QString& filePath);

private:
    static IR::IRProject parseHtmlToIR(const QDomDocument& doc, const QString& css, ImportResult& result);
    static void recordWarning(ImportResult& result, const QString& message);
    static void recordError(ImportResult& result, const QString& message);
};

} // namespace Importers

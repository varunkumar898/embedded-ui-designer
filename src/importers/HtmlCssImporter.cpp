#include "HtmlCssImporter.h"
#include "HtmlCssParser.h"
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QDebug>

namespace Importers {

static QString prepareHtmlForDom(const QString& rawHtml) {
    QString html = rawHtml;

    // Self-close void tags for XML/QDomDocument compliance if not self-closed
    static const QStringList voidTags = { "input", "img", "br", "hr", "meta", "link" };
    for (const QString& tag : voidTags) {
        // Regex: <tag (attrs) [^/]> -> <tag \1 />
        QRegularExpression voidRegex(QString("<(%1)\\b([^>]*?)(?<!/)>").arg(tag), QRegularExpression::CaseInsensitiveOption);
        html.replace(voidRegex, "<\\1\\2 />");
    }

    return html;
}

ImportResult HtmlCssImporter::importHtml(const QString& filePath) {
    ImportResult result;

    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        recordError(result, QString("File not found: %1").arg(filePath));
        return result;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        recordError(result, QString("Could not open file for reading: %1").arg(filePath));
        return result;
    }

    QString content = QString::fromUtf8(file.readAll());
    file.close();

    if (content.trimmed().isEmpty()) {
        recordError(result, "HTML file is empty");
        return result;
    }

    // Prepare HTML for QDomDocument
    QString preparedHtml = prepareHtmlForDom(content);

    QDomDocument doc;
    QString errorMsg;
    int errorLine = 0;
    int errorCol = 0;

    bool ok = doc.setContent(preparedHtml, false, &errorMsg, &errorLine, &errorCol);
    if (!ok) {
        // Try wrapping fragment in <html><body>
        QString wrapped = "<html><body>" + preparedHtml + "</body></html>";
        ok = doc.setContent(wrapped, false, &errorMsg, &errorLine, &errorCol);
        if (!ok) {
            recordWarning(result, QString("HTML structure warning: %1 at line %2, col %3").arg(errorMsg).arg(errorLine).arg(errorCol));
            // Try extracting whatever elements we can or wrap aggressively
            // Fix unclosed tags by simple closing tag injection if needed
            QString cleaned = preparedHtml;
            cleaned.replace("<div>", "<div></div>");
            wrapped = "<html><body>" + cleaned + "</body></html>";
            if (!doc.setContent(wrapped, false, &errorMsg, &errorLine, &errorCol)) {
                // If still cannot parse, create minimal document with error warning
                recordWarning(result, QString("Malformed HTML could not be fully parsed: %1").arg(errorMsg));
                doc.setContent("<html><body><p>Malformed Content</p></body></html>");
            }
        }
    }

    // Extract styles
    QString css;
    QDomNodeList styleNodes = doc.elementsByTagName("style");
    for (int i = 0; i < styleNodes.size(); ++i) {
        css += styleNodes.at(i).toElement().text() + "\n";
    }

    // Parse DOM to IR
    result.ir = parseHtmlToIR(doc, css, result);
    result.success = true;

    return result;
}

IR::IRProject HtmlCssImporter::parseHtmlToIR(const QDomDocument& doc, const QString& css, ImportResult& result) {
    IR::IRProject project;
    project.version = IR::IRVersion();
    project.id = IR::IRNodeId::generate();
    project.name = "HtmlImportedProject";
    project.displayName = "Imported HTML UI";
    project.displayWidth = 480;
    project.displayHeight = 320;

    IR::IRScreen screen;
    screen.id = IR::IRNodeId::fromString("screen_main");
    screen.name = "MainScreen";
    screen.width = 480;
    screen.height = 320;
    screen.backgroundColor.type = IR::IRColor::Type::Solid;
    screen.backgroundColor.solidColor = QColor("#ffffff");

    HtmlCssParser parser(doc, css, &result);

    // Look for body or document element
    QDomElement rootElem = doc.documentElement();
    QDomElement contentElem = rootElem;

    QDomNodeList bodyList = doc.elementsByTagName("body");
    if (!bodyList.isEmpty()) {
        contentElem = bodyList.at(0).toElement();
    }

    // Convert children of body / root
    screen.widgets = parser.convertChildren(contentElem);

    project.screens.append(screen);
    project.activeScreenId = screen.id;

    return project;
}

void HtmlCssImporter::recordWarning(ImportResult& result, const QString& message) {
    result.warnings.append(message);
    qWarning() << "[HtmlCssImporter]" << message;
}

void HtmlCssImporter::recordError(ImportResult& result, const QString& message) {
    result.errors.append(message);
    result.success = false;
    qCritical() << "[HtmlCssImporter]" << message;
}

} // namespace Importers

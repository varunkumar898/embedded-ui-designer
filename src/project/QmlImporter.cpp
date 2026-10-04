#include "QmlImporter.h"

#include "../models/RectangleComponent.h"
#include "../models/LabelComponent.h"
#include "../models/ButtonComponent.h"
#include "../models/ImageComponent.h"
#include "../models/SliderComponent.h"
#include "../models/ProgressBarComponent.h"

#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>
#include <QDebug>

namespace {

struct ParsedNode {
    QString typeName;
    QString id;
    QMap<QString, QString> literalProperties;
    QStringList rejectedMessages;
    QList<ParsedNode> children;
};

// Skip whitespace and optional semicolons/commas
void skipWhitespace(const QString& s, int& pos) {
    while (pos < s.length()) {
        QChar c = s[pos];
        if (c.isSpace() || c == ';' || c == ',') {
            pos++;
        } else {
            break;
        }
    }
}

// Read balanced block enclosed by openChar and closeChar (e.g. '{' and '}', or '[' and ']')
QString readBalancedBlock(const QString& s, int& pos, QChar openChar, QChar closeChar) {
    if (pos >= s.length() || s[pos] != openChar) return QString();
    pos++; // skip openChar
    int depth = 1;
    int start = pos;
    bool inString = false;
    QChar quoteChar;

    while (pos < s.length() && depth > 0) {
        QChar c = s[pos];
        if (inString) {
            if (c == '\\' && pos + 1 < s.length()) {
                pos += 2;
                continue;
            }
            if (c == quoteChar) {
                inString = false;
            }
            pos++;
        } else {
            if (c == '"' || c == '\'') {
                inString = true;
                quoteChar = c;
                pos++;
            } else if (c == openChar) {
                depth++;
                pos++;
            } else if (c == closeChar) {
                depth--;
                pos++;
            } else {
                pos++;
            }
        }
    }

    int end = pos - 1;
    return s.mid(start, end - start);
}

// Read identifier or dotted property name
QString readIdentifier(const QString& s, int& pos) {
    skipWhitespace(s, pos);
    int start = pos;
    while (pos < s.length()) {
        QChar c = s[pos];
        if (c.isLetterOrNumber() || c == '_' || c == '.') {
            pos++;
        } else {
            break;
        }
    }
    return s.mid(start, pos - start);
}

// Read raw value expression until newline, semicolon, or enclosing delimiter
QString readRawValue(const QString& s, int& pos) {
    skipWhitespace(s, pos);
    int start = pos;
    bool inString = false;
    QChar quoteChar;
    int parenDepth = 0;

    while (pos < s.length()) {
        QChar c = s[pos];
        if (inString) {
            if (c == '\\' && pos + 1 < s.length()) {
                pos += 2;
                continue;
            }
            if (c == quoteChar) {
                inString = false;
            }
            pos++;
        } else {
            if (c == '"' || c == '\'') {
                inString = true;
                quoteChar = c;
                pos++;
            } else if (c == '(') {
                parenDepth++;
                pos++;
            } else if (c == ')') {
                if (parenDepth > 0) parenDepth--;
                pos++;
            } else if (parenDepth == 0 && (c == '\n' || c == '\r' || c == ';' || c == '}')) {
                break;
            } else {
                pos++;
            }
        }
    }

    return s.mid(start, pos - start).trimmed();
}

ParsedNode parseNodeBody(const QString& typeName, const QString& body, QStringList& fileRejections) {
    ParsedNode node;
    node.typeName = typeName;
    int pos = 0;

    while (pos < body.length()) {
        skipWhitespace(body, pos);
        if (pos >= body.length()) break;

        // Skip any standalone import statements
        if (body.mid(pos).startsWith("import ")) {
            while (pos < body.length() && body[pos] != '\n' && body[pos] != ';') pos++;
            continue;
        }

        QString token = readIdentifier(body, pos);
        if (token.isEmpty()) {
            pos++;
            continue;
        }

        skipWhitespace(body, pos);
        if (pos >= body.length()) break;

        QChar nextC = body[pos];
        if (nextC == '{') {
            // Child element declaration: TypeName { ... }
            QString childBody = readBalancedBlock(body, pos, '{', '}');
            ParsedNode childNode = parseNodeBody(token, childBody, fileRejections);
            node.children.append(childNode);
        } else if (nextC == ':') {
            pos++; // skip ':'
            skipWhitespace(body, pos);
            if (pos >= body.length()) break;

            if (body[pos] == '{') {
                // Object or script block: e.g. onClicked: { ... }
                QString block = readBalancedBlock(body, pos, '{', '}');
                if (token.startsWith("on") || token == "function") {
                    node.rejectedMessages.append(QString("Rejected script/handler on %1: '%2' (JavaScript and signal handlers are not supported)").arg(typeName, token));
                } else {
                    node.rejectedMessages.append(QString("Rejected complex property object on %1: '%2' (inline object blocks are not supported)").arg(typeName, token));
                }
            } else if (body[pos] == '[') {
                // Array block: e.g. states: [ ... ] or transitions: [ ... ]
                QString block = readBalancedBlock(body, pos, '[', ']');
                if (token == "states") {
                    node.rejectedMessages.append(QString("Rejected block on %1: 'states' (states and transitions are not supported)").arg(typeName));
                } else if (token == "transitions") {
                    node.rejectedMessages.append(QString("Rejected block on %1: 'transitions' (states and transitions are not supported)").arg(typeName));
                } else {
                    node.rejectedMessages.append(QString("Rejected array property on %1: '%2' (array bindings are not supported)").arg(typeName, token));
                }
            } else {
                // Property value
                QString rawVal = readRawValue(body, pos);
                if (token == "id") {
                    node.id = rawVal;
                } else if (token.startsWith("anchors.") || token == "anchors") {
                    node.rejectedMessages.append(QString("Rejected anchor on %1: '%2: %3' (anchors not supported, only literal x/y/width/height are supported)").arg(typeName, token, rawVal));
                } else if (token.startsWith("on") || rawVal.contains("=>") || rawVal.startsWith("function")) {
                    node.rejectedMessages.append(QString("Rejected script/handler on %1: '%2' (JavaScript and signal handlers are not supported)").arg(typeName, token));
                } else {
                    QString literalVal;
                    if (QmlImporter::isPlainLiteral(rawVal, &literalVal)) {
                        node.literalProperties[token] = literalVal;
                    } else {
                        node.rejectedMessages.append(QString("Rejected property binding on %1: '%2: %3' (dynamic expressions, id references, and JS bindings are not supported; literal values only)").arg(typeName, token, rawVal));
                    }
                }
            }
        }
    }

    return node;
}

} // namespace

QString QmlImporter::stripComments(const QString& content) {
    QString out;
    out.reserve(content.size());
    int i = 0;
    int len = content.size();
    bool inString = false;
    QChar quoteChar;

    while (i < len) {
        QChar c = content[i];
        if (inString) {
            out.append(c);
            if (c == '\\' && i + 1 < len) {
                out.append(content[i + 1]);
                i += 2;
                continue;
            }
            if (c == quoteChar) {
                inString = false;
            }
            i++;
        } else {
            if (c == '"' || c == '\'') {
                inString = true;
                quoteChar = c;
                out.append(c);
                i++;
            } else if (c == '/' && i + 1 < len && content[i + 1] == '/') {
                i += 2;
                while (i < len && content[i] != '\n') {
                    i++;
                }
            } else if (c == '/' && i + 1 < len && content[i + 1] == '*') {
                i += 2;
                while (i + 1 < len && !(content[i] == '*' && content[i + 1] == '/')) {
                    i++;
                }
                if (i + 1 < len) i += 2;
            } else {
                out.append(c);
                i++;
            }
        }
    }
    return out;
}

bool QmlImporter::isNumericLiteral(const QString& s) {
    if (s.isEmpty()) return false;
    bool ok = false;
    s.toDouble(&ok);
    if (!ok) return false;
    for (int i = 0; i < s.length(); ++i) {
        QChar c = s[i];
        if (i == 0 && (c == '+' || c == '-')) continue;
        if (!c.isDigit() && c != '.') return false;
    }
    return true;
}

bool QmlImporter::isPlainLiteral(const QString& rawValue, QString* outLiteral) {
    QString s = rawValue.trimmed();
    if (s.isEmpty()) return false;

    // 1. Quoted string
    if ((s.startsWith('"') && s.endsWith('"') && s.length() >= 2) ||
        (s.startsWith('\'') && s.endsWith('\'') && s.length() >= 2)) {
        if (outLiteral) *outLiteral = s.mid(1, s.length() - 2);
        return true;
    }

    // 2. Numeric literal
    if (isNumericLiteral(s)) {
        if (outLiteral) *outLiteral = s;
        return true;
    }

    // 3. Boolean literal
    if (s == "true" || s == "false") {
        if (outLiteral) *outLiteral = s;
        return true;
    }

    // 4. Alignment enum literals
    if (s.startsWith("Text.Align")) {
        if (outLiteral) *outLiteral = s;
        return true;
    }

    return false;
}

QmlImportResult QmlImporter::importFromFile(const QString& filePath) {
    QmlImportResult result;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        result.errorMessage = QString("Could not open file: %1").arg(filePath);
        return result;
    }
    const QString content = QString::fromUtf8(file.readAll());
    return importFromString(content);
}

QmlImportResult QmlImporter::importFromString(const QString& qmlContent) {
    QmlImportResult result;
    QString cleanContent = stripComments(qmlContent);

    // Parse top-level nodes
    QStringList globalRejections;
    int pos = 0;
    QList<ParsedNode> topNodes;

    while (pos < cleanContent.length()) {
        skipWhitespace(cleanContent, pos);
        if (pos >= cleanContent.length()) break;

        if (cleanContent.mid(pos).startsWith("import ")) {
            while (pos < cleanContent.length() && cleanContent[pos] != '\n' && cleanContent[pos] != ';') pos++;
            continue;
        }

        QString typeName = readIdentifier(cleanContent, pos);
        if (typeName.isEmpty()) {
            pos++;
            continue;
        }

        skipWhitespace(cleanContent, pos);
        if (pos < cleanContent.length() && cleanContent[pos] == '{') {
            QString body = readBalancedBlock(cleanContent, pos, '{', '}');
            ParsedNode rootNode = parseNodeBody(typeName, body, globalRejections);
            topNodes.append(rootNode);
        }
    }

    result.rejectedItems.append(globalRejections);

    // Recognized types set
    const QSet<QString> supportedTypes = {
        "Rectangle", "Text", "Button", "Image", "Slider", "ProgressBar"
    };

    // Recursive helper to traverse ParsedNodes and instantiate UIComponents
    int autoIdSeq = 1;
    auto processNode = [&](auto self, const ParsedNode& node) -> void {
        const QString& type = node.typeName;
        QString compId = node.id.isEmpty() ? QString("%1_%2").arg(type.toLower()).arg(autoIdSeq++) : node.id;

        UIComponent* comp = nullptr;
        if (supportedTypes.contains(type)) {
            if (type == "Rectangle") {
                auto* rect = new RectangleComponent(compId);
                const auto& p = node.literalProperties;
                if (p.contains("color")) rect->setFillColor(QColor(p["color"]));
                if (p.contains("border.color")) rect->setStrokeColor(QColor(p["border.color"]));
                if (p.contains("border.width")) rect->setStrokeWidth(p["border.width"].toInt());
                if (p.contains("radius")) rect->setCornerRadius(p["radius"].toInt());
                comp = rect;
            } else if (type == "Text") {
                auto* lbl = new LabelComponent(compId);
                const auto& p = node.literalProperties;
                if (p.contains("text")) lbl->setText(p["text"]);
                if (p.contains("color")) lbl->setColor(QColor(p["color"]));
                if (p.contains("font.family")) lbl->setFontFamily(p["font.family"]);
                if (p.contains("font.pixelSize")) lbl->setPixelSize(p["font.pixelSize"].toInt());
                else if (p.contains("font.pointSize")) lbl->setPixelSize(p["font.pointSize"].toInt());
                if (p.contains("font.bold")) lbl->setBold(p["font.bold"] == "true");
                if (p.contains("font.italic")) lbl->setItalic(p["font.italic"] == "true");
                if (p.contains("horizontalAlignment")) {
                    QString a = p["horizontalAlignment"];
                    if (a.contains("AlignHCenter") || a == "Center") lbl->setAlignment(Qt::AlignHCenter);
                    else if (a.contains("AlignRight") || a == "Right") lbl->setAlignment(Qt::AlignRight);
                    else if (a.contains("AlignLeft") || a == "Left") lbl->setAlignment(Qt::AlignLeft);
                }
                comp = lbl;
            } else if (type == "Button") {
                auto* btn = new ButtonComponent(compId);
                const auto& p = node.literalProperties;
                if (p.contains("text")) btn->setText(p["text"]);
                if (p.contains("color") || p.contains("backgroundColor")) btn->setBackgroundColor(QColor(p.contains("color") ? p["color"] : p["backgroundColor"]));
                if (p.contains("textColor")) btn->setTextColor(QColor(p["textColor"]));
                if (p.contains("radius")) btn->setCornerRadius(p["radius"].toInt());
                comp = btn;
            } else if (type == "Image") {
                auto* img = new ImageComponent(compId);
                const auto& p = node.literalProperties;
                if (p.contains("source")) img->setImagePath(p["source"]);
                comp = img;
            } else if (type == "Slider") {
                auto* sld = new SliderComponent(compId);
                const auto& p = node.literalProperties;
                if (p.contains("from")) sld->setMinimum(p["from"].toInt());
                if (p.contains("to")) sld->setMaximum(p["to"].toInt());
                if (p.contains("value")) sld->setValue(p["value"].toInt());
                comp = sld;
            } else if (type == "ProgressBar") {
                auto* bar = new ProgressBarComponent(compId);
                const auto& p = node.literalProperties;
                if (p.contains("value")) {
                    double v = p["value"].toDouble();
                    if (v > 1.0 && p.contains("to")) {
                        double toVal = p["to"].toDouble();
                        if (toVal > 0.0) v = v / toVal;
                    }
                    bar->setValue(v);
                }
                comp = bar;
            }

            if (comp) {
                // Common geometry properties
                const auto& p = node.literalProperties;
                qreal x = p.contains("x") ? p["x"].toDouble() : comp->compX();
                qreal y = p.contains("y") ? p["y"].toDouble() : comp->compY();
                qreal w = p.contains("width") ? p["width"].toDouble() : comp->compWidth();
                qreal h = p.contains("height") ? p["height"].toDouble() : comp->compHeight();
                comp->setCompPos(x, y);
                comp->setCompSize(w, h);
                result.components.append(comp);
            }
        } else {
            // Explicitly rejected element type
            if (type == "Loader" || type == "Repeater") {
                result.rejectedItems.append(QString("Rejected element: '%1' (Loader/Repeater are not supported)").arg(type));
            } else {
                result.rejectedItems.append(QString("Rejected element: '%1' (unrecognized/unsupported QML type; only Rectangle, Text, Button, Image, Slider, ProgressBar are supported)").arg(type));
            }
        }

        // Collect rejected messages recorded inside this node
        for (const QString& msg : node.rejectedMessages) {
            result.rejectedItems.append(msg);
        }

        // Traverse children
        for (const ParsedNode& child : node.children) {
            self(self, child);
        }
    };

    for (const ParsedNode& rootNode : topNodes) {
        processNode(processNode, rootNode);
    }

    result.success = (!result.components.isEmpty() || !result.rejectedItems.isEmpty());
    return result;
}

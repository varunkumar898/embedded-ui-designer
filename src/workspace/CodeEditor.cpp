#include "CodeEditor.h"
#include "CppSyntaxHighlighter.h"
#include "theme/Theme.h"

#include <QPainter>
#include <QTextBlock>
#include <QFontDatabase>
#include <QFileInfo>
#include <QKeyEvent>

CodeEditor::CodeEditor(QWidget *parent)
    : QPlainTextEdit(parent)
{
    m_lineNumberArea = new LineNumberArea(this);

    // Font
    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setPointSize(11);
    font.setStyleHint(QFont::Monospace);
    setFont(font);

    // Styling
    setStyleSheet(QString(
        "QPlainTextEdit { "
        "  background-color: %1; "
        "  color: %2; "
        "  border: none; "
        "  selection-background-color: %3; "
        "  selection-color: %4; "
        "} "
    ).arg(Theme::BgBase, Theme::TextPrimary, Theme::AccentSoft, Theme::AccentText));

    setLineWrapMode(QPlainTextEdit::NoWrap);
    setTabStopDistance(fontMetrics().horizontalAdvance(' ') * 4);

    connect(this, &CodeEditor::blockCountChanged, this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &CodeEditor::updateRequest, this, &CodeEditor::updateLineNumberArea);
    connect(this, &CodeEditor::cursorPositionChanged, this, &CodeEditor::highlightCurrentLine);
    connect(this, &CodeEditor::cursorPositionChanged, this, &CodeEditor::onCursorPositionChanged);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();

    m_highlighter = new CppSyntaxHighlighter(document(), CppSyntaxHighlighter::FileType::Cpp);
}

CodeEditor::~CodeEditor() {
}

int CodeEditor::lineNumberAreaWidth() {
    int digits = 1;
    int maxLines = qMax(1, blockCount());
    while (maxLines >= 10) {
        maxLines /= 10;
        ++digits;
    }
    digits = qMax(digits, 3); // minimum 3 digits width for a clean look

    int space = 18 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    return space;
}

void CodeEditor::updateLineNumberAreaWidth(int /* newBlockCount */) {
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy) {
    if (dy) {
        m_lineNumberArea->scroll(0, dy);
    } else {
        m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());
    }

    if (rect.contains(viewport()->rect())) {
        updateLineNumberAreaWidth(0);
    }
}

void CodeEditor::resizeEvent(QResizeEvent *e) {
    QPlainTextEdit::resizeEvent(e);
    QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void CodeEditor::highlightCurrentLine() {
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;
        QColor lineColor = QColor(Theme::BgRaised);
        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event) {
    QPainter painter(m_lineNumberArea);
    painter.fillRect(event->rect(), QColor(Theme::BgSurface));

    // Right border line for line number area
    painter.setPen(QColor(Theme::BorderSubtle));
    painter.drawLine(event->rect().topRight(), event->rect().bottomRight());

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    int currentLine = textCursor().blockNumber();

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);

            if (blockNumber == currentLine) {
                painter.setPen(QColor(Theme::TextPrimary));
                QFont boldFont = font();
                boldFont.setBold(true);
                painter.setFont(boldFont);
            } else {
                painter.setPen(QColor(Theme::TextMuted));
                painter.setFont(font());
            }

            painter.drawText(0, top, m_lineNumberArea->width() - 8, fontMetrics().height(),
                             Qt::AlignRight | Qt::AlignVCenter, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

void CodeEditor::setFilePath(const QString& path) {
    m_filePath = path;
    setLanguageByFileName(QFileInfo(path).fileName());
}

void CodeEditor::setLanguageByFileName(const QString& fileName) {
    QString lower = fileName.toLower();
    if (lower.endsWith(".c") || lower.endsWith(".cpp") || lower.endsWith(".cc") ||
        lower.endsWith(".h") || lower.endsWith(".hpp")) {
        m_language = "C / C++";
        if (m_highlighter) m_highlighter->setFileType(CppSyntaxHighlighter::FileType::Cpp);
    } else if (lower == "cmakelists.txt" || lower.endsWith(".cmake")) {
        m_language = "CMake";
        if (m_highlighter) m_highlighter->setFileType(CppSyntaxHighlighter::FileType::CMake);
    } else if (lower == "makefile" || lower.endsWith(".mk")) {
        m_language = "Makefile";
        if (m_highlighter) m_highlighter->setFileType(CppSyntaxHighlighter::FileType::Makefile);
    } else {
        m_language = "Text";
        if (m_highlighter) m_highlighter->setFileType(CppSyntaxHighlighter::FileType::Generic);
    }
}

QString CodeEditor::detectedLanguage() const {
    return m_language;
}

void CodeEditor::gotoLine(int line, int col) {
    QTextCursor cursor(document()->findBlockByLineNumber(qMax(0, line - 1)));
    int pos = cursor.position() + qMax(0, col - 1);
    cursor.setPosition(qMin(pos, document()->characterCount() - 1));
    setTextCursor(cursor);
    centerCursor();
    setFocus();
}

void CodeEditor::onCursorPositionChanged() {
    QTextCursor cursor = textCursor();
    int line = cursor.blockNumber() + 1;
    int col = cursor.columnNumber() + 1;
    emit lineColChanged(line, col);
}

void CodeEditor::keyPressEvent(QKeyEvent *e) {
    if (e->key() == Qt::Key_Tab && !e->modifiers()) {
        insertPlainText("    "); // 4 spaces
        return;
    }
    QPlainTextEdit::keyPressEvent(e);
}

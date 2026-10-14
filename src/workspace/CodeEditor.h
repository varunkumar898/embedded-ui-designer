#pragma once

#include <QPlainTextEdit>
#include <QWidget>

class LineNumberArea;
class CppSyntaxHighlighter;

class CodeEditor : public QPlainTextEdit {
    Q_OBJECT

public:
    explicit CodeEditor(QWidget *parent = nullptr);
    ~CodeEditor() override;

    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth();

    QString filePath() const { return m_filePath; }
    void setFilePath(const QString& path);

    void setLanguageByFileName(const QString& fileName);
    QString detectedLanguage() const;

    void gotoLine(int line, int col = 1);

signals:
    void lineColChanged(int line, int col);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *e) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &rect, int dy);
    void onCursorPositionChanged();

private:
    QWidget *m_lineNumberArea = nullptr;
    CppSyntaxHighlighter *m_highlighter = nullptr;
    QString m_filePath;
    QString m_language = "C/C++";
};

class LineNumberArea : public QWidget {
public:
    explicit LineNumberArea(CodeEditor *editor) : QWidget(editor), m_codeEditor(editor) {}

    QSize sizeHint() const override {
        return QSize(m_codeEditor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        m_codeEditor->lineNumberAreaPaintEvent(event);
    }

private:
    CodeEditor *m_codeEditor;
};

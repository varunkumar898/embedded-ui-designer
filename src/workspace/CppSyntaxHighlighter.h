#pragma once

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>
#include <QVector>

class CppSyntaxHighlighter : public QSyntaxHighlighter {
    Q_OBJECT

public:
    enum class FileType {
        Cpp,
        CMake,
        Makefile,
        Generic
    };

    explicit CppSyntaxHighlighter(QTextDocument* parent = nullptr, FileType type = FileType::Cpp);

    void setFileType(FileType type);

protected:
    void highlightBlock(const QString& text) override;

private:
    struct HighlightingRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };

    void setupCppRules();
    void setupCMakeRules();
    void setupMakefileRules();

    FileType m_fileType;
    QVector<HighlightingRule> m_highlightingRules;

    QRegularExpression m_commentStartExpression;
    QRegularExpression m_commentEndExpression;

    QTextCharFormat m_keywordFormat;
    QTextCharFormat m_typeFormat;
    QTextCharFormat m_preprocessorFormat;
    QTextCharFormat m_singleLineCommentFormat;
    QTextCharFormat m_multiLineCommentFormat;
    QTextCharFormat m_quotationFormat;
    QTextCharFormat m_functionFormat;
    QTextCharFormat m_numberFormat;
};

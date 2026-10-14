#include "CppSyntaxHighlighter.h"

CppSyntaxHighlighter::CppSyntaxHighlighter(QTextDocument* parent, FileType type)
    : QSyntaxHighlighter(parent)
    , m_fileType(type)
{
    setFileType(type);
}

void CppSyntaxHighlighter::setFileType(FileType type) {
    m_fileType = type;
    m_highlightingRules.clear();

    switch (type) {
    case FileType::Cpp:
        setupCppRules();
        break;
    case FileType::CMake:
        setupCMakeRules();
        break;
    case FileType::Makefile:
        setupMakefileRules();
        break;
    case FileType::Generic:
    default:
        setupCppRules();
        break;
    }

    rehighlight();
}

void CppSyntaxHighlighter::setupCppRules() {
    HighlightingRule rule;

    // Keywords
    m_keywordFormat.setForeground(QColor("#569cd6")); // VSCode Blue
    m_keywordFormat.setFontWeight(QFont::Bold);
    const QString keywordPatterns[] = {
        QStringLiteral("\\bchar\\b"), QStringLiteral("\\bclass\\b"), QStringLiteral("\\bconst\\b"),
        QStringLiteral("\\bdouble\\b"), QStringLiteral("\\benum\\b"), QStringLiteral("\\bexplicit\\b"),
        QStringLiteral("\\bfriend\\b"), QStringLiteral("\\binline\\b"), QStringLiteral("\\bint\\b"),
        QStringLiteral("\\blong\\b"), QStringLiteral("\\bnamespace\\b"), QStringLiteral("\\boperator\\b"),
        QStringLiteral("\\bshort\\b"), QStringLiteral("\\bsignals\\b"), QStringLiteral("\\bsigned\\b"),
        QStringLiteral("\\bslots\\b"), QStringLiteral("\\bstatic\\b"), QStringLiteral("\\bstruct\\b"),
        QStringLiteral("\\btemplate\\b"), QStringLiteral("\\btypedef\\b"), QStringLiteral("\\btypename\\b"),
        QStringLiteral("\\bunion\\b"), QStringLiteral("\\bunsigned\\b"), QStringLiteral("\\bvirtual\\b"),
        QStringLiteral("\\bvoid\\b"), QStringLiteral("\\bvolatile\\b"), QStringLiteral("\\bbool\\b"),
        QStringLiteral("\\bif\\b"), QStringLiteral("\\belse\\b"), QStringLiteral("\\bwhile\\b"),
        QStringLiteral("\\bfor\\b"), QStringLiteral("\\bdo\\b"), QStringLiteral("\\bswitch\\b"),
        QStringLiteral("\\bcase\\b"), QStringLiteral("\\bdefault\\b"), QStringLiteral("\\bbreak\\b"),
        QStringLiteral("\\bcontinue\\b"), QStringLiteral("\\breturn\\b"), QStringLiteral("\\bgoto\\b"),
        QStringLiteral("\\btrue\\b"), QStringLiteral("\\bfalse\\b"), QStringLiteral("\\bnullptr\\b"),
        QStringLiteral("\\bNULL\\b"), QStringLiteral("\\bconstexpr\\b"), QStringLiteral("\\bauto\\b"),
        QStringLiteral("\\bextern\\b"), QStringLiteral("\\basm\\b"), QStringLiteral("\\bsizeof\\b")
    };
    for (const QString& pattern : keywordPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = m_keywordFormat;
        m_highlightingRules.append(rule);
    }

    // Fixed-width integer and standard embedded types
    m_typeFormat.setForeground(QColor("#4ec9b0")); // Teal
    const QString typePatterns[] = {
        QStringLiteral("\\bu?int(8|16|32|64)_t\\b"),
        QStringLiteral("\\bsize_t\\b"), QStringLiteral("\\bu?intptr_t\\b"),
        QStringLiteral("\\bfloat\\b"), QStringLiteral("\\bdouble\\b"),
        QStringLiteral("\\bGPIO_TypeDef\\b"), QStringLiteral("\\bUSART_TypeDef\\b"),
        QStringLiteral("\\bSPI_TypeDef\\b"), QStringLiteral("\\bI2C_TypeDef\\b"),
        QStringLiteral("\\bHAL_StatusTypeDef\\b"), QStringLiteral("\\blv_obj_t\\b"),
        QStringLiteral("\\blv_color_t\\b"), QStringLiteral("\\bcoord_t\\b")
    };
    for (const QString& pattern : typePatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = m_typeFormat;
        m_highlightingRules.append(rule);
    }

    // Preprocessor directives
    m_preprocessorFormat.setForeground(QColor("#c586c0")); // Purple
    m_preprocessorFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression(QStringLiteral("^\\s*#[a-zA-Z_]+"));
    rule.format = m_preprocessorFormat;
    m_highlightingRules.append(rule);

    // Functions
    m_functionFormat.setForeground(QColor("#dcdcaa")); // Light Yellow
    rule.pattern = QRegularExpression(QStringLiteral("\\b[A-Za-z0-9_]+(?=\\()"));
    rule.format = m_functionFormat;
    m_highlightingRules.append(rule);

    // Numbers (dec, hex, bin)
    m_numberFormat.setForeground(QColor("#b5cea8")); // Light Green
    rule.pattern = QRegularExpression(QStringLiteral("\\b(0x[0-9a-fA-F]+|0b[01]+|[0-9]+(\\.[0-9]+)?f?)\\b"));
    rule.format = m_numberFormat;
    m_highlightingRules.append(rule);

    // Strings
    m_quotationFormat.setForeground(QColor("#ce9178")); // Coral/Orange
    rule.pattern = QRegularExpression(QStringLiteral("\".*?\""));
    rule.format = m_quotationFormat;
    m_highlightingRules.append(rule);

    // Characters
    rule.pattern = QRegularExpression(QStringLiteral("'.*?'"));
    rule.format = m_quotationFormat;
    m_highlightingRules.append(rule);

    // Single-line comments
    m_singleLineCommentFormat.setForeground(QColor("#6a9955")); // Forest Green
    m_singleLineCommentFormat.setFontItalic(true);
    rule.pattern = QRegularExpression(QStringLiteral("//[^\n]*"));
    rule.format = m_singleLineCommentFormat;
    m_highlightingRules.append(rule);

    // Multi-line comments
    m_multiLineCommentFormat.setForeground(QColor("#6a9955"));
    m_multiLineCommentFormat.setFontItalic(true);
    m_commentStartExpression = QRegularExpression(QStringLiteral("/\\*"));
    m_commentEndExpression = QRegularExpression(QStringLiteral("\\*/"));
}

void CppSyntaxHighlighter::setupCMakeRules() {
    HighlightingRule rule;

    m_keywordFormat.setForeground(QColor("#569cd6"));
    m_keywordFormat.setFontWeight(QFont::Bold);
    const QString cmakeKeywords[] = {
        QStringLiteral("\\bcmake_minimum_required\\b"), QStringLiteral("\\bproject\\b"),
        QStringLiteral("\\bset\\b"), QStringLiteral("\\badd_executable\\b"),
        QStringLiteral("\\badd_library\\b"), QStringLiteral("\\btarget_link_libraries\\b"),
        QStringLiteral("\\btarget_include_directories\\b"), QStringLiteral("\\btarget_compile_options\\b"),
        QStringLiteral("\\btarget_compile_definitions\\b"), QStringLiteral("\\bfind_package\\b"),
        QStringLiteral("\\binclude_directories\\b"), QStringLiteral("\\blink_directories\\b"),
        QStringLiteral("\\bif\\b"), QStringLiteral("\\belse\\b"), QStringLiteral("\\belseif\\b"),
        QStringLiteral("\\bendif\\b"), QStringLiteral("\\bforeach\\b"), QStringLiteral("\\bendforeach\\b"),
        QStringLiteral("\\bwhile\\b"), QStringLiteral("\\bendwhile\\b"), QStringLiteral("\\bfunction\\b"),
        QStringLiteral("\\bendfunction\\b"), QStringLiteral("\\bmacro\\b"), QStringLiteral("\\bendmacro\\b"),
        QStringLiteral("\\bmessage\\b"), QStringLiteral("\\boption\\b"), QStringLiteral("\\bconfigure_file\\b"),
        QStringLiteral("\\binstall\\b"), QStringLiteral("\\binclude\\b")
    };
    for (const QString& kw : cmakeKeywords) {
        rule.pattern = QRegularExpression(kw, QRegularExpression::CaseInsensitiveOption);
        rule.format = m_keywordFormat;
        m_highlightingRules.append(rule);
    }

    // Variables ${VAR}
    m_typeFormat.setForeground(QColor("#4ec9b0"));
    rule.pattern = QRegularExpression(QStringLiteral("\\$\\{[a-zA-Z0-9_]+\\}"));
    rule.format = m_typeFormat;
    m_highlightingRules.append(rule);

    // Strings
    m_quotationFormat.setForeground(QColor("#ce9178"));
    rule.pattern = QRegularExpression(QStringLiteral("\".*?\""));
    rule.format = m_quotationFormat;
    m_highlightingRules.append(rule);

    // Comments
    m_singleLineCommentFormat.setForeground(QColor("#6a9955"));
    m_singleLineCommentFormat.setFontItalic(true);
    rule.pattern = QRegularExpression(QStringLiteral("#[^\n]*"));
    rule.format = m_singleLineCommentFormat;
    m_highlightingRules.append(rule);
}

void CppSyntaxHighlighter::setupMakefileRules() {
    HighlightingRule rule;

    // Targets and directives
    m_keywordFormat.setForeground(QColor("#569cd6"));
    m_keywordFormat.setFontWeight(QFont::Bold);
    const QString makeKeywords[] = {
        QStringLiteral("\\bifeq\\b"), QStringLiteral("\\bifneq\\b"), QStringLiteral("\\bifdef\\b"),
        QStringLiteral("\\bifndef\\b"), QStringLiteral("\\belse\\b"), QStringLiteral("\\bendif\\b"),
        QStringLiteral("\\binclude\\b"), QStringLiteral("\\b-include\\b"), QStringLiteral("\\bsinclude\\b"),
        QStringLiteral("\\boverride\\b"), QStringLiteral("\\bexport\\b"), QStringLiteral("\\bunexport\\b"),
        QStringLiteral("\\bdefine\\b"), QStringLiteral("\\bendef\\b"), QStringLiteral("\\.PHONY\\b")
    };
    for (const QString& kw : makeKeywords) {
        rule.pattern = QRegularExpression(kw);
        rule.format = m_keywordFormat;
        m_highlightingRules.append(rule);
    }

    // Variables $(VAR) or ${VAR}
    m_typeFormat.setForeground(QColor("#4ec9b0"));
    rule.pattern = QRegularExpression(QStringLiteral("(\\$[a-zA-Z0-9_]|\\$\\([a-zA-Z0-9_]+\\)|\\$\\{[a-zA-Z0-9_]+\\})"));
    rule.format = m_typeFormat;
    m_highlightingRules.append(rule);

    // Comments
    m_singleLineCommentFormat.setForeground(QColor("#6a9955"));
    m_singleLineCommentFormat.setFontItalic(true);
    rule.pattern = QRegularExpression(QStringLiteral("#[^\n]*"));
    rule.format = m_singleLineCommentFormat;
    m_highlightingRules.append(rule);
}

void CppSyntaxHighlighter::highlightBlock(const QString& text) {
    for (const HighlightingRule& rule : m_highlightingRules) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }

    if (m_fileType == FileType::Cpp || m_fileType == FileType::Generic) {
        setCurrentBlockState(0);

        int startIndex = 0;
        if (previousBlockState() != 1)
            startIndex = text.indexOf(m_commentStartExpression);

        while (startIndex >= 0) {
            QRegularExpressionMatch endMatch;
            int endIndex = text.indexOf(m_commentEndExpression, startIndex, &endMatch);
            int commentLength;
            if (endIndex == -1) {
                setCurrentBlockState(1);
                commentLength = text.length() - startIndex;
            } else {
                commentLength = endIndex - startIndex + endMatch.capturedLength();
            }
            setFormat(startIndex, commentLength, m_multiLineCommentFormat);
            startIndex = text.indexOf(m_commentStartExpression, startIndex + commentLength);
        }
    }
}

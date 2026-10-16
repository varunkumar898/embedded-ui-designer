#include "DiagnosticParser.h"
#include <QRegularExpression>

CompilerDiagnostic DiagnosticParser::parseLine(const QString& line) {
    CompilerDiagnostic diag;
    diag.rawLine = line;

    // 1. GCC/Clang: "file:line:col: severity: message" or "file:line: severity: message"
    // e.g. "src/main.c:42:15: error: expected ';' before 'return'"
    static const QRegularExpression gccRegex(R"(^([^:\r\n]+):(\d+)(?::(\d+))?:\s*(error|warning|note|info):\s*(.+)$)", QRegularExpression::CaseInsensitiveOption);
    auto gccMatch = gccRegex.match(line);
    if (gccMatch.hasMatch()) {
        diag.filePath = gccMatch.captured(1).trimmed();
        diag.line = gccMatch.captured(2).toInt();
        if (!gccMatch.captured(3).isEmpty()) {
            diag.column = gccMatch.captured(3).toInt();
        }
        QString sevStr = gccMatch.captured(4).toLower();
        if (sevStr == "error") {
            diag.severity = DiagnosticSeverity::Error;
        } else if (sevStr == "warning") {
            diag.severity = DiagnosticSeverity::Warning;
        } else {
            diag.severity = DiagnosticSeverity::Info;
        }
        diag.message = gccMatch.captured(5).trimmed();
        return diag;
    }

    // 2. MSVC: "file(line,col): error Cxxxx: message" or "file(line): error Cxxxx: message"
    // e.g. "C:\project\main.cpp(55,10): error C2065: 'undeclared_var': undeclared identifier"
    static const QRegularExpression msvcRegex(R"(^([A-Za-z]:[^\(\r\n]+|[^\(\r\n]+)\((\d+)(?:,(\d+))?\)\s*:\s*(error|warning|note|info)(?:\s+[A-Za-z0-9]+)?\s*:\s*(.+)$)", QRegularExpression::CaseInsensitiveOption);
    auto msvcMatch = msvcRegex.match(line);
    if (msvcMatch.hasMatch()) {
        diag.filePath = msvcMatch.captured(1).trimmed();
        diag.line = msvcMatch.captured(2).toInt();
        if (!msvcMatch.captured(3).isEmpty()) {
            diag.column = msvcMatch.captured(3).toInt();
        }
        QString sevStr = msvcMatch.captured(4).toLower();
        if (sevStr == "error") {
            diag.severity = DiagnosticSeverity::Error;
        } else if (sevStr == "warning") {
            diag.severity = DiagnosticSeverity::Warning;
        } else {
            diag.severity = DiagnosticSeverity::Info;
        }
        diag.message = msvcMatch.captured(5).trimmed();
        return diag;
    }

    // 3. CMake: "CMake Error at CMakeLists.txt:14 (find_package): Could not find Qt6"
    // or "CMake Warning at ..."
    static const QRegularExpression cmakeRegex(R"(^CMake\s+(Error|Warning)\s+at\s+([^:\r\n]+):(\d+)(?:\s+\([^\)]+\))?:\s*(.+)$)", QRegularExpression::CaseInsensitiveOption);
    auto cmakeMatch = cmakeRegex.match(line);
    if (cmakeMatch.hasMatch()) {
        QString sevStr = cmakeMatch.captured(1).toLower();
        diag.severity = (sevStr == "error") ? DiagnosticSeverity::Error : DiagnosticSeverity::Warning;
        diag.filePath = cmakeMatch.captured(2).trimmed();
        diag.line = cmakeMatch.captured(3).toInt();
        diag.message = cmakeMatch.captured(4).trimmed();
        return diag;
    }

    diag.message = line.trimmed();
    return diag;
}

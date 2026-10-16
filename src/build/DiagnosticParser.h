#pragma once

#include <QString>

enum class DiagnosticSeverity {
    Info,
    Warning,
    Error
};

inline QString diagnosticSeverityToString(DiagnosticSeverity s) {
    switch (s) {
    case DiagnosticSeverity::Error: return "Error";
    case DiagnosticSeverity::Warning: return "Warning";
    case DiagnosticSeverity::Info:
    default: return "Info";
    }
}

struct CompilerDiagnostic {
    DiagnosticSeverity severity = DiagnosticSeverity::Info;
    QString filePath;
    int line = 0;
    int column = 0;
    QString message;
    QString rawLine;
};

class DiagnosticParser {
public:
    CompilerDiagnostic parseLine(const QString& line);
};

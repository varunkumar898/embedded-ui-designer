#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>
#include "build/DiagnosticParser.h"

class ProblemsWidget : public QWidget {
    Q_OBJECT

public:
    explicit ProblemsWidget(QWidget *parent = nullptr);
    ~ProblemsWidget() override;

    void addDiagnostic(const CompilerDiagnostic& diag);
    void clearDiagnostics();
    int count() const;
    int errorCount() const { return m_errorCount; }
    int warningCount() const { return m_warningCount; }

signals:
    void problemDoubleClicked(const QString& filePath, int line, int column);
    void countsChanged(int total, int errors, int warnings);

private slots:
    void onCellDoubleClicked(int row, int col);

private:
    void setupUi();

    QTableWidget *m_table = nullptr;
    QLabel *m_summaryLabel = nullptr;
    QPushButton *m_clearBtn = nullptr;

    int m_errorCount = 0;
    int m_warningCount = 0;
};

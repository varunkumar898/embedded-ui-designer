#pragma once

#include <QDialog>
#include <QLabel>
#include <QProgressBar>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include "ResourceAnalysis.h"

class ExportValidationDialog : public QDialog {
    Q_OBJECT

public:
    enum ResultAction { EXPORT, CANCEL, OPTIMIZE };

    explicit ExportValidationDialog(const ResourceAnalysis& analysis, QWidget* parent = nullptr);
    ~ExportValidationDialog() override = default;

    ResultAction resultAction() const { return m_action; }

private slots:
    void onExportClicked();
    void onCancelClicked();
    void onOptimizeClicked();

private:
    void setupUi();

    ResourceAnalysis m_analysis;
    ResultAction m_action = CANCEL;

    QLabel* m_headerLabel = nullptr;
    QLabel* m_summaryLabel = nullptr;
    QProgressBar* m_flashBar = nullptr;
    QProgressBar* m_ramBar = nullptr;
    QListWidget* m_warningsList = nullptr;
};

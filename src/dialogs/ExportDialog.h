#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QTextEdit>
#include <QPushButton>
#include "codegen/FirmwareExportManager.h"

class Project;

class ExportDialog : public QDialog {
    Q_OBJECT

public:
    explicit ExportDialog(Project* project, QWidget* parent = nullptr);
    ~ExportDialog() override = default;

private slots:
    void onBrowseClicked();
    void onExportClicked();

private:
    void setupUi();
    void updateValidation();

    Project* m_project = nullptr;
    FirmwareExportManager m_exportManager;

    QLineEdit* m_outputPathEdit = nullptr;
    QPushButton* m_browseBtn = nullptr;
    QComboBox* m_frameworkCombo = nullptr;
    QComboBox* m_targetCombo = nullptr;
    QCheckBox* m_generateCmakeCheck = nullptr;
    QCheckBox* m_overwriteUserCodeCheck = nullptr;
    QTextEdit* m_summaryEdit = nullptr;
    QPushButton* m_exportBtn = nullptr;
    QPushButton* m_closeBtn = nullptr;
};

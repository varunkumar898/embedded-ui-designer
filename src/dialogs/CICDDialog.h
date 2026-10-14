#pragma once

#include <QDialog>
#include "cicd/CICDGenerator.h"

class QComboBox;
class QLineEdit;
class QCheckBox;
class QTextEdit;
class QPushButton;
class QLabel;
class Project;

namespace CICD {

class CICDDialog : public QDialog {
    Q_OBJECT

public:
    explicit CICDDialog(Project* project, QWidget* parent = nullptr);
    ~CICDDialog() override = default;

private slots:
    void onValidate();
    void onGenerate();
    void onSaveToFile();
    void onProviderChanged(int index);

private:
    void setupUI();
    CICDConfig currentConfig() const;

    Project* m_project = nullptr;

    QComboBox* m_providerCombo = nullptr;
    QLineEdit* m_targetMcuEdit = nullptr;
    QComboBox* m_generatorCombo = nullptr;
    QLineEdit* m_toolchainEdit = nullptr;
    QLineEdit* m_artifactPathEdit = nullptr;
    QCheckBox* m_runBuildCheck = nullptr;
    QCheckBox* m_runTestsCheck = nullptr;
    QCheckBox* m_archiveCheck = nullptr;

    QTextEdit* m_validationOutput = nullptr;
    QTextEdit* m_pipelinePreview = nullptr;
    QPushButton* m_saveButton = nullptr;
    QLabel* m_statusLabel = nullptr;
};

} // namespace CICD

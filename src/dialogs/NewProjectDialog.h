#pragma once

#include <QDialog>

class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QSpinBox;
class QTabWidget;

class NewProjectDialog : public QDialog {
    Q_OBJECT

public:
    explicit NewProjectDialog(QWidget* parent = nullptr);

    bool startsFromTemplate() const;
    QString templateResource() const;
    QString projectName() const;
    int projectWidth() const;
    int projectHeight() const;
    QString targetFramework() const;
    int colorDepth() const;
    bool roundDisplay() const;

private slots:
    void updateTemplateDetails();

private:
    QTabWidget* m_tabs = nullptr;
    QLineEdit* m_projectNameEdit = nullptr;
    QSpinBox* m_widthSpin = nullptr;
    QSpinBox* m_heightSpin = nullptr;
    QSpinBox* m_colorDepthSpin = nullptr;
    QComboBox* m_shapeCombo = nullptr;
    QComboBox* m_frameworkCombo = nullptr;
    QListWidget* m_templateList = nullptr;
    QLabel* m_templateDescription = nullptr;
    QString m_suggestedName;
};
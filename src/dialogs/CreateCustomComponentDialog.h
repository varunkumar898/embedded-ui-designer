#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>

class CreateCustomComponentDialog : public QDialog {
    Q_OBJECT

public:
    explicit CreateCustomComponentDialog(QWidget* parent = nullptr);

    QString componentName() const;
    QString behaviorRole() const;
    double minValue() const;
    double maxValue() const;
    double defaultValue() const;

private slots:
    void onRoleChanged(int index);

private:
    QLineEdit* m_nameEdit = nullptr;
    QComboBox* m_roleCombo = nullptr;
    QWidget* m_rangeContainer = nullptr;
    QDoubleSpinBox* m_minSpin = nullptr;
    QDoubleSpinBox* m_maxSpin = nullptr;
    QDoubleSpinBox* m_defaultSpin = nullptr;

    void setupUi();
};

#include "CreateCustomComponentDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QGroupBox>

CreateCustomComponentDialog::CreateCustomComponentDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Create Custom Component");
    setModal(true);
    setMinimumWidth(380);
    setupUi();
}

void CreateCustomComponentDialog::setupUi() {
    setStyleSheet(
        "QDialog { background-color: #16181d; color: #e4ecf7; font-family: 'Segoe UI', sans-serif; }"
        "QLabel { color: #abb7c9; font-size: 12px; font-weight: 500; }"
        "QLineEdit, QComboBox, QDoubleSpinBox { "
        "  background-color: #1f232b; color: #ffffff; border: 1px solid #2d3340; "
        "  border-radius: 6px; padding: 6px 10px; font-size: 13px; "
        "}"
        "QLineEdit:focus, QComboBox:focus, QDoubleSpinBox:focus { "
        "  border-color: #1a73e8; background-color: #242933; "
        "}"
        "QComboBox::drop-down { border: none; width: 20px; }"
        "QPushButton { "
        "  background-color: #1a73e8; color: #ffffff; border: none; "
        "  border-radius: 6px; padding: 8px 18px; font-weight: 600; font-size: 12px; "
        "}"
        "QPushButton:hover { background-color: #1557b0; }"
        "QPushButton#cancelBtn { "
        "  background-color: #262a34; color: #cbd5e1; border: 1px solid #363c4a; "
        "}"
        "QPushButton#cancelBtn:hover { background-color: #313744; color: #ffffff; }"
    );

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(16);

    QLabel* header = new QLabel("CREATE REUSABLE COMPONENT", this);
    header->setStyleSheet("color: #7b889b; font-size: 11px; font-weight: bold; letter-spacing: 0.8px;");
    mainLayout->addWidget(header);

    QFormLayout* form = new QFormLayout();
    form->setSpacing(12);

    m_nameEdit = new QLineEdit("MySlider", this);
    form->addRow("Component Name:", m_nameEdit);

    m_roleCombo = new QComboBox(this);
    m_roleCombo->addItems({"Slider", "ProgressBar", "Button", "None"});
    form->addRow("Behavior Role:", m_roleCombo);

    mainLayout->addLayout(form);

    m_rangeContainer = new QWidget(this);
    QFormLayout* rangeForm = new QFormLayout(m_rangeContainer);
    rangeForm->setContentsMargins(0, 0, 0, 0);
    rangeForm->setSpacing(8);

    m_minSpin = new QDoubleSpinBox(this);
    m_minSpin->setRange(-999999, 999999);
    m_minSpin->setValue(0.0);
    rangeForm->addRow("Minimum Value:", m_minSpin);

    m_maxSpin = new QDoubleSpinBox(this);
    m_maxSpin->setRange(-999999, 999999);
    m_maxSpin->setValue(100.0);
    rangeForm->addRow("Maximum Value:", m_maxSpin);

    m_defaultSpin = new QDoubleSpinBox(this);
    m_defaultSpin->setRange(-999999, 999999);
    m_defaultSpin->setValue(50.0);
    rangeForm->addRow("Default Value:", m_defaultSpin);

    mainLayout->addWidget(m_rangeContainer);

    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    QPushButton* cancelBtn = new QPushButton("Cancel", this);
    cancelBtn->setObjectName("cancelBtn");
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(cancelBtn);

    QPushButton* okBtn = new QPushButton("Create Component", this);
    connect(okBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(okBtn);

    mainLayout->addLayout(btnLayout);

    connect(m_roleCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CreateCustomComponentDialog::onRoleChanged);

    onRoleChanged(m_roleCombo->currentIndex());
}

void CreateCustomComponentDialog::onRoleChanged(int index) {
    Q_UNUSED(index);
    QString role = m_roleCombo->currentText();
    bool needsRange = (role == "Slider" || role == "ProgressBar");
    m_rangeContainer->setVisible(needsRange);
}

QString CreateCustomComponentDialog::componentName() const {
    QString name = m_nameEdit->text().trimmed();
    return name.isEmpty() ? "CustomComponent" : name;
}

QString CreateCustomComponentDialog::behaviorRole() const {
    return m_roleCombo->currentText();
}

double CreateCustomComponentDialog::minValue() const {
    return m_minSpin->value();
}

double CreateCustomComponentDialog::maxValue() const {
    return m_maxSpin->value();
}

double CreateCustomComponentDialog::defaultValue() const {
    return m_defaultSpin->value();
}

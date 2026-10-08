#include "NewProjectDialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>

NewProjectDialog::NewProjectDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("New Project");
    setMinimumSize(720, 500);

    auto* root = new QVBoxLayout(this);
    auto* projectNameRow = new QFormLayout();
    m_projectNameEdit = new QLineEdit("NewEmbeddedApp", this);
    projectNameRow->addRow("Project name", m_projectNameEdit);
    root->addLayout(projectNameRow);

    m_tabs = new QTabWidget(this);
    auto* blankPage = new QWidget(m_tabs);
    auto* blankForm = new QFormLayout(blankPage);
    m_widthSpin = new QSpinBox(blankPage);
    m_widthSpin->setRange(64, 2000);
    m_widthSpin->setValue(320);
    m_heightSpin = new QSpinBox(blankPage);
    m_heightSpin->setRange(64, 2000);
    m_heightSpin->setValue(240);
    m_colorDepthSpin = new QSpinBox(blankPage);
    m_colorDepthSpin->setRange(1, 32);
    m_colorDepthSpin->setValue(16);
    m_shapeCombo = new QComboBox(blankPage);
    m_shapeCombo->addItem("Rectangular", false);
    m_shapeCombo->addItem("Round", true);
    m_frameworkCombo = new QComboBox(blankPage);
    m_frameworkCombo->addItem("uGFX", "ugfx");
    m_frameworkCombo->addItem("Qt for MCUs (QUL)", "qt-for-mcus");
    m_frameworkCombo->addItem("LVGL", "lvgl");

    m_hardwareFamilyCombo = new QComboBox(blankPage);
    m_hardwareFamilyCombo->addItem("STMicroelectronics STM32", "STM32");
    m_hardwareFamilyCombo->addItem("Espressif ESP32", "ESP32");
    m_hardwareFamilyCombo->addItem("Raspberry Pi", "Raspberry Pi");
    m_hardwareFamilyCombo->addItem("Custom Hardware Target", "Custom");

    m_hardwareBoardCombo = new QComboBox(blankPage);

    blankForm->addRow("Display width", m_widthSpin);
    blankForm->addRow("Display height", m_heightSpin);
    blankForm->addRow("Color depth", m_colorDepthSpin);
    blankForm->addRow("Display shape", m_shapeCombo);
    blankForm->addRow("Target framework", m_frameworkCombo);
    blankForm->addRow("Target hardware family", m_hardwareFamilyCombo);
    blankForm->addRow("Target board", m_hardwareBoardCombo);

    connect(m_hardwareFamilyCombo, &QComboBox::currentTextChanged, this, &NewProjectDialog::updateHardwareBoards);
    updateHardwareBoards();

    connect(m_shapeCombo, &QComboBox::currentTextChanged, this, [this](const QString& selected) {
        const bool round = selected == "Round";
        m_heightSpin->setEnabled(!round);
        if (round) m_heightSpin->setValue(m_widthSpin->value());
    });
    connect(m_widthSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int value) {
        if (roundDisplay()) m_heightSpin->setValue(value);
    });
    m_tabs->addTab(blankPage, "Blank Project");

    auto* templatePage = new QWidget(m_tabs);
    auto* templateLayout = new QHBoxLayout(templatePage);
    m_templateList = new QListWidget(templatePage);
    m_templateList->setMinimumWidth(230);
    m_templateList->addItem("Thermostat");
    m_templateList->item(0)->setData(Qt::UserRole, ":/examples/thermostat.euiproj");
    m_templateList->item(0)->setData(Qt::UserRole + 1, "Thermostat");
    m_templateList->item(0)->setData(Qt::UserRole + 2,
                                     "Room temperature, humidity, and heating controls.");
    m_templateList->addItem("Automotive Cluster");
    m_templateList->item(1)->setData(Qt::UserRole, ":/examples/automotive_cluster.euiproj");
    m_templateList->item(1)->setData(Qt::UserRole + 1, "AutomotiveCluster");
    m_templateList->item(1)->setData(Qt::UserRole + 2,
                                     "A compact instrument panel with speed and fuel indicators.");
    m_templateList->addItem("Smartwatch");
    m_templateList->item(2)->setData(Qt::UserRole, ":/examples/smartwatch.euiproj");
    m_templateList->item(2)->setData(Qt::UserRole + 1, "Smartwatch");
    m_templateList->item(2)->setData(Qt::UserRole + 2,
                                     "A round watch face with time, activity, and battery status.");

    auto* templateInfo = new QWidget(templatePage);
    auto* infoLayout = new QVBoxLayout(templateInfo);
    QLabel* templateTitle = new QLabel("Choose a starting point", templateInfo);
    templateTitle->setStyleSheet("font-size: 18px; font-weight: 600;");
    m_templateDescription = new QLabel(templateInfo);
    m_templateDescription->setWordWrap(true);
    m_templateDescription->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    infoLayout->addWidget(templateTitle);
    infoLayout->addWidget(m_templateDescription);
    infoLayout->addStretch(1);
    templateLayout->addWidget(m_templateList);
    templateLayout->addWidget(templateInfo, 1);
    m_tabs->addTab(templatePage, "Start from Template");
    root->addWidget(m_tabs, 1);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Cancel, this);
    auto* createButton = buttons->addButton("Create Project", QDialogButtonBox::AcceptRole);
    root->addWidget(buttons);
    connect(createButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_tabs, &QTabWidget::currentChanged, this, &NewProjectDialog::updateTemplateDetails);
    connect(m_templateList, &QListWidget::currentItemChanged, this,
            [this](QListWidgetItem*, QListWidgetItem*) { updateTemplateDetails(); });

    m_templateList->setCurrentRow(0);
    updateTemplateDetails();
}

bool NewProjectDialog::startsFromTemplate() const {
    return m_tabs->currentIndex() == 1;
}

QString NewProjectDialog::templateResource() const {
    const QListWidgetItem* item = m_templateList->currentItem();
    return item ? item->data(Qt::UserRole).toString() : QString();
}

QString NewProjectDialog::projectName() const {
    const QString name = m_projectNameEdit->text().trimmed();
    return name.isEmpty() ? QString("MyEmbeddedApp") : name;
}

int NewProjectDialog::projectWidth() const {
    return m_widthSpin->value();
}

int NewProjectDialog::projectHeight() const {
    return roundDisplay() ? m_widthSpin->value() : m_heightSpin->value();
}

QString NewProjectDialog::targetFramework() const {
    return m_frameworkCombo->currentData().toString();
}

int NewProjectDialog::colorDepth() const {
    return m_colorDepthSpin->value();
}

bool NewProjectDialog::roundDisplay() const {
    return m_shapeCombo->currentData().toBool();
}

QString NewProjectDialog::hardwareFamily() const {
    return m_hardwareFamilyCombo ? m_hardwareFamilyCombo->currentData().toString() : "STM32";
}

QString NewProjectDialog::hardwareBoard() const {
    return m_hardwareBoardCombo ? m_hardwareBoardCombo->currentData().toString() : "";
}

void NewProjectDialog::updateHardwareBoards() {
    if (!m_hardwareFamilyCombo || !m_hardwareBoardCombo) return;
    QString family = m_hardwareFamilyCombo->currentData().toString();
    m_hardwareBoardCombo->clear();

    if (family == "STM32") {
        m_hardwareBoardCombo->addItem("NUCLEO-F030R8 (STM32F030R8)", "NUCLEO-F030R8");
        m_hardwareBoardCombo->addItem("STM32F4-Discovery (STM32F407VG)", "STM32F407G-DISC1");
        m_hardwareBoardCombo->addItem("STM32H747I-DISCO", "STM32H747I-DISCO");
    } else if (family == "ESP32") {
        m_hardwareBoardCombo->addItem("ESP32-S3-DevKitC-1 (ESP32-S3)", "ESP32-S3-DevKitC-1");
        m_hardwareBoardCombo->addItem("ESP32-WROOM-32", "ESP32-WROOM-32");
    } else if (family == "Raspberry Pi") {
        m_hardwareBoardCombo->addItem("Raspberry Pi 40-Pin Header (BCM2835/BCM2711)", "RPI_40PIN");
        m_hardwareBoardCombo->addItem("Raspberry Pi Pico (RP2040)", "Raspberry-Pi-Pico");
    } else {
        m_hardwareBoardCombo->addItem("Generic Custom Board Definition", "CUSTOM_BOARD");
    }
}

void NewProjectDialog::updateTemplateDetails() {
    const QListWidgetItem* item = m_templateList->currentItem();
    if (!item) {
        m_templateDescription->clear();
        return;
    }

    const QString suggestedName = item->data(Qt::UserRole + 1).toString();
    m_templateDescription->setText(item->data(Qt::UserRole + 2).toString());
    if (startsFromTemplate()) {
        if (m_projectNameEdit->text().isEmpty()
            || m_projectNameEdit->text() == "NewEmbeddedApp"
            || m_projectNameEdit->text() == m_suggestedName) {
            m_projectNameEdit->setText(suggestedName);
        }
        m_suggestedName = suggestedName;
    } else if (m_projectNameEdit->text() == m_suggestedName) {
        m_projectNameEdit->setText("NewEmbeddedApp");
        m_suggestedName.clear();
    }
}
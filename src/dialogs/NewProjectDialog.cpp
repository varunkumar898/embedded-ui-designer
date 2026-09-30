#include "NewProjectDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QGroupBox>

NewProjectDialog::NewProjectDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("New Embedded Project");
    setModal(true);
    setMinimumWidth(500);
    setMinimumHeight(460);

    setupUi();
    loadDevices();
}

void NewProjectDialog::setupUi() {
    setStyleSheet(
        "QDialog { background-color: #16181d; color: #e4ecf7; font-family: 'Segoe UI', sans-serif; }"
        "QLabel { color: #abb7c9; font-size: 12px; font-weight: 500; }"
        "QLineEdit, QSpinBox, QComboBox { "
        "  background-color: #1f232b; color: #ffffff; border: 1px solid #2d3340; "
        "  border-radius: 6px; padding: 6px 10px; font-size: 13px; "
        "}"
        "QLineEdit:focus, QSpinBox:focus, QComboBox:focus { "
        "  border-color: #1a73e8; background-color: #242933; "
        "}"
        "QTabWidget::pane { border: 1px solid #2d3340; border-radius: 6px; background-color: #1a1c22; padding: 12px; }"
        "QTabBar::tab { "
        "  background-color: #1f232b; color: #8f9cae; border: 1px solid #2d3340; "
        "  border-bottom: none; border-top-left-radius: 6px; border-top-right-radius: 6px; "
        "  padding: 8px 16px; margin-right: 4px; font-weight: 600; font-size: 12px; "
        "}"
        "QTabBar::tab:selected { background-color: #1a1c22; color: #ffffff; border-color: #3d4556; }"
        "QTabBar::tab:hover { color: #ffffff; background-color: #262c37; }"
        "QListWidget { "
        "  background-color: #16181d; color: #e2e8f0; border: 1px solid #2d3340; "
        "  border-radius: 6px; padding: 4px; font-size: 12px; "
        "}"
        "QListWidget::item { padding: 8px 10px; border-radius: 4px; margin-bottom: 2px; }"
        "QListWidget::item:selected { background-color: #1a73e8; color: #ffffff; font-weight: bold; }"
        "QListWidget::item:hover:!selected { background-color: #242a36; }"
        "QPushButton { "
        "  background-color: #1a73e8; color: #ffffff; border: none; "
        "  border-radius: 6px; padding: 8px 20px; font-weight: 600; font-size: 12px; "
        "}"
        "QPushButton:hover { background-color: #1557b0; }"
        "QPushButton#cancelBtn { "
        "  background-color: #262a34; color: #cbd5e1; border: 1px solid #363c4a; "
        "}"
        "QPushButton#cancelBtn:hover { background-color: #313744; color: #ffffff; }"
    );

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(22, 20, 22, 20);
    mainLayout->setSpacing(16);

    // Header
    QLabel* header = new QLabel("NEW PROJECT WIZARD", this);
    header->setStyleSheet("color: #00e5ff; font-size: 11px; font-weight: bold; letter-spacing: 1px;");
    mainLayout->addWidget(header);

    QLabel* subheader = new QLabel("Configure your target hardware platform and display specifications", this);
    subheader->setStyleSheet("color: #7b889b; font-size: 12px; margin-bottom: 4px;");
    mainLayout->addWidget(subheader);

    // Project Name Field
    QFormLayout* nameForm = new QFormLayout();
    m_nameEdit = new QLineEdit("MyEmbeddedApp", this);
    m_nameEdit->setPlaceholderText("Enter project name...");
    nameForm->addRow("Project Name:", m_nameEdit);
    mainLayout->addLayout(nameForm);

    // Modes Tab Widget
    m_tabWidget = new QTabWidget(this);

    // Tab 1: Choose a Board
    QWidget* boardTab = new QWidget();
    QVBoxLayout* boardLayout = new QVBoxLayout(boardTab);
    boardLayout->setContentsMargins(6, 8, 6, 6);
    boardLayout->setSpacing(10);

    QLabel* boardLabel = new QLabel("Select pre-configured microcontroller board profile:", boardTab);
    boardLayout->addWidget(boardLabel);

    m_boardList = new QListWidget(boardTab);
    m_boardList->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_boardList, &QListWidget::itemSelectionChanged, this, &NewProjectDialog::onBoardSelectionChanged);
    boardLayout->addWidget(m_boardList, 1);

    m_boardDetailsLabel = new QLabel(boardTab);
    m_boardDetailsLabel->setStyleSheet(
        "background-color: #13151a; border: 1px solid #242933; border-radius: 6px; "
        "padding: 8px 12px; color: #94a3b8; font-size: 11.5px; line-height: 1.4;"
    );
    m_boardDetailsLabel->setWordWrap(true);
    boardLayout->addWidget(m_boardDetailsLabel);

    m_tabWidget->addTab(boardTab, "Choose a Board");

    // Tab 2: Custom Display
    QWidget* customTab = new QWidget();
    QVBoxLayout* customTabLayout = new QVBoxLayout(customTab);
    customTabLayout->setContentsMargins(6, 8, 6, 6);
    customTabLayout->setSpacing(12);

    QLabel* customLabel = new QLabel("Define custom embedded display parameters:", customTab);
    customTabLayout->addWidget(customLabel);

    QFormLayout* customForm = new QFormLayout();
    customForm->setSpacing(12);

    m_widthSpin = new QSpinBox(customTab);
    m_widthSpin->setRange(16, 4096);
    m_widthSpin->setValue(320);
    m_widthSpin->setSuffix(" px");
    customForm->addRow("Display Width:", m_widthSpin);

    m_heightSpin = new QSpinBox(customTab);
    m_heightSpin->setRange(16, 4096);
    m_heightSpin->setValue(240);
    m_heightSpin->setSuffix(" px");
    customForm->addRow("Display Height:", m_heightSpin);

    m_colorDepthCombo = new QComboBox(customTab);
    m_colorDepthCombo->addItem("16-bit (RGB565 - High Color)", 16);
    m_colorDepthCombo->addItem("24-bit (RGB888 - True Color)", 24);
    m_colorDepthCombo->addItem("8-bit (256 Colors / Grayscale)", 8);
    m_colorDepthCombo->addItem("1-bit (Monochrome OLED / E-Ink)", 1);
    m_colorDepthCombo->setCurrentIndex(0);
    customForm->addRow("Color Depth:", m_colorDepthCombo);

    m_displayTypeCombo = new QComboBox(customTab);
    m_displayTypeCombo->addItems({"TFT LCD", "IPS LCD", "Capacitive Touch LCD", "OLED", "E-Ink"});
    customForm->addRow("Panel Type:", m_displayTypeCombo);

    customTabLayout->addLayout(customForm);
    customTabLayout->addStretch(1);

    m_tabWidget->addTab(customTab, "Custom Display");

    mainLayout->addWidget(m_tabWidget, 1);

    connect(m_tabWidget, &QTabWidget::currentChanged, this, &NewProjectDialog::onModeTabChanged);

    // Action Buttons
    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addStretch(1);

    QPushButton* cancelBtn = new QPushButton("Cancel", this);
    cancelBtn->setObjectName("cancelBtn");
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(cancelBtn);

    QPushButton* okBtn = new QPushButton("Create Project", this);
    okBtn->setDefault(true);
    connect(okBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(okBtn);

    mainLayout->addLayout(btnLayout);
}

void NewProjectDialog::loadDevices() {
    m_boards.clear();
    m_boardList->clear();

    QFile file(":/config/devices.json");
    if (!file.open(QIODevice::ReadOnly)) {
        file.setFileName("resources/config/devices.json");
        file.open(QIODevice::ReadOnly);
    }

    if (file.isOpen()) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        file.close();

        if (doc.isObject()) {
            QJsonArray devicesArr = doc.object().value("devices").toArray();
            for (const auto& devVal : devicesArr) {
                QJsonObject obj = devVal.toObject();
                BoardDeviceInfo info;
                info.name = obj.value("name").toString();
                info.manufacturer = obj.value("manufacturer").toString();
                info.architecture = obj.value("architecture").toString();
                info.ramKb = obj.value("ram_kb").toInt();
                info.flashKb = obj.value("flash_kb").toInt();

                QJsonObject disp = obj.value("display").toObject();
                info.width = disp.value("width").toInt(320);
                info.height = disp.value("height").toInt(240);
                info.displayType = disp.value("type").toString("LCD");
                info.displayInterface = disp.value("interface").toString("SPI");

                m_boards.append(info);

                QString itemText = QString("%1  —  %2 × %3  (%4)")
                    .arg(info.name)
                    .arg(info.width)
                    .arg(info.height)
                    .arg(info.manufacturer);
                m_boardList->addItem(itemText);
            }
        }
    }

    // Fallback if file couldn't be loaded
    if (m_boards.isEmpty()) {
        BoardDeviceInfo esp32;
        esp32.name = "ESP32-S3-BOX";
        esp32.manufacturer = "Espressif";
        esp32.architecture = "Xtensa LX7 Dual-Core";
        esp32.width = 320;
        esp32.height = 240;
        esp32.displayType = "IPS LCD";
        m_boards.append(esp32);
        m_boardList->addItem("ESP32-S3-BOX  —  320 × 240  (Espressif)");

        BoardDeviceInfo stm32;
        stm32.name = "STM32H747I-DISCO";
        stm32.manufacturer = "STMicroelectronics";
        stm32.architecture = "ARM Cortex-M7 + M4";
        stm32.width = 800;
        stm32.height = 480;
        stm32.displayType = "TFT LCD";
        m_boards.append(stm32);
        m_boardList->addItem("STM32H747I-DISCO  —  800 × 480  (STMicroelectronics)");
    }

    if (m_boardList->count() > 0) {
        m_boardList->setCurrentRow(0);
        updateBoardDetails();
    }
}

void NewProjectDialog::onBoardSelectionChanged() {
    updateBoardDetails();
}

void NewProjectDialog::onModeTabChanged(int index) {
    Q_UNUSED(index);
}

void NewProjectDialog::updateBoardDetails() {
    int row = m_boardList->currentRow();
    if (row >= 0 && row < m_boards.size()) {
        const BoardDeviceInfo& b = m_boards.at(row);
        QString text = QString("<b>Board:</b> %1 (%2)<br>"
                               "<b>CPU:</b> %3 | <b>RAM:</b> %4 KB | <b>Flash:</b> %5 KB<br>"
                               "<b>Display:</b> %6 × %7 (%8 via %9)")
            .arg(b.name, b.manufacturer, b.architecture)
            .arg(b.ramKb).arg(b.flashKb)
            .arg(b.width).arg(b.height).arg(b.displayType, b.displayInterface);
        m_boardDetailsLabel->setText(text);
    } else {
        m_boardDetailsLabel->setText("No board selected");
    }
}

QString NewProjectDialog::projectName() const {
    QString name = m_nameEdit->text().trimmed();
    return name.isEmpty() ? "MyEmbeddedApp" : name;
}

DisplayConfig NewProjectDialog::displayConfig() const {
    DisplayConfig cfg;
    if (isBoardMode()) {
        int row = m_boardList->currentRow();
        if (row >= 0 && row < m_boards.size()) {
            const BoardDeviceInfo& b = m_boards.at(row);
            cfg.width = b.width;
            cfg.height = b.height;
            cfg.colorDepth = 16;
            cfg.type = b.displayType;
            cfg.dpi = 96;
            return cfg;
        }
    }

    cfg.width = m_widthSpin->value();
    cfg.height = m_heightSpin->value();
    cfg.colorDepth = m_colorDepthCombo->currentData().toInt();
    cfg.type = m_displayTypeCombo->currentText();
    cfg.dpi = 96;
    return cfg;
}

bool NewProjectDialog::isBoardMode() const {
    return m_tabWidget->currentIndex() == 0;
}

QString NewProjectDialog::selectedBoardName() const {
    int row = m_boardList->currentRow();
    if (row >= 0 && row < m_boards.size()) {
        return m_boards.at(row).name;
    }
    return QString();
}

void NewProjectDialog::selectBoard(const QString& boardName) {
    m_tabWidget->setCurrentIndex(0);
    for (int i = 0; i < m_boards.size(); ++i) {
        if (m_boards.at(i).name.contains(boardName, Qt::CaseInsensitive)) {
            m_boardList->setCurrentRow(i);
            updateBoardDetails();
            break;
        }
    }
}

void NewProjectDialog::setCustomDisplay(int width, int height, int colorDepth, const QString& type) {
    m_tabWidget->setCurrentIndex(1);
    m_widthSpin->setValue(width);
    m_heightSpin->setValue(height);
    for (int i = 0; i < m_colorDepthCombo->count(); ++i) {
        if (m_colorDepthCombo->itemData(i).toInt() == colorDepth) {
            m_colorDepthCombo->setCurrentIndex(i);
            break;
        }
    }
    int typeIdx = m_displayTypeCombo->findText(type);
    if (typeIdx >= 0) {
        m_displayTypeCombo->setCurrentIndex(typeIdx);
    }
}

void NewProjectDialog::setProjectName(const QString& name) {
    m_nameEdit->setText(name);
}

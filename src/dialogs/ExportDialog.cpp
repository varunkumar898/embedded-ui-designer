#include "ExportDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFileDialog>

ExportDialog::ExportDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Export Project");
    setModal(true);
    setMinimumWidth(440);
    setupUi();
}

void ExportDialog::setupUi() {
    setStyleSheet(
        "QDialog { background-color: #16181d; color: #e4ecf7; font-family: 'Segoe UI', sans-serif; }"
        "QLabel { color: #abb7c9; font-size: 12px; }"
        "QGroupBox { "
        "  border: 1px solid #2d3340; border-radius: 8px; margin-top: 10px; "
        "  padding: 10px 14px; background-color: #1a1c22; "
        "}"
        "QGroupBox::title { "
        "  subcontrol-origin: margin; subcontrol-position: top left; "
        "  padding: 0 6px; color: #7b889b; font-size: 11px; font-weight: bold; letter-spacing: 0.6px; "
        "}"
        "QRadioButton { color: #e2e8f0; font-size: 13px; spacing: 8px; padding: 5px 0; }"
        "QRadioButton::indicator { width: 16px; height: 16px; }"
        "QRadioButton::indicator:unchecked { "
        "  border: 2px solid #3b4455; border-radius: 8px; background-color: #1f2430; "
        "}"
        "QRadioButton::indicator:checked { "
        "  border: 2px solid #1a73e8; border-radius: 8px; background-color: #1a73e8; "
        "}"
        "QLineEdit { "
        "  background-color: #1f232b; color: #ffffff; border: 1px solid #2d3340; "
        "  border-radius: 6px; padding: 6px 10px; font-size: 12px; "
        "}"
        "QLineEdit:focus { border-color: #1a73e8; }"
        "QPushButton { "
        "  background-color: #1a73e8; color: #ffffff; border: none; "
        "  border-radius: 6px; padding: 8px 18px; font-weight: 600; font-size: 12px; "
        "}"
        "QPushButton:hover { background-color: #1557b0; }"
        "QPushButton#cancelBtn { "
        "  background-color: #262a34; color: #cbd5e1; border: 1px solid #363c4a; "
        "}"
        "QPushButton#cancelBtn:hover { background-color: #313744; color: #ffffff; }"
        "QPushButton#browseBtn { "
        "  background-color: #262a34; color: #94a3b8; border: 1px solid #363c4a; "
        "  padding: 6px 12px; font-size: 12px; font-weight: 500; "
        "}"
        "QPushButton#browseBtn:hover { background-color: #313744; color: #ffffff; }"
    );

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(22, 20, 22, 20);
    mainLayout->setSpacing(14);

    // Header
    QLabel* header = new QLabel("EXPORT PROJECT", this);
    header->setStyleSheet("color: #00e5ff; font-size: 11px; font-weight: bold; letter-spacing: 1px;");
    mainLayout->addWidget(header);

    QLabel* sub = new QLabel("Choose a code generation target and output folder.", this);
    sub->setStyleSheet("color: #7b889b; font-size: 12px; margin-bottom: 4px;");
    mainLayout->addWidget(sub);

    // Format Picker
    QGroupBox* fmtGroup = new QGroupBox("Target Framework", this);
    QVBoxLayout* fmtLayout = new QVBoxLayout(fmtGroup);
    fmtLayout->setSpacing(2);

    m_ugfxBtn = new QRadioButton("µGFX  —  Embedded C (open-source, royalty-free)", fmtGroup);
    m_ugfxBtn->setChecked(true);

    m_qulBtn = new QRadioButton("Qt for MCUs (QUL)  —  Qt Quick Ultralite (Qt Widgets replacement for MCUs)", fmtGroup);

    m_lvglBtn = new QRadioButton("LVGL (C/C++)  —  Light and Versatile Graphics Library (v8/v9)", fmtGroup);

    fmtLayout->addWidget(m_ugfxBtn);
    fmtLayout->addWidget(m_qulBtn);
    fmtLayout->addWidget(m_lvglBtn);
    mainLayout->addWidget(fmtGroup);

    // Output Folder
    QGroupBox* outGroup = new QGroupBox("Output Folder", this);
    QHBoxLayout* outLayout = new QHBoxLayout(outGroup);
    outLayout->setSpacing(8);

    m_folderEdit = new QLineEdit(outGroup);
    m_folderEdit->setPlaceholderText("Select output directory...");
    outLayout->addWidget(m_folderEdit, 1);

    QPushButton* browseBtn = new QPushButton("Browse...", outGroup);
    browseBtn->setObjectName("browseBtn");
    connect(browseBtn, &QPushButton::clicked, this, &ExportDialog::onBrowseFolder);
    outLayout->addWidget(browseBtn);
    mainLayout->addWidget(outGroup);

    // Info label
    QLabel* info = new QLabel(
        "<i>The Export button generates all files into the selected folder using the chosen framework's existing generator.</i>",
        this
    );
    info->setStyleSheet("color: #64748b; font-size: 11px;");
    info->setWordWrap(true);
    mainLayout->addWidget(info);

    // Buttons
    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addStretch(1);

    QPushButton* cancelBtn = new QPushButton("Cancel", this);
    cancelBtn->setObjectName("cancelBtn");
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(cancelBtn);

    QPushButton* exportBtn = new QPushButton("Export", this);
    exportBtn->setDefault(true);
    connect(exportBtn, &QPushButton::clicked, this, [this]() {
        if (m_folderEdit->text().trimmed().isEmpty()) {
            onBrowseFolder();
        }
        if (!m_folderEdit->text().trimmed().isEmpty()) {
            accept();
        }
    });
    btnLayout->addWidget(exportBtn);
    mainLayout->addLayout(btnLayout);
}

void ExportDialog::onBrowseFolder() {
    QString dir = QFileDialog::getExistingDirectory(
        this,
        "Choose Export Destination Folder",
        QString(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );
    if (!dir.isEmpty()) {
        m_folderEdit->setText(dir);
    }
}

QString ExportDialog::selectedTarget() const {
    if (m_qulBtn->isChecked())  return "qul";
    if (m_lvglBtn->isChecked()) return "lvgl";
    return "ugfx";
}

QString ExportDialog::outputFolder() const {
    return m_folderEdit->text().trimmed();
}

#include "AISettingsDialog.h"
#include "ai/AIProviderRegistry.h"
#include "ai/AIProvider.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QTabWidget>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QMessageBox>
#include <QGroupBox>

AISettingsDialog::AISettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("AI Providers & Settings");
    setMinimumSize(640, 500);
    setupUI();
}

void AISettingsDialog::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);

    // Global Routing Section
    auto* routingGroup = new QGroupBox("Routing & Preferences", this);
    auto* routingLayout = new QFormLayout(routingGroup);

    m_defaultProviderCombo = new QComboBox(this);
    m_fallbackProviderCombo = new QComboBox(this);
    m_fallbackProviderCombo->addItem("None (No Fallback)", "");

    auto& registry = AI::AIProviderRegistry::instance();
    const auto providers = registry.availableProviders();

    for (const auto& p : providers) {
        m_defaultProviderCombo->addItem(p->displayName(), p->providerId());
        m_fallbackProviderCombo->addItem(p->displayName(), p->providerId());
    }

    int defIdx = m_defaultProviderCombo->findData(registry.defaultProviderId());
    if (defIdx >= 0) m_defaultProviderCombo->setCurrentIndex(defIdx);

    int fbIdx = m_fallbackProviderCombo->findData(registry.fallbackProviderId());
    if (fbIdx >= 0) m_fallbackProviderCombo->setCurrentIndex(fbIdx);

    routingLayout->addRow("Default Provider:", m_defaultProviderCombo);
    routingLayout->addRow("Fallback Provider:", m_fallbackProviderCombo);
    mainLayout->addWidget(routingGroup);

    // Providers Tab Widget
    auto* tabWidget = new QTabWidget(this);
    for (const auto& p : providers) {
        tabWidget->addTab(createProviderTab(p->providerId()), p->displayName());
    }
    mainLayout->addWidget(tabWidget, 1);

    // Bottom Action Buttons
    auto* btnLayout = new QHBoxLayout();
    auto* clearBtn = new QPushButton("Clear Credentials", this);
    clearBtn->setToolTip("Securely wipes all stored API keys from this machine.");
    connect(clearBtn, &QPushButton::clicked, this, &AISettingsDialog::onClearCredentialsClicked);
    btnLayout->addWidget(clearBtn);

    btnLayout->addStretch();

    auto* cancelBtn = new QPushButton("Cancel", this);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(cancelBtn);

    auto* saveBtn = new QPushButton("Save & Close", this);
    saveBtn->setDefault(true);
    connect(saveBtn, &QPushButton::clicked, this, &AISettingsDialog::onSaveClicked);
    btnLayout->addWidget(saveBtn);

    mainLayout->addLayout(btnLayout);
}

QWidget* AISettingsDialog::createProviderTab(const QString& providerId) {
    auto* page = new QWidget(this);
    auto* layout = new QFormLayout(page);
    layout->setContentsMargins(15, 15, 15, 15);

    auto& registry = AI::AIProviderRegistry::instance();
    auto p = registry.getProvider(providerId);

    ProviderWidgets widgets;

    // Status label
    widgets.statusLabel = new QLabel(page);
    if (!p || !p->isConfigured()) {
        widgets.statusLabel->setText("Status: Not configured");
        widgets.statusLabel->setStyleSheet("font-weight: bold; color: #888888;");
    } else {
        widgets.statusLabel->setText("Status: Configured (Untested)");
        widgets.statusLabel->setStyleSheet("font-weight: bold; color: #dcdcaa;");
    }
    layout->addRow(widgets.statusLabel);

    // API Key field (masked)
    if (providerId != "mock") {
        auto* keyRow = new QWidget(page);
        auto* keyLayout = new QHBoxLayout(keyRow);
        keyLayout->setContentsMargins(0, 0, 0, 0);

        widgets.apiKeyEdit = new QLineEdit(page);
        widgets.apiKeyEdit->setEchoMode(QLineEdit::Password);
        if (p) widgets.apiKeyEdit->setText(p->apiKey());

        auto* toggleBtn = new QPushButton("Show", keyRow);
        toggleBtn->setFixedWidth(60);
        connect(toggleBtn, &QPushButton::clicked, this, [widgets, toggleBtn]() {
            if (widgets.apiKeyEdit->echoMode() == QLineEdit::Password) {
                widgets.apiKeyEdit->setEchoMode(QLineEdit::Normal);
                toggleBtn->setText("Hide");
            } else {
                widgets.apiKeyEdit->setEchoMode(QLineEdit::Password);
                toggleBtn->setText("Show");
            }
        });

        keyLayout->addWidget(widgets.apiKeyEdit, 1);
        keyLayout->addWidget(toggleBtn);
        layout->addRow("API Key:", keyRow);
    }

    // Base URL field for custom/local endpoints
    if (providerId == "custom" || providerId == "openai") {
        widgets.baseUrlEdit = new QLineEdit(page);
        if (p) widgets.baseUrlEdit->setText(p->baseUrl());
        widgets.baseUrlEdit->setPlaceholderText(providerId == "custom" ? "http://localhost:11434/v1" : "https://api.openai.com/v1 (default)");
        layout->addRow("Base URL:", widgets.baseUrlEdit);
    }

    // Model selection combo
    widgets.modelCombo = new QComboBox(page);
    widgets.modelCombo->setEditable(true);
    if (p) {
        widgets.modelCombo->addItems(p->cachedModels());
        widgets.modelCombo->setCurrentText(p->currentModel());
    }
    layout->addRow("Model:", widgets.modelCombo);

    // Test Connection Button
    widgets.testButton = new QPushButton("Test Connection", page);
    connect(widgets.testButton, &QPushButton::clicked, this, [this, providerId]() {
        onTestConnection(providerId);
    });
    layout->addRow("", widgets.testButton);

    m_providerWidgets[providerId] = widgets;
    return page;
}

void AISettingsDialog::updateProviderStatus(const QString& providerId, const QString& statusText, const QString& colorHex) {
    if (m_providerWidgets.contains(providerId)) {
        auto* lbl = m_providerWidgets[providerId].statusLabel;
        lbl->setText(QString("Status: %1").arg(statusText));
        lbl->setStyleSheet(QString("font-weight: bold; color: %1;").arg(colorHex));
    }
}

void AISettingsDialog::onTestConnection(const QString& providerId) {
    if (!m_providerWidgets.contains(providerId)) return;
    auto& widgets = m_providerWidgets[providerId];

    auto& registry = AI::AIProviderRegistry::instance();
    auto p = registry.getProvider(providerId);
    if (!p) return;

    QString key = widgets.apiKeyEdit ? widgets.apiKeyEdit->text().trimmed() : "";
    QString model = widgets.modelCombo ? widgets.modelCombo->currentText().trimmed() : "";
    QString baseUrl = widgets.baseUrlEdit ? widgets.baseUrlEdit->text().trimmed() : "";

    p->configure(key, model, baseUrl);

    widgets.testButton->setEnabled(false);
    updateProviderStatus(providerId, "Testing connection...", "#dcdcaa");

    p->testConnection([this, providerId](const AI::ConnectionTestResult& result) {
        if (!m_providerWidgets.contains(providerId)) return;
        auto& w = m_providerWidgets[providerId];
        w.testButton->setEnabled(true);

        QString color = "#888888";
        switch (result.status) {
            case AI::ConnectionStatus::Connected:
                color = "#4ec9b0"; // Green
                break;
            case AI::ConnectionStatus::AuthenticationFailed:
            case AI::ConnectionStatus::InvalidModel:
                color = "#f44747"; // Red
                break;
            case AI::ConnectionStatus::RateLimited:
            case AI::ConnectionStatus::NetworkError:
            case AI::ConnectionStatus::ProviderUnavailable:
                color = "#ce9178"; // Orange/Yellow
                break;
            case AI::ConnectionStatus::NotConfigured:
                color = "#888888"; // Gray
                break;
        }

        updateProviderStatus(providerId, QString("%1 (%2)").arg(AI::connectionStatusToString(result.status), result.message), color);

        if (!result.detectedModels.isEmpty()) {
            w.modelCombo->clear();
            w.modelCombo->addItems(result.detectedModels);
            if (!result.detectedModels.isEmpty()) {
                w.modelCombo->setCurrentText(result.detectedModels.first());
            }
        }
    });
}

void AISettingsDialog::onSaveClicked() {
    auto& registry = AI::AIProviderRegistry::instance();

    // Save default and fallback
    registry.setDefaultProviderId(m_defaultProviderCombo->currentData().toString());
    registry.setFallbackProviderId(m_fallbackProviderCombo->currentData().toString());

    // Update each provider
    for (auto it = m_providerWidgets.begin(); it != m_providerWidgets.end(); ++it) {
        QString pid = it.key();
        auto p = registry.getProvider(pid);
        if (p) {
            QString key = it.value().apiKeyEdit ? it.value().apiKeyEdit->text().trimmed() : p->apiKey();
            QString model = it.value().modelCombo ? it.value().modelCombo->currentText().trimmed() : p->currentModel();
            QString baseUrl = it.value().baseUrlEdit ? it.value().baseUrlEdit->text().trimmed() : p->baseUrl();
            p->configure(key, model, baseUrl);
        }
    }

    registry.saveSettings();
    accept();
}

void AISettingsDialog::onClearCredentialsClicked() {
    auto ret = QMessageBox::question(this, "Clear All Credentials",
        "Are you sure you want to clear all stored AI API keys? This cannot be undone.",
        QMessageBox::Yes | QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        auto& registry = AI::AIProviderRegistry::instance();
        registry.clearAllCredentials();
        for (auto it = m_providerWidgets.begin(); it != m_providerWidgets.end(); ++it) {
            if (it.value().apiKeyEdit) it.value().apiKeyEdit->clear();
            updateProviderStatus(it.key(), "Not configured", "#888888");
        }
    }
}

#pragma once

#include <QDialog>
#include <QMap>
#include <memory>

class QComboBox;
class QLineEdit;
class QLabel;
class QPushButton;
class QTabWidget;

namespace AI {
    class AIProvider;
}

class AISettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit AISettingsDialog(QWidget* parent = nullptr);
    ~AISettingsDialog() override = default;

private slots:
    void onSaveClicked();
    void onClearCredentialsClicked();
    void onTestConnection(const QString& providerId);

private:
    void setupUI();
    QWidget* createProviderTab(const QString& providerId);
    void updateProviderStatus(const QString& providerId, const QString& statusText, const QString& colorHex);

    QComboBox* m_defaultProviderCombo = nullptr;
    QComboBox* m_fallbackProviderCombo = nullptr;

    struct ProviderWidgets {
        QLabel* statusLabel = nullptr;
        QLineEdit* apiKeyEdit = nullptr;
        QLineEdit* baseUrlEdit = nullptr;
        QComboBox* modelCombo = nullptr;
        QPushButton* testButton = nullptr;
    };

    QMap<QString, ProviderWidgets> m_providerWidgets;
};

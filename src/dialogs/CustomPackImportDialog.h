#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>
#include "driverpack/CustomPackModel.h"

namespace driverpack {
class CustomPackManager;
}

class CustomPackImportDialog : public QDialog {
    Q_OBJECT

public:
    explicit CustomPackImportDialog(QWidget* parent = nullptr);
    ~CustomPackImportDialog() override = default;

    driverpack::CustomPackDef importedPack() const { return m_pack; }

private slots:
    void onBrowseSvd();
    void onImportCsvPins();
    void onBrowseSdk();
    void onVerifySdk();
    void onInstallPack();

private:
    void setupUi();
    void updatePinTable();
    void validateAll();

    // Identity widgets
    QLineEdit* m_nameEdit = nullptr;
    QLineEdit* m_vendorEdit = nullptr;
    QLineEdit* m_partNumberEdit = nullptr;
    QLineEdit* m_familyEdit = nullptr;
    QLineEdit* m_coreEdit = nullptr;
    QSpinBox* m_clockSpin = nullptr;

    // SVD widgets
    QLineEdit* m_svdPathEdit = nullptr;
    QLabel* m_svdStatusLabel = nullptr;

    // Pin table widgets
    QTableWidget* m_pinTable = nullptr;
    QLabel* m_pinStatusLabel = nullptr;

    // SDK widgets
    QLineEdit* m_sdkPathEdit = nullptr;
    QLabel* m_sdkStatusLabel = nullptr;

    // Overall status
    QLabel* m_statusLabel = nullptr;
    QPushButton* m_installButton = nullptr;

    driverpack::CustomPackDef m_pack;
};

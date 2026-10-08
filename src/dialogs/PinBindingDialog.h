#pragma once

#include <QDialog>
#include <QTableWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QSlider>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QGroupBox>
#include "HardwareBridge.h"

class CanvasScene;

class PinBindingDialog : public QDialog {
    Q_OBJECT

public:
    explicit PinBindingDialog(CanvasScene* scene, QWidget* parent = nullptr);
    ~PinBindingDialog() override = default;

    void refreshTable();

private slots:
    void onBoardChanged(int index);
    void onPinModeChanged(int row, int index);
    void onComponentBindingChanged(int row, int index);
    void onPotentiometerSliderChanged(int value);
    void onSpiTransferClicked();
    void onHardwareBridgeUpdate();
    void onImportBoardConfigClicked();

private:
    void setupUi();
    void updatePotentiometerSection();
    void updateStatusBadge();

    CanvasScene* m_scene = nullptr;
    HardwareBridge& m_bridge;

    // UI Widgets
    QComboBox* m_boardCombo = nullptr;
    QLabel* m_statusBadge = nullptr;
    QPushButton* m_connectButton = nullptr;
    QTableWidget* m_pinTable = nullptr;

    // ADC Simulator
    QGroupBox* m_adcGroup = nullptr;
    QComboBox* m_adcPinSelect = nullptr;
    QSlider* m_potentiometerSlider = nullptr;
    QLabel* m_adcRawCountLabel = nullptr;

    // SPI Raw Transfer Panel
    QGroupBox* m_spiGroup = nullptr;
    QComboBox* m_spiBusCombo = nullptr;
    QSpinBox* m_spiByteOutSpin = nullptr;
    QPushButton* m_spiTransferButton = nullptr;
    QLabel* m_spiByteInLabel = nullptr;
    QTextEdit* m_spiLogEdit = nullptr;

    QPushButton* m_livePollButton = nullptr;
};

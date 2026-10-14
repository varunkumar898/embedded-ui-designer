#pragma once

#include <QDialog>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QTextBrowser>
#include <QTableWidget>
#include "BoardDatabase.h"
#include "BoardDetector.h"

/**
 * @brief BoardSelectorDialog provides an interactive board browser, specification viewer,
 * datasheet launcher, and OpenOCD configuration selector.
 */
class BoardSelectorDialog : public QDialog {
    Q_OBJECT

public:
    explicit BoardSelectorDialog(QWidget* parent = nullptr);
    ~BoardSelectorDialog() override = default;

    BoardSpec selectedBoard() const { return m_selectedBoard; }
    void selectBoardById(const QString& id);

signals:
    void boardSelected(const BoardSpec& board);

private slots:
    void onBoardIndexChanged(int index);
    void onViewDatasheet();
    void onAutoDetectClicked();

private:
    void setupUi();
    void populateBoards();
    void updateSpecDisplay(const BoardSpec& spec);

    QComboBox* m_boardCombo = nullptr;
    QPushButton* m_autoDetectBtn = nullptr;
    QPushButton* m_datasheetBtn = nullptr;
    QLabel* m_detectedStatusLabel = nullptr;

    // Spec display widgets
    QLabel* m_nameLabel = nullptr;
    QLabel* m_mcuLabel = nullptr;
    QLabel* m_archLabel = nullptr;
    QLabel* m_freqLabel = nullptr;
    QLabel* m_flashLabel = nullptr;
    QLabel* m_ramLabel = nullptr;
    QLabel* m_gpioLabel = nullptr;
    QLabel* m_adapterLabel = nullptr;
    QLabel* m_openocdConfigLabel = nullptr;
    QLabel* m_toolchainLabel = nullptr;
    QTextBrowser* m_pinsBrowser = nullptr;

    BoardSpec m_selectedBoard;
    BoardDetector m_detector;
};

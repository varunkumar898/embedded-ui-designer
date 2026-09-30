#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QTabWidget>
#include <QListWidget>
#include <QSpinBox>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QList>
#include "DisplayConfig.h"

struct BoardDeviceInfo {
    QString name;
    QString manufacturer;
    QString architecture;
    int width = 320;
    int height = 240;
    QString displayType = "LCD";
    QString displayInterface;
    int ramKb = 0;
    int flashKb = 0;
};

class NewProjectDialog : public QDialog {
    Q_OBJECT

public:
    explicit NewProjectDialog(QWidget* parent = nullptr);

    QString projectName() const;
    DisplayConfig displayConfig() const;
    bool isBoardMode() const;
    QString selectedBoardName() const;

    void selectBoard(const QString& boardName);
    void setCustomDisplay(int width, int height, int colorDepth = 16, const QString& type = "LCD");
    void setProjectName(const QString& name);

private slots:
    void onBoardSelectionChanged();
    void onModeTabChanged(int index);

private:
    QLineEdit* m_nameEdit = nullptr;
    QTabWidget* m_tabWidget = nullptr;

    // Board Tab
    QListWidget* m_boardList = nullptr;
    QLabel* m_boardDetailsLabel = nullptr;
    QList<BoardDeviceInfo> m_boards;

    // Custom Tab
    QSpinBox* m_widthSpin = nullptr;
    QSpinBox* m_heightSpin = nullptr;
    QComboBox* m_colorDepthCombo = nullptr;
    QComboBox* m_displayTypeCombo = nullptr;

    void setupUi();
    void loadDevices();
    void updateBoardDetails();
};

#pragma once

#include <QDialog>
#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QList>
#include "HardwareModel.h"
#include "HardwareTarget.h"

class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QSpinBox;
class QTabWidget;
class QGroupBox;
class QPushButton;
class QFrame;

struct TargetBoardInfo {
    QString id;
    QString displayName;
    QString family;
    QString mcuModel;
    QString boardName;
    QString architecture;
    quint64 flashBytes = 0;
    quint64 ramBytes = 0;
    int gpioCount = 0;
    QString debugInterface;
    QStringList supportedFrameworks;
    QString displayRecommendation;
    int defaultWidth = 320;
    int defaultHeight = 240;
    bool defaultRound = false;
    QString defaultFramework = "lvgl";
    QString backendType = "stm32";
    QString connectionType = "openocd";
};

class NewProjectDialog : public QDialog {
    Q_OBJECT

public:
    explicit NewProjectDialog(QWidget* parent = nullptr);
    ~NewProjectDialog() override = default;

    bool startsFromTemplate() const;
    QString templateResource() const;
    QString projectName() const;
    int projectWidth() const;
    int projectHeight() const;
    QString targetFramework() const;
    int colorDepth() const;
    bool roundDisplay() const;
    QString targetPlatform() const;
    QString hardwareFamily() const { return targetPlatform(); }
    QString hardwareBoard() const;
    QString selectedBoardId() const;

    // Hardware configuration & target models
    Hardware::HardwareConfig hardwareConfiguration() const;
    Hardware::HardwareTarget hardwareTarget() const;

    // Profile database access
    static QList<TargetBoardInfo> allBoardProfiles();
    static TargetBoardInfo findBoardInfo(const QString& boardId);

public slots:
    void setSelectedPlatform(const QString& family);
    void setSelectedBoard(const QString& boardId);
    void setTargetFramework(const QString& framework);

private slots:
    void onPlatformChanged();
    void onBoardChanged();
    void onFrameworkChanged();
    void onToggleProfileSummary();
    void updateTemplateDetails();
    void validateFrameworkCompatibility();

private:
    void setupUi();
    void populatePlatforms();
    void populateBoardsForCurrentPlatform();
    void updateProfileSummary();

    // Top Level fields
    QLineEdit* m_projectNameEdit = nullptr;
    QComboBox* m_platformCombo = nullptr;
    QComboBox* m_boardCombo = nullptr;

    // Dynamic Profile Summary
    QFrame* m_summaryFrame = nullptr;
    QLabel* m_summaryTargetLabel = nullptr;
    QLabel* m_summaryArchLabel = nullptr;
    QLabel* m_summaryMemoryLabel = nullptr;
    QLabel* m_summaryDebugLabel = nullptr;
    QLabel* m_summaryFrameworksLabel = nullptr;
    QLabel* m_summaryDisplayLabel = nullptr;
    QPushButton* m_summaryToggleBtn = nullptr;
    QWidget* m_summaryDetailsContainer = nullptr;
    bool m_summaryExpanded = true;

    // Custom Target Configuration
    QGroupBox* m_customTargetGroup = nullptr;
    QLineEdit* m_customMcuEdit = nullptr;
    QComboBox* m_customArchCombo = nullptr;
    QSpinBox* m_customFlashSpin = nullptr;
    QSpinBox* m_customRamSpin = nullptr;
    QSpinBox* m_customGpioSpin = nullptr;
    QComboBox* m_customDebugCombo = nullptr;

    // Tabs
    QTabWidget* m_tabs = nullptr;

    // Blank Project page
    QSpinBox* m_widthSpin = nullptr;
    QSpinBox* m_heightSpin = nullptr;
    QSpinBox* m_colorDepthSpin = nullptr;
    QComboBox* m_shapeCombo = nullptr;
    QComboBox* m_frameworkCombo = nullptr;
    QLabel* m_compatibilityBanner = nullptr;

    // Template page
    QListWidget* m_templateList = nullptr;
    QLabel* m_templateDescription = nullptr;
    QLabel* m_templateCompatibilityLabel = nullptr;
    QString m_suggestedName;
};
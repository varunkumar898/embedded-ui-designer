#pragma once

#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include <QComboBox>
#include <QListWidget>
#include <QPushButton>
#include <QGroupBox>
#include <QVBoxLayout>
#include "ResourceEstimator.h"
#include "ResourceAnalysis.h"
#include "CapabilityDatabase.h"

class Project;
class UIComponent;

class ResourceAnalyzerPanel : public QWidget {
    Q_OBJECT

public:
    explicit ResourceAnalyzerPanel(QWidget* parent = nullptr);
    ~ResourceAnalyzerPanel() override = default;

    void setProject(Project* project);
    void setDevice(const DeviceProfile& device);
    void setDevice(const QString& deviceName);

    ResourceAnalysis lastAnalysis() const { return m_last_analysis; }
    ResourceEstimator& estimator() { return m_estimator; }

public slots:
    void refresh();
    // Live Test Mode: the panel must stay visible; only a banner is toggled.
    void setLiveTestMode(bool enabled);

    bool isLiveTestMode() const { return m_liveTestMode; }
    QString liveTestBannerText() const;

private slots:
    void onProjectModified();
    void onDeviceComboChanged(int index);

private:
    void setupUi();
    void updateUI(const ResourceAnalysis& analysis);
    void updateLiveTestBanner();

    ResourceEstimator m_estimator;
    Project* m_project = nullptr;
    DeviceProfile m_device;
    ResourceAnalysis m_last_analysis;
    bool m_updatingCombo = false;
    bool m_liveTestMode = false;

    // UI elements
    QLabel* m_liveTestBannerLabel = nullptr;
    QComboBox* m_deviceCombo = nullptr;
    QLabel* m_deviceInfoLabel = nullptr;
    QLabel* m_statusBadge = nullptr;

    QProgressBar* m_flashBar = nullptr;
    QLabel* m_flashDetailsLabel = nullptr;

    QProgressBar* m_ramBar = nullptr;
    QLabel* m_ramDetailsLabel = nullptr;

    QProgressBar* m_cpuBar = nullptr;
    QLabel* m_cpuDetailsLabel = nullptr;

    QLabel* m_breakdownLabel = nullptr;
    QListWidget* m_warningsList = nullptr;
    QPushButton* m_refreshBtn = nullptr;
};

#pragma once

#include <QObject>
#include <QVector>
#include <QString>
#include "ResourceAnalysis.h"
#include "CapabilityDatabase.h"
#include "EstimationCache.h"
#include "EstimationFormulas.h"
#include "hardware/DeviceDatabase.h"

class Project;
class UIComponent;

class ResourceEstimator : public QObject {
    Q_OBJECT

public:
    explicit ResourceEstimator(QObject* parent = nullptr);
    ~ResourceEstimator() override = default;

    // Device database access (prefers Hardware::DeviceDatabase, falls back to CapabilityDatabase)
    CapabilityDatabase& capabilityDatabase() { return m_db; }
    const CapabilityDatabase& capabilityDatabase() const { return m_db; }
    Hardware::DeviceDatabase& hardwareDatabase() { return Hardware::DeviceDatabase::instance(); }
    const Hardware::DeviceDatabase& hardwareDatabase() const { return Hardware::DeviceDatabase::instance(); }

    // Main analysis entry point
    ResourceAnalysis analyze(const Project* project, const DeviceProfile& device, const QString& framework = QString());
    ResourceAnalysis analyze(const Project* project, const QString& deviceName = QString());
    ResourceAnalysis analyzeProject(const Project* project, const DeviceProfile& device, const QString& framework = QString()) {
        return analyze(project, device, framework);
    }
    ResourceAnalysis analyzeProject(const Project* project, const QString& framework = QString());

    // Heaviest components breakdown for optimization
    struct ComponentResourceEntry {
        QString componentType;
        int count = 0;
        uint32_t totalFlashBytes = 0;
        uint32_t totalRamBytes = 0;
        QString optimizationSuggestion;
    };
    QVector<ComponentResourceEntry> getHeaviestComponents(const Project* project) const;

    // Component-level analysis
    ResourceAnalysis::Breakdown analyzeComponent(const UIComponent* comp);
    ResourceAnalysis::Breakdown analyzeAllComponents(const Project* project);

    // Framework overhead
    uint32_t analyzeFramework(const QString& target, int num_components, int num_screens);

    // Assets (images, fonts)
    uint32_t analyzeAssets(const Project* project, uint32_t* ramAssetsOut = nullptr);

    // Framebuffer calculations (with DMA alignment)
    static size_t calculateFramebufferBytes(int width, int height, int bpp, int bufferCount, size_t dmaAlignment = 16);
    static size_t calculateRowStrideBytes(int width, int bpp, size_t dmaAlignment = 16);

    // Validation
    void validateFitsOnDevice(ResourceAnalysis& analysis);
    void validateFitsOnDevice(const ResourceAnalysis& analysis,
                              ResourceAnalysis::Status& status_out,
                              QVector<QString>& warnings_out);
    void clearCache() { m_cache.clear(); }

    // Convert Hardware::DeviceDefinition to DeviceProfile
    static DeviceProfile deviceDefinitionToProfile(const Hardware::DeviceDefinition& dev);

    // ========================================================================
    // Stage 4.5: Resource-Based MCU Compatibility Evaluation for EmbedWeb
    // ========================================================================
    struct EmbedWebTargetEvaluation {
        QString targetName;
        QString family;
        QString architecture;
        uint32_t flashAvailableKb = 0;
        uint32_t flashRequiredKb = 0;
        uint32_t internalSramAvailableKb = 0;
        uint32_t internalSramRequiredKb = 0;
        uint32_t psramAvailableKb = 0;
        uint32_t psramRequiredKb = 0;
        uint32_t dmaSliceBufferBytes = 0;
        uint32_t treeStorageBytes = 0;
        uint32_t coreAndStackBytes = 0;
        uint8_t bufferCount = 2;
        bool treePlacedInPsram = false;

        // Orthogonal status 1: Resource Placement & Capacity
        enum class ResourceStatus {
            ResourceCompatible,
            ResourceCompatibleWithWarnings,
            InsufficientFlash,
            InsufficientInternalSRAM,
            InsufficientPSRAM,
            MissingPeripherals
        };
        ResourceStatus resourceStatus = ResourceStatus::ResourceCompatible;

        // Orthogonal status 2: Target Toolchain & Build Status
        enum class BuildStatus {
            BuildNotAttempted,
            CompilerObjectsBuilt,
            TargetApplicationLinked,
            BuildFailed,
            ToolchainUnavailable
        };
        BuildStatus buildStatus = BuildStatus::BuildNotAttempted;

        // Orthogonal status 3: Physical Hardware Validation Status
        enum class HardwareStatus {
            HardwareNotTested,
            PhysicalDisplayVerified,
            PhysicalInputVerified,
            FullUiVerified
        };
        HardwareStatus hardwareStatus = HardwareStatus::HardwareNotTested;

        // Backward compatibility enum for existing callers
        enum Compatibility {
            Compatible,
            CompatibleWithWarnings,
            InsufficientFlash,
            InsufficientRAM,
            MissingPeripherals,
            MissingToolchain,
            AdapterNotImplemented,
            NotValidated
        };
        Compatibility compatibility = Compatible;
        QString statusText;
        QString reason;
        QStringList details;
        bool toolchainAvailable = false;
        QString toolchainName;
        float safetyMarginApplied = 20.0f;
    };

    QVector<EmbedWebTargetEvaluation> evaluateEmbedWebTargets(const Project* project,
                                                             float safetyMarginPercent = 20.0f,
                                                             uint8_t bufferCount = 2,
                                                             bool allowTreeInPsram = false) const;
    static bool isToolchainAvailable(const QString& toolchain);

signals:
    void analysisUpdated(const ResourceAnalysis& analysis);

private:
    EstimationCache m_cache;
    CapabilityDatabase m_db;
};
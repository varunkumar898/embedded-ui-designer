#pragma once

#include <QString>
#include <QStringList>
#include <QMap>
#include <QJsonObject>
#include <QJsonArray>
#include "HardwareCapabilities.h"
#include "HardwareModel.h"

namespace Hardware {

/**
 * @brief Represents an active or configured hardware target device/board.
 * Decouples project configuration from concrete connection/probe details.
 */
class HardwareTarget {
public:
    HardwareTarget() = default;
    HardwareTarget(const QString& id, const QString& name, const QString& family,
                   const QString& mcuModel = QString(), const QString& boardId = QString());

    QString id() const { return m_id; }
    void setId(const QString& id) { m_id = id; }

    QString name() const { return m_name; }
    void setName(const QString& name) { m_name = name; }

    QString family() const { return m_family; } // "STM32", "ESP32", "Raspberry Pi", "Custom", "Mock"
    void setFamily(const QString& fam) { m_family = fam; }

    QString mcuModel() const { return m_mcuModel; }
    void setMcuModel(const QString& mcu) { m_mcuModel = mcu; }

    QString boardId() const { return m_boardId; }
    void setBoardId(const QString& id) { m_boardId = id; }

    QString boardName() const { return m_boardName; }
    void setBoardName(const QString& name) { m_boardName = name; }

    QString architecture() const { return m_architecture; }
    void setArchitecture(const QString& arch) { m_architecture = arch; }

    QString backendType() const { return m_backendType; } // "stm32", "esp32", "raspberrypi", "mock", "custom"
    void setBackendType(const QString& type) { m_backendType = type; }

    QString connectionType() const { return m_connectionType; } // "openocd", "serial", "mock", "linux"
    void setConnectionType(const QString& conn) { m_connectionType = conn; }

    HardwareCapabilities capabilities() const { return m_capabilities; }
    void setCapabilities(const HardwareCapabilities& caps) { m_capabilities = caps; }

    QMap<QString, QString> pinMappings() const { return m_pinMappings; }
    void setPinMapping(const QString& logicalName, const QString& physicalPin);
    QString resolvePin(const QString& logicalName) const;

    QJsonObject metadata() const { return m_metadata; }
    void setMetadata(const QJsonObject& meta) { m_metadata = meta; }

    bool isValid() const { return !m_id.isEmpty() && !m_family.isEmpty(); }

    QJsonObject toJson() const;
    static HardwareTarget fromJson(const QJsonObject& json);

private:
    QString m_id;
    QString m_name;
    QString m_family;
    QString m_mcuModel;
    QString m_boardId;
    QString m_boardName;
    QString m_architecture;
    QString m_backendType = "mock";
    QString m_connectionType = "mock";
    HardwareCapabilities m_capabilities;
    QMap<QString, QString> m_pinMappings; // e.g. "STATUS_LED" -> "PA5"
    QJsonObject m_metadata;
};

} // namespace Hardware

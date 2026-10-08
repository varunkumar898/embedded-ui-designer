#include "HardwareTarget.h"

namespace Hardware {

HardwareTarget::HardwareTarget(const QString& id, const QString& name, const QString& family,
                               const QString& mcuModel, const QString& boardId)
    : m_id(id)
    , m_name(name)
    , m_family(family)
    , m_mcuModel(mcuModel)
    , m_boardId(boardId)
{
    if (family.compare("STM32", Qt::CaseInsensitive) == 0) {
        m_backendType = "stm32";
        m_connectionType = "openocd";
        m_architecture = "ARM Cortex-M";
        m_capabilities.gpioInput = true;
        m_capabilities.gpioOutput = true;
        m_capabilities.adc = true;
        m_capabilities.pwm = true;
        m_capabilities.uart = true;
        m_capabilities.spi = true;
        m_capabilities.i2c = true;
        m_capabilities.reset = true;
        m_capabilities.liveMonitoring = true;
        m_capabilities.atomicBsrr = true;
    } else if (family.compare("ESP32", Qt::CaseInsensitive) == 0) {
        m_backendType = "esp32";
        m_connectionType = "serial";
        m_architecture = "Xtensa / RISC-V";
        m_capabilities.gpioInput = true;
        m_capabilities.gpioOutput = true;
        m_capabilities.adc = true;
        m_capabilities.pwm = true;
        m_capabilities.uart = true;
        m_capabilities.reset = true;
        m_capabilities.liveMonitoring = true;
    } else if (family.compare("Raspberry Pi", Qt::CaseInsensitive) == 0 ||
               family.compare("RaspberryPi", Qt::CaseInsensitive) == 0) {
        m_backendType = "raspberrypi";
        m_connectionType = "linux";
        m_architecture = "ARM Cortex-A";
        m_capabilities.gpioInput = true;
        m_capabilities.gpioOutput = true;
        m_capabilities.uart = true;
        m_capabilities.spi = true;
        m_capabilities.i2c = true;
        m_capabilities.liveMonitoring = true;
    } else {
        m_backendType = "mock";
        m_connectionType = "mock";
        m_capabilities.gpioInput = true;
        m_capabilities.gpioOutput = true;
        m_capabilities.adc = true;
        m_capabilities.pwm = true;
        m_capabilities.reset = true;
    }
}

void HardwareTarget::setPinMapping(const QString& logicalName, const QString& physicalPin) {
    m_pinMappings[logicalName] = physicalPin;
}

QString HardwareTarget::resolvePin(const QString& logicalName) const {
    if (m_pinMappings.contains(logicalName)) {
        return m_pinMappings.value(logicalName);
    }
    return logicalName; // fallback if physical pin name directly given
}

QJsonObject HardwareTarget::toJson() const {
    QJsonObject json;
    json["id"] = m_id;
    json["name"] = m_name;
    json["family"] = m_family;
    json["mcuModel"] = m_mcuModel;
    json["boardId"] = m_boardId;
    json["boardName"] = m_boardName;
    json["architecture"] = m_architecture;
    json["backendType"] = m_backendType;
    json["connectionType"] = m_connectionType;
    json["capabilities"] = m_capabilities.toJson();

    QJsonObject pinsObj;
    for (auto it = m_pinMappings.begin(); it != m_pinMappings.end(); ++it) {
        pinsObj[it.key()] = it.value();
    }
    json["pinMappings"] = pinsObj;
    json["metadata"] = m_metadata;

    return json;
}

HardwareTarget HardwareTarget::fromJson(const QJsonObject& json) {
    HardwareTarget target;
    target.setId(json.value("id").toString());
    target.setName(json.value("name").toString());
    target.setFamily(json.value("family").toString());
    target.setMcuModel(json.value("mcuModel").toString());
    target.setBoardId(json.value("boardId").toString());
    target.setBoardName(json.value("boardName").toString());
    target.setArchitecture(json.value("architecture").toString());
    target.setBackendType(json.value("backendType").toString("mock"));
    target.setConnectionType(json.value("connectionType").toString("mock"));

    if (json.contains("capabilities")) {
        target.setCapabilities(HardwareCapabilities::fromJson(json.value("capabilities").toObject()));
    }

    if (json.contains("pinMappings") && json.value("pinMappings").isObject()) {
        QJsonObject pinsObj = json.value("pinMappings").toObject();
        for (auto it = pinsObj.begin(); it != pinsObj.end(); ++it) {
            target.setPinMapping(it.key(), it.value().toString());
        }
    }

    if (json.contains("metadata")) {
        target.setMetadata(json.value("metadata").toObject());
    }

    return target;
}

} // namespace Hardware

#pragma once

#include <QString>
#include <QList>
#include <QMap>
#include <memory>
#include "HardwareModel.h"

namespace Hardware {

class IHardwareProvider {
public:
    virtual ~IHardwareProvider() = default;

    virtual QString id() const = 0;
    virtual QString name() const = 0;
    virtual QString vendor() const = 0;

    virtual QList<DeviceDefinition> devices() const = 0;
    virtual QList<BoardDefinition> boards() const = 0;

    virtual DeviceDefinition findDevice(const QString& partNumber) const = 0;
    virtual BoardDefinition findBoard(const QString& boardId) const = 0;

    virtual QList<ToolchainConfiguration> supportedToolchains() const = 0;
    virtual bool loadFromDirectory(const QString& dirPath) = 0;
    virtual void addDevice(const DeviceDefinition& device) = 0;
    virtual void addBoard(const BoardDefinition& board) = 0;
};

class GenericPackProvider : public IHardwareProvider {
public:
    GenericPackProvider(const QString& id, const QString& name, const QString& vendor);
    ~GenericPackProvider() override = default;

    QString id() const override { return m_id; }
    QString name() const override { return m_name; }
    QString vendor() const override { return m_vendor; }

    QList<DeviceDefinition> devices() const override { return m_devices.values(); }
    QList<BoardDefinition> boards() const override { return m_boards.values(); }

    DeviceDefinition findDevice(const QString& partNumber) const override;
    BoardDefinition findBoard(const QString& boardId) const override;

    QList<ToolchainConfiguration> supportedToolchains() const override { return m_toolchains; }
    void setSupportedToolchains(const QList<ToolchainConfiguration>& tc) { m_toolchains = tc; }

    bool loadFromDirectory(const QString& dirPath) override;
    void addDevice(const DeviceDefinition& device) override;
    void addBoard(const BoardDefinition& board) override;

protected:
    QString m_id;
    QString m_name;
    QString m_vendor;
    QMap<QString, DeviceDefinition> m_devices;
    QMap<QString, BoardDefinition> m_boards;
    QList<ToolchainConfiguration> m_toolchains;
};

class Stm32Provider : public GenericPackProvider {
public:
    Stm32Provider();
};

class Esp32Provider : public GenericPackProvider {
public:
    Esp32Provider();
};

class Rp2040Provider : public GenericPackProvider {
public:
    Rp2040Provider();
};

class RaspberryPiProvider : public GenericPackProvider {
public:
    RaspberryPiProvider();
};

class CustomHardwareProvider : public GenericPackProvider {
public:
    CustomHardwareProvider();
    void removeDevice(const QString& partNumber);
    void removeBoard(const QString& boardId);
};

} // namespace Hardware

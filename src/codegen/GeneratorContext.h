#pragma once

#include <QString>
#include <QStringList>
#include <QList>
#include <QMap>
#include <memory>
#include "Project.h"
#include "Screen.h"
#include "DataSource.h"
#include "DataBinding.h"
#include "HardwareTarget.h"
#include "DisplayConfig.h"

namespace CodeGen {

struct ValidationMessage {
    enum Level { Info, Warning, Error };
    Level level;
    QString message;
    QString componentId;
    QString screenId;
};

/**
 * @brief GeneratorContext provides clean, read-only access to the Project model,
 * multi-screen hierarchy, DataSources, DataBindings, and HardwareTarget for code generators.
 */
class GeneratorContext {
public:
    explicit GeneratorContext(const Project* project);
    ~GeneratorContext() = default;

    const Project* project() const { return m_project; }
    QString projectName() const;
    QString targetFramework() const;
    DisplayConfig displayConfig() const;

    // Multi-Screen access
    const QList<Screen*>& screens() const;
    Screen* activeScreen() const;
    Screen* findScreen(const QString& screenId) const;

    // Data Sources & Bindings
    const QList<DataSource>& dataSources() const;
    const QList<DataBinding>& dataBindings() const;
    const DataSource* findDataSource(const QString& sourceId) const;
    QList<DataBinding> bindingsForComponent(const QString& compId) const;

    // Target Hardware
    Hardware::HardwareTarget hardwareTarget() const;
    QString targetFamily() const;
    QString targetBoard() const;

    // Validation
    bool validate(QList<ValidationMessage>* outMessages = nullptr) const;
    bool hasErrors() const;

private:
    const Project* m_project = nullptr;
};

} // namespace CodeGen

#include "GeneratorContext.h"
#include <QSet>

namespace CodeGen {

GeneratorContext::GeneratorContext(const Project* project)
    : m_project(project)
{
}

QString GeneratorContext::projectName() const {
    return m_project ? m_project->projectName() : "EmbeddedApp";
}

QString GeneratorContext::targetFramework() const {
    return m_project ? m_project->targetFramework() : "lvgl";
}

DisplayConfig GeneratorContext::displayConfig() const {
    return m_project ? m_project->displayConfig() : DisplayConfig();
}

const QList<Screen*>& GeneratorContext::screens() const {
    static const QList<Screen*> emptyList;
    return m_project ? m_project->screens() : emptyList;
}

Screen* GeneratorContext::activeScreen() const {
    return m_project ? m_project->activeScreen() : nullptr;
}

Screen* GeneratorContext::findScreen(const QString& screenId) const {
    return m_project ? m_project->findScreen(screenId) : nullptr;
}

const QList<DataSource>& GeneratorContext::dataSources() const {
    static const QList<DataSource> emptyDs;
    return m_project ? m_project->dataSources() : emptyDs;
}

const QList<DataBinding>& GeneratorContext::dataBindings() const {
    static const QList<DataBinding> emptyDb;
    return m_project ? m_project->dataBindings() : emptyDb;
}

const DataSource* GeneratorContext::findDataSource(const QString& sourceId) const {
    return m_project ? m_project->findDataSource(sourceId) : nullptr;
}

QList<DataBinding> GeneratorContext::bindingsForComponent(const QString& compId) const {
    QList<DataBinding> result;
    if (!m_project) return result;
    for (const auto& b : m_project->dataBindings()) {
        if (b.componentId() == compId) {
            result.append(b);
        }
    }
    return result;
}

Hardware::HardwareTarget GeneratorContext::hardwareTarget() const {
    if (!m_project) return Hardware::HardwareTarget();
    Hardware::HardwareConfig hw = m_project->hardwareConfig();
    Hardware::HardwareTarget target(hw.boardId.isEmpty() ? hw.deviceId : hw.boardId,
                                    hw.boardId.isEmpty() ? hw.deviceId : hw.boardId,
                                    hw.family.isEmpty() ? "STM32" : hw.family,
                                    hw.deviceId, hw.boardId);
    target.setArchitecture(hw.architecture);
    return target;
}

QString GeneratorContext::targetFamily() const {
    return m_project ? m_project->hardwareConfig().family : "STM32";
}

QString GeneratorContext::targetBoard() const {
    return m_project ? m_project->hardwareConfig().boardId : "";
}

bool GeneratorContext::validate(QList<ValidationMessage>* outMessages) const {
    bool ok = true;
    if (!m_project) {
        if (outMessages) {
            outMessages->append({ValidationMessage::Error, "Project pointer is null", "", ""});
        }
        return false;
    }

    QSet<QString> componentIds;
    QSet<QString> screenIds;

    // Validate screens & components
    for (Screen* scr : screens()) {
        if (!scr) continue;
        if (screenIds.contains(scr->id())) {
            ok = false;
            if (outMessages) {
                outMessages->append({ValidationMessage::Error, QString("Duplicate Screen ID: %1").arg(scr->id()), "", scr->id()});
            }
        }
        screenIds.insert(scr->id());

        for (UIComponent* comp : scr->components()) {
            if (!comp) continue;
            QString compId = comp->componentId();
            if (componentIds.contains(compId)) {
                ok = false;
                if (outMessages) {
                    outMessages->append({ValidationMessage::Error, QString("Duplicate Component ID: %1").arg(compId), compId, scr->id()});
                }
            }
            componentIds.insert(compId);
        }
    }

    // Validate DataBindings
    for (const auto& binding : dataBindings()) {
        if (!componentIds.contains(binding.componentId())) {
            if (outMessages) {
                outMessages->append({ValidationMessage::Warning,
                                     QString("DataBinding targets non-existent Component: %1").arg(binding.componentId()),
                                     binding.componentId(), ""});
            }
        }
        if (!binding.sourceId().isEmpty() && !findDataSource(binding.sourceId())) {
            if (outMessages) {
                outMessages->append({ValidationMessage::Warning,
                                     QString("DataBinding references non-existent DataSource: %1").arg(binding.sourceId()),
                                     binding.componentId(), ""});
            }
        }
    }

    return ok;
}

bool GeneratorContext::hasErrors() const {
    QList<ValidationMessage> msgs;
    validate(&msgs);
    for (const auto& m : msgs) {
        if (m.level == ValidationMessage::Error) return true;
    }
    return false;
}

} // namespace CodeGen

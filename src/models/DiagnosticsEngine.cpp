#include "DiagnosticsEngine.h"
#include "Project.h"
#include "Screen.h"
#include "UIComponent.h"
#include "DataSource.h"
#include "DataBinding.h"
#include "HardwareManager.h"
#include "HardwareModel.h"
#include "DeviceDatabase.h"

#include <QUuid>
#include <QSet>

namespace Diagnostics {

QString categoryToString(Category cat) {
    switch (cat) {
        case Category::Project: return "PROJECT";
        case Category::Component: return "COMPONENT";
        case Category::Binding: return "BINDING";
        case Category::Hardware: return "HARDWARE";
        case Category::Generator: return "GENERATOR";
        case Category::CICD: return "CI/CD";
        case Category::AI: return "AI";
        case Category::MCP: return "MCP";
        case Category::Plugin: return "PLUGIN";
    }
    return "PROJECT";
}

Category stringToCategory(const QString& str) {
    QString s = str.toUpper();
    if (s == "COMPONENT") return Category::Component;
    if (s == "BINDING") return Category::Binding;
    if (s == "HARDWARE") return Category::Hardware;
    if (s == "GENERATOR") return Category::Generator;
    if (s == "CI/CD" || s == "CICD") return Category::CICD;
    if (s == "AI") return Category::AI;
    if (s == "MCP") return Category::MCP;
    if (s == "PLUGIN") return Category::Plugin;
    return Category::Project;
}

QString severityToString(Severity sev) {
    switch (sev) {
        case Severity::Info: return "INFO";
        case Severity::Warning: return "WARNING";
        case Severity::Error: return "ERROR";
    }
    return "INFO";
}

Severity stringToSeverity(const QString& str) {
    QString s = str.toUpper();
    if (s == "WARNING") return Severity::Warning;
    if (s == "ERROR") return Severity::Error;
    return Severity::Info;
}

QJsonObject DiagnosticItem::toJson() const {
    QJsonObject obj;
    obj["id"] = id;
    obj["category"] = categoryToString(category);
    obj["severity"] = severityToString(severity);
    obj["title"] = title;
    obj["message"] = message;
    obj["sourceId"] = sourceId;
    obj["fixSuggestion"] = fixSuggestion;
    obj["timestamp"] = timestamp.toString(Qt::ISODate);
    return obj;
}

DiagnosticItem DiagnosticItem::fromJson(const QJsonObject& obj) {
    DiagnosticItem item;
    item.id = obj["id"].toString();
    item.category = stringToCategory(obj["category"].toString());
    item.severity = stringToSeverity(obj["severity"].toString());
    item.title = obj["title"].toString();
    item.message = obj["message"].toString();
    item.sourceId = obj["sourceId"].toString();
    item.fixSuggestion = obj["fixSuggestion"].toString();
    if (obj.contains("timestamp")) {
        item.timestamp = QDateTime::fromString(obj["timestamp"].toString(), Qt::ISODate);
    }
    return item;
}

DiagnosticsEngine& DiagnosticsEngine::instance() {
    static DiagnosticsEngine s_instance;
    return s_instance;
}

DiagnosticsEngine::DiagnosticsEngine(QObject* parent)
    : QObject(parent)
{
}

void DiagnosticsEngine::addDiagnostic(const DiagnosticItem& item) {
    m_items.append(item);
    emit diagnosticAdded(item);
    emit diagnosticsChanged();
}

void DiagnosticsEngine::addDiagnostic(Category cat, Severity sev, const QString& title, const QString& msg,
                                      const QString& sourceId, const QString& fix) {
    DiagnosticItem item;
    item.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    item.category = cat;
    item.severity = sev;
    item.title = title;
    item.message = msg;
    item.sourceId = sourceId;
    item.fixSuggestion = fix;
    item.timestamp = QDateTime::currentDateTime();
    addDiagnostic(item);
}

void DiagnosticsEngine::clearCategory(Category cat) {
    bool changed = false;
    for (int i = m_items.size() - 1; i >= 0; --i) {
        if (m_items[i].category == cat) {
            m_items.removeAt(i);
            changed = true;
        }
    }
    if (changed) {
        emit diagnosticsChanged();
    }
}

void DiagnosticsEngine::clearAll() {
    if (!m_items.isEmpty()) {
        m_items.clear();
        emit diagnosticsChanged();
    }
}

QList<DiagnosticItem> DiagnosticsEngine::allDiagnostics() const {
    return m_items;
}

QList<DiagnosticItem> DiagnosticsEngine::diagnosticsForCategory(Category cat) const {
    QList<DiagnosticItem> result;
    for (const auto& it : m_items) {
        if (it.category == cat) result.append(it);
    }
    return result;
}

QList<DiagnosticItem> DiagnosticsEngine::diagnosticsForSeverity(Severity sev) const {
    QList<DiagnosticItem> result;
    for (const auto& it : m_items) {
        if (it.severity == sev) result.append(it);
    }
    return result;
}

int DiagnosticsEngine::errorCount() const {
    int count = 0;
    for (const auto& it : m_items) {
        if (it.severity == Severity::Error) ++count;
    }
    return count;
}

int DiagnosticsEngine::warningCount() const {
    int count = 0;
    for (const auto& it : m_items) {
        if (it.severity == Severity::Warning) ++count;
    }
    return count;
}

void DiagnosticsEngine::scanProject(const Project* project) {
    clearCategory(Category::Project);
    clearCategory(Category::Component);
    clearCategory(Category::Binding);
    clearCategory(Category::Hardware);

    if (!project) {
        addDiagnostic(Category::Project, Severity::Warning, "No Project", "No active project is loaded.");
        return;
    }

    // 1. Project-level checks
    if (project->screens().isEmpty()) {
        addDiagnostic(Category::Project, Severity::Error, "No Screens",
                      "Project has no screens defined. At least one screen is required.",
                      "", "Add a new screen to the project.");
    }

    if (!project->activeScreen() && !project->screens().isEmpty()) {
        addDiagnostic(Category::Project, Severity::Warning, "No Active Screen",
                      "No active screen is currently selected.",
                      "", "Select an active screen.");
    }

    // 2. Component & Screen checks
    QSet<QString> componentIds;
    int displayW = project->displayConfig().width > 0 ? project->displayConfig().width : 320;
    int displayH = project->displayConfig().height > 0 ? project->displayConfig().height : 240;

    for (Screen* screen : project->screens()) {
        if (!screen) continue;
        int screenW = screen->width() > 0 ? screen->width() : displayW;
        int screenH = screen->height() > 0 ? screen->height() : displayH;

        for (UIComponent* comp : screen->components()) {
            if (!comp) continue;
            QString cid = comp->componentId();

            // Duplicate ID check
            if (componentIds.contains(cid)) {
                addDiagnostic(Category::Component, Severity::Error, "Duplicate Component ID",
                              QString("Component ID '%1' is used more than once across screens.").arg(cid),
                              cid, "Rename one of the components to a unique ID.");
            }
            componentIds.insert(cid);

            // Size check
            if (comp->compWidth() <= 0 || comp->compHeight() <= 0) {
                addDiagnostic(Category::Component, Severity::Warning, "Zero Size Component",
                              QString("Component '%1' has zero or negative dimensions (%2x%3).").arg(cid).arg(comp->compWidth()).arg(comp->compHeight()),
                              cid, "Resize the component to positive dimensions.");
            }

            // Screen boundary check
            if (comp->compX() < 0 || comp->compY() < 0 ||
                comp->compX() + comp->compWidth() > screenW ||
                comp->compY() + comp->compHeight() > screenH) {
                addDiagnostic(Category::Component, Severity::Warning, "Out of Screen Bounds",
                              QString("Component '%1' extends outside screen '%2' bounds (%3x%4).")
                                  .arg(cid, screen->name()).arg(screenW).arg(screenH),
                              cid, "Move or scale the component inside the screen area.");
            }
        }
    }

    // 3. Binding & DataSource checks
    QSet<QString> dataSourceIds;
    for (const auto& ds : project->dataSources()) {
        dataSourceIds.insert(ds.id());
    }

    for (const auto& binding : project->dataBindings()) {
        // Missing DataSource
        if (!dataSourceIds.contains(binding.sourceId())) {
            addDiagnostic(Category::Binding, Severity::Error, "Broken Binding: Missing DataSource",
                          QString("Binding for component '%1' property '%2' references non-existent DataSource '%3'.")
                              .arg(binding.componentId(), binding.propertyName(), binding.sourceId()),
                          binding.componentId(), "Create the missing DataSource or update the binding target.");
        }

        // Missing Component
        if (!componentIds.contains(binding.componentId())) {
            addDiagnostic(Category::Binding, Severity::Error, "Broken Binding: Missing Component",
                          QString("Binding references component '%1' which does not exist in any screen.")
                              .arg(binding.componentId()),
                          binding.componentId(), "Remove obsolete binding or recreate the component.");
        }
    }

    // 4. Hardware checks
    auto hwCfg = project->hardwareConfig();
    if (!hwCfg.deviceId.isEmpty()) {
        auto dev = Hardware::DeviceDatabase::instance().findDevice(hwCfg.deviceId);
        if (dev.partNumber.isEmpty()) {
            addDiagnostic(Category::Hardware, Severity::Warning, "Unknown Hardware Device",
                          QString("Target MCU '%1' is not recognized in device database.").arg(hwCfg.deviceId),
                          hwCfg.deviceId, "Select a valid device from the Hardware Wizard.");
        }
    }
}

QJsonObject DiagnosticsEngine::toJson() const {
    QJsonObject obj;
    QJsonArray arr;
    for (const auto& item : m_items) {
        arr.append(item.toJson());
    }
    obj["diagnostics"] = arr;
    obj["errorCount"] = errorCount();
    obj["warningCount"] = warningCount();
    obj["totalCount"] = m_items.size();
    return obj;
}

QString DiagnosticsEngine::toMarkdown() const {
    if (m_items.isEmpty()) {
        return "### Diagnostics\nAll systems nominal. No errors or warnings detected.\n";
    }

    QString md = QString("### Diagnostics (%1 errors, %2 warnings)\n\n").arg(errorCount()).arg(warningCount());
    for (const auto& item : m_items) {
        QString prefix = item.severity == Severity::Error ? "🚨 **[ERROR]**" :
                         (item.severity == Severity::Warning ? "⚠️ **[WARNING]**" : "ℹ️ **[INFO]**");
        md += QString("- %1 [%2] **%3**: %4\n").arg(prefix, categoryToString(item.category), item.title, item.message);
        if (!item.sourceId.isEmpty()) {
            md += QString("  - Source: `%1`\n").arg(item.sourceId);
        }
        if (!item.fixSuggestion.isEmpty()) {
            md += QString("  - Suggested Fix: %1\n").arg(item.fixSuggestion);
        }
    }
    return md;
}

} // namespace Diagnostics

#include "AIProjectContext.h"
#include "project/Project.h"
#include "models/Screen.h"
#include "models/UIComponent.h"
#include "hardware/HardwareManager.h"
#include <QRegularExpression>

namespace AI {

QString AIProjectContext::sanitizeSecrets(const QString& text) {
    if (text.isEmpty()) return text;
    QString sanitized = text;

    // Redact OpenAI keys
    sanitized.replace(QRegularExpression("sk-[A-Za-z0-9_-]{20,}"), "[REDACTED_API_KEY]");
    // Redact Anthropic keys
    sanitized.replace(QRegularExpression("sk-ant-[A-Za-z0-9_-]{20,}"), "[REDACTED_API_KEY]");
    // Redact Gemini/Google API keys
    sanitized.replace(QRegularExpression("AIza[0-9A-Za-z-_]{35}"), "[REDACTED_API_KEY]");
    // Redact generic bearer tokens
    sanitized.replace(QRegularExpression("Bearer\\s+[A-Za-z0-9_.-]{16,}", QRegularExpression::CaseInsensitiveOption), "Bearer [REDACTED_TOKEN]");
    // Redact password assignments
    sanitized.replace(QRegularExpression("password\\s*=\\s*['\"][^'\"]+['\"]", QRegularExpression::CaseInsensitiveOption), "password=\"[REDACTED]\"");

    return sanitized;
}

QJsonObject AIProjectContext::sanitizeJsonObject(const QJsonObject& obj) {
    QJsonObject out;
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        QString key = it.key();
        QString lowerKey = key.toLower();

        // Check if key is sensitive
        if (lowerKey.contains("key") || lowerKey.contains("secret") || lowerKey.contains("token") || lowerKey.contains("password")) {
            out[key] = "[REDACTED]";
            continue;
        }

        if (it.value().isString()) {
            out[key] = sanitizeSecrets(it.value().toString());
        } else if (it.value().isObject()) {
            out[key] = sanitizeJsonObject(it.value().toObject());
        } else if (it.value().isArray()) {
            out[key] = sanitizeJsonArray(it.value().toArray());
        } else {
            out[key] = it.value();
        }
    }
    return out;
}

QJsonArray AIProjectContext::sanitizeJsonArray(const QJsonArray& arr) {
    QJsonArray out;
    for (const auto& val : arr) {
        if (val.isString()) {
            out.append(sanitizeSecrets(val.toString()));
        } else if (val.isObject()) {
            out.append(sanitizeJsonObject(val.toObject()));
        } else if (val.isArray()) {
            out.append(sanitizeJsonArray(val.toArray()));
        } else {
            out.append(val);
        }
    }
    return out;
}

QJsonObject AIProjectContext::buildScopedScreenContext(const Project* project, const QString& screenName) {
    QJsonObject ctx;
    if (!project) return ctx;

    const Screen* targetScreen = nullptr;
    if (!screenName.isEmpty()) {
        for (const auto* s : project->screens()) {
            if (s && s->name() == screenName) {
                targetScreen = s;
                break;
            }
        }
    }
    if (!targetScreen) {
        targetScreen = project->activeScreen();
    }

    if (!targetScreen && !project->screens().isEmpty()) {
        targetScreen = project->screens().first();
    }

    if (targetScreen) {
        ctx["screen_name"] = targetScreen->name();
        ctx["screen_id"] = targetScreen->id();
        ctx["width"] = targetScreen->width() > 0 ? targetScreen->width() : project->displayConfig().width;
        ctx["height"] = targetScreen->height() > 0 ? targetScreen->height() : project->displayConfig().height;

        QJsonArray compArr;
        for (const auto* comp : targetScreen->components()) {
            if (!comp) continue;
            QJsonObject cObj = comp->toJson();
            cObj["id"] = comp->componentId();
            cObj["type"] = comp->componentType();
            cObj["x"] = comp->compX();
            cObj["y"] = comp->compY();
            cObj["width"] = comp->compWidth();
            cObj["height"] = comp->compHeight();
            cObj["visible"] = comp->isComponentVisible();
            cObj["z_order"] = comp->zValue();
            compArr.append(sanitizeJsonObject(cObj));
        }
        ctx["components"] = compArr;
    }

    // Add scoped DataSources
    QJsonArray dsArr;
    for (const auto& ds : project->dataSources()) {
        QJsonObject dsObj;
        dsObj["id"] = ds.id();
        dsObj["name"] = ds.name();
        dsObj["type"] = dataSourceTypeToString(ds.type());
        dsObj["value"] = ds.value().toString();
        dsArr.append(dsObj);
    }
    ctx["data_sources"] = dsArr;

    // Add scoped DataBindings
    QJsonArray dbArr;
    for (const auto& db : project->dataBindings()) {
        QJsonObject dbObj;
        dbObj["component_id"] = db.componentId();
        dbObj["property"] = db.propertyName();
        dbObj["source_id"] = db.sourceId();
        dbArr.append(dbObj);
    }
    ctx["data_bindings"] = dbArr;

    return sanitizeJsonObject(ctx);
}

QString AIProjectContext::formatScreenContextAsMarkdown(const Project* project, const QString& screenName) {
    QJsonObject ctx = buildScopedScreenContext(project, screenName);
    QString md;

    md += QString("### Screen: %1\n").arg(ctx["screen_name"].toString("Default"));
    md += QString("- **Resolution**: %1x%2\n").arg(ctx["width"].toInt(320)).arg(ctx["height"].toInt(240));

    QJsonArray comps = ctx["components"].toArray();
    md += QString("- **Components** (%1):\n").arg(comps.size());
    for (const auto& cVal : comps) {
        QJsonObject c = cVal.toObject();
        md += QString("  - `%1` (%2): x=%3, y=%4, w=%5, h=%6\n")
            .arg(c["id"].toString(), c["type"].toString())
            .arg(c["x"].toDouble())
            .arg(c["y"].toDouble())
            .arg(c["width"].toDouble())
            .arg(c["height"].toDouble());
    }

    QJsonArray ds = ctx["data_sources"].toArray();
    if (!ds.isEmpty()) {
        md += QString("- **DataSources** (%1):\n").arg(ds.size());
        for (const auto& dVal : ds) {
            QJsonObject d = dVal.toObject();
            md += QString("  - `%1` [%2] = %3\n").arg(d["id"].toString(), d["type"].toString(), d["value"].toString());
        }
    }

    QJsonArray db = ctx["data_bindings"].toArray();
    if (!db.isEmpty()) {
        md += QString("- **Bindings** (%1):\n").arg(db.size());
        for (const auto& bVal : db) {
            QJsonObject b = bVal.toObject();
            md += QString("  - `%1.%2` <- `%3`\n").arg(b["component_id"].toString(), b["property"].toString(), b["source_id"].toString());
        }
    }

    return md;
}

QJsonObject AIProjectContext::buildHardwareContext(const Project* project) {
    QJsonObject hw;
    if (!project) return hw;

    auto cfg = project->hardwareConfig();
    hw["board_id"] = cfg.boardId;
    hw["mcu"] = cfg.deviceId;
    hw["vendor"] = cfg.vendor;

    bool isConnected = Hardware::HardwareManager::instance().isHardwareConnected();
    hw["mode"] = isConnected ? "PHYSICAL_HARDWARE" : "SIMULATION";
    hw["hardware_connected"] = isConnected;

    return hw;
}

QJsonObject AIProjectContext::buildDiagnosticsContext(const Project* project) {
    QJsonObject diag;
    if (!project) return diag;

    // Check for unbound components or out-of-bounds items
    QJsonArray issues;
    if (auto* screen = project->activeScreen()) {
        int sw = screen->width() > 0 ? screen->width() : project->displayConfig().width;
        int sh = screen->height() > 0 ? screen->height() : project->displayConfig().height;

        for (const auto* comp : screen->components()) {
            if (!comp) continue;
            if (comp->compX() < 0 || comp->compY() < 0 || comp->compX() + comp->compWidth() > sw || comp->compY() + comp->compHeight() > sh) {
                QJsonObject iss;
                iss["component_id"] = comp->componentId();
                iss["type"] = "out_of_bounds";
                iss["message"] = QString("Component '%1' extends outside screen bounds (%2x%3).").arg(comp->componentId()).arg(sw).arg(sh);
                issues.append(iss);
            }
        }
    }
    diag["issues"] = issues;
    return diag;
}

} // namespace AI

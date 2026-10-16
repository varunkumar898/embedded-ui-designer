#pragma once

#include <QList>
#include <QString>
#include <QStringList>

class Project;

/**
 * Task 6 - data-driven registry of UI code-generation backends.
 *
 * Every backend entry describes what the backend actually is and what it can
 * generate today (derived from the real generator implementations, never from
 * guesses):
 *   - id / display name / status (Stable, Experimental, Planned)
 *   - the framework string persisted in Project::targetFramework
 *   - supported pixel formats and whether animations can be expressed
 *   - per-component support with an optional note (Supported / Partial /
 *     Unsupported / Unknown); component types the backend has no documented
 *     handling for stay Unknown and surface as Info, never as a guess
 *   - the policy the generator applies to unsupported components (silent
 *     omission, placeholder, hard failure), which decides whether the
 *     "Backend" diagnostics source emits a Warning or an Error.
 *
 * Built-in entries are registered lazily on first access ("startup"); later
 * backends call registerBackend() without any UI change.
 */
enum class BackendSupportLevel {
    Supported,
    Partial,
    Unsupported,
    Unknown
};

struct BackendComponentSupport {
    QString componentType;   // canonical Project component type, e.g. "Gauge"
    QString displayName;     // human readable, e.g. "Gauge", "Progress Bar"
    BackendSupportLevel level = BackendSupportLevel::Unknown;
    QString note;            // per-component clarification (may be empty)
};

/** What the generator does when it meets a component it does not support. */
enum class BackendUnsupportedPolicy {
    OmitWarning,        // widget is silently omitted from generated output
    PlaceholderWarning, // a placeholder visual is generated instead
    HardError           // generation cannot produce a usable UI / aborts
};

enum class BackendFindingSeverity {
    Info = 0,
    Warning = 1,
    Error = 2
};

/** One compatibility finding for a component instance in a project. */
struct BackendFinding {
    QString backendId;
    QString componentType;        // canonical type (or the raw type when unknown)
    QString componentDisplayName; // human readable type name
    QString screenId;
    QString componentId;
    BackendSupportLevel level = BackendSupportLevel::Unknown;
    BackendFindingSeverity severity = BackendFindingSeverity::Info;
    QString message;
};

struct BackendDescriptor {
    QString id;              // "lvgl", "ugfx", "qt-for-mcus", "embedweb", "native"
    QString displayName;     // e.g. "LVGL", "EmbedWeb", "Native Runtime (planned)"
    QString status;          // "Stable" / "Experimental" / "Planned"
    QString description;     // short, factual description
    QString frameworkId;     // value written to Project::targetFramework ("" for planned)
    QStringList pixelFormats; // e.g. {"RGB565", "RGB888"}
    bool supportsAnimations = false;
    QString animationsNote;  // factual qualification of the animation support
    bool selectable = true;  // planned backends are shown but cannot be chosen
    QString disabledTooltip; // e.g. "Not implemented yet."
    BackendUnsupportedPolicy unsupportedPolicy = BackendUnsupportedPolicy::OmitWarning;
    QList<BackendComponentSupport> components;
};

class BackendRegistry {
public:
    /** All registered backends in registration order (built-ins first). */
    static QList<BackendDescriptor> backends();

    /** Registers a new backend. Fails on empty or duplicate ids. */
    static bool registerBackend(const BackendDescriptor& backend);

    /** Removes a backend (used by tests; built-ins may be re-added). */
    static bool unregisterBackend(const QString& id);

    static bool contains(const QString& id);

    /** Descriptor by id; default-constructed descriptor when unknown. */
    static BackendDescriptor backend(const QString& id);

    /**
     * Effective backend for a persisted targetFramework string.
     *
     * Mirrors the FirmwareExportManager generator dispatch exactly, so the
     * selected backend always matches the generator that would actually run
     * (including legacy spellings such as "qul" or "qtmcu").
     */
    static BackendDescriptor backendForFramework(const QString& targetFramework);

    /** Support of one canonical component type in one backend. */
    static BackendComponentSupport supportFor(const QString& backendId, const QString& componentType);

    /** Human readable name for a component type (falls back to the raw type). */
    static QString componentDisplayName(const QString& componentType);

    /**
     * Every component of the project the given backend cannot fully handle.
     * Uses the project's effective backend when backendId is empty. Never
     * guesses: undocumented component types produce Info findings.
     */
    static QList<BackendFinding> validate(const Project* project, const QString& backendId = QString());
};

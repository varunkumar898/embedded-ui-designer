#pragma once

#include <QObject>
#include <QProcess>
#include <QElapsedTimer>
#include "BuildConfiguration.h"
#include "DiagnosticParser.h"

class BuildRunner : public QObject {
    Q_OBJECT

public:
    explicit BuildRunner(QObject* parent = nullptr);
    ~BuildRunner() override;

    bool isRunning() const;
    bool startConfigure(const BuildConfiguration& cfg);
    bool startBuild(const BuildConfiguration& cfg);
    bool startClean(const QString& workingDir);
    void cancelBuild();

signals:
    void buildStarted(const QString& description);
    void outputReceived(const QString& text, bool isStderr);
    void diagnosticFound(const CompilerDiagnostic& diag);
    void buildFinished(bool success, int exitCode, qint64 durationMs);

private slots:
    void onReadyReadStandardOutput();
    void onReadyReadStandardError();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);

private:
    void handleOutput(const QString& text, bool isStderr);

    QProcess* m_process = nullptr;
    DiagnosticParser m_parser;
    QElapsedTimer m_timer;
    QString m_currentAction;
};

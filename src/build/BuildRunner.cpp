#include "BuildRunner.h"
#include <QDir>
#include <QFileInfo>

BuildRunner::BuildRunner(QObject* parent)
    : QObject(parent)
{
}

BuildRunner::~BuildRunner() {
    cancelBuild();
}

bool BuildRunner::isRunning() const {
    return m_process && m_process->state() != QProcess::NotRunning;
}

bool BuildRunner::startConfigure(const BuildConfiguration& cfg) {
    if (isRunning()) return false;

    QString err;
    if (!cfg.validate(&err)) {
        emit outputReceived(QString("Configuration Error: %1\n").arg(err), true);
        return false;
    }

    if (m_process) {
        delete m_process;
        m_process = nullptr;
    }
    m_process = new QProcess(this);
    m_process->setWorkingDirectory(cfg.workingDirectory);

    connect(m_process, &QProcess::readyReadStandardOutput, this, &BuildRunner::onReadyReadStandardOutput);
    connect(m_process, &QProcess::readyReadStandardError, this, &BuildRunner::onReadyReadStandardError);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &BuildRunner::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred, this, &BuildRunner::onProcessError);

    QStringList args;
    args << "-B" << "build" << "-S" << ".";

    QString buildTypeStr = "Release";
    if (cfg.buildType == BuildType::Debug) buildTypeStr = "Debug";
    else if (cfg.buildType == BuildType::MinSizeRel) buildTypeStr = "MinSizeRel";
    else if (cfg.buildType == BuildType::RelWithDebInfo) buildTypeStr = "RelWithDebInfo";
    args << QString("-DCMAKE_BUILD_TYPE=%1").arg(buildTypeStr);

    m_currentAction = "Configuring CMake";
    emit buildStarted(m_currentAction);
    m_timer.start();

    m_process->start("cmake", args);
    return true;
}

bool BuildRunner::startBuild(const BuildConfiguration& cfg) {
    if (isRunning()) return false;

    QString err;
    if (!cfg.validate(&err)) {
        emit outputReceived(QString("Build Error: %1\n").arg(err), true);
        return false;
    }

    if (m_process) {
        delete m_process;
        m_process = nullptr;
    }
    m_process = new QProcess(this);
    m_process->setWorkingDirectory(cfg.workingDirectory);

    connect(m_process, &QProcess::readyReadStandardOutput, this, &BuildRunner::onReadyReadStandardOutput);
    connect(m_process, &QProcess::readyReadStandardError, this, &BuildRunner::onReadyReadStandardError);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &BuildRunner::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred, this, &BuildRunner::onProcessError);

    QStringList args;
    args << "--build" << "build";
    if (cfg.parallelJobs > 0) {
        args << "-j" << QString::number(cfg.parallelJobs);
    }

    m_currentAction = "Building Target";
    emit buildStarted(m_currentAction);
    m_timer.start();

    m_process->start("cmake", args);
    return true;
}

bool BuildRunner::startClean(const QString& workingDir) {
    if (isRunning()) return false;

    if (workingDir.trimmed().isEmpty() || !QDir(workingDir).exists()) {
        emit outputReceived("Clean Error: Invalid working directory\n", true);
        return false;
    }

    if (m_process) {
        delete m_process;
        m_process = nullptr;
    }
    m_process = new QProcess(this);
    m_process->setWorkingDirectory(workingDir);

    connect(m_process, &QProcess::readyReadStandardOutput, this, &BuildRunner::onReadyReadStandardOutput);
    connect(m_process, &QProcess::readyReadStandardError, this, &BuildRunner::onReadyReadStandardError);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &BuildRunner::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred, this, &BuildRunner::onProcessError);

    QStringList args;
    args << "--build" << "build" << "--target" << "clean";

    m_currentAction = "Cleaning Build Directory";
    emit buildStarted(m_currentAction);
    m_timer.start();

    m_process->start("cmake", args);
    return true;
}

void BuildRunner::cancelBuild() {
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        m_process->waitForFinished(1000);
    }
}

void BuildRunner::onReadyReadStandardOutput() {
    if (!m_process) return;
    QByteArray data = m_process->readAllStandardOutput();
    handleOutput(QString::fromUtf8(data), false);
}

void BuildRunner::onReadyReadStandardError() {
    if (!m_process) return;
    QByteArray data = m_process->readAllStandardError();
    handleOutput(QString::fromUtf8(data), true);
}

void BuildRunner::handleOutput(const QString& text, bool isStderr) {
    emit outputReceived(text, isStderr);

    QStringList lines = text.split('\n');
    for (const QString& line : lines) {
        if (line.trimmed().isEmpty()) continue;
        CompilerDiagnostic diag = m_parser.parseLine(line);
        if (diag.severity == DiagnosticSeverity::Error || diag.severity == DiagnosticSeverity::Warning) {
            emit diagnosticFound(diag);
        }
    }
}

void BuildRunner::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    qint64 elapsed = m_timer.elapsed();
    bool success = (exitStatus == QProcess::NormalExit && exitCode == 0);
    emit buildFinished(success, exitCode, elapsed);

    if (m_process) {
        m_process->deleteLater();
        m_process = nullptr;
    }
}

void BuildRunner::onProcessError(QProcess::ProcessError error) {
    Q_UNUSED(error);
    qint64 elapsed = m_timer.elapsed();
    emit outputReceived("Process execution failed to start or crashed.\n", true);
    emit buildFinished(false, -1, elapsed);

    if (m_process) {
        m_process->deleteLater();
        m_process = nullptr;
    }
}

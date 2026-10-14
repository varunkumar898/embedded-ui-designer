#include "OpenOCDWorker.h"
#include "OpenOCDManager.h"
#include <QTimer>
#include <QDebug>
#include <QThread>
#include <chrono>

OpenOCDWorker::OpenOCDWorker(OpenOCDManager* openocd)
    : m_openocd(openocd) {
}

OpenOCDWorker::~OpenOCDWorker() {
}

void OpenOCDWorker::connectToOpenOCD(const QString& host, int port, int timeoutSeconds) {
    m_host = host;
    m_port = port;
    m_timeoutSeconds = timeoutSeconds;
    m_cancelRequested = false;
    
    // Schedule connection to run in this thread's event loop
    QMetaObject::invokeMethod(this, &OpenOCDWorker::doConnect, Qt::QueuedConnection);
}

void OpenOCDWorker::cancelConnection() {
    m_cancelRequested = true;
}

void OpenOCDWorker::doConnect() {
    if (m_cancelRequested) {
        emit connectionFailed("Connection cancelled by user");
        return;
    }
    
    if (!m_openocd) {
        emit connectionFailed("OpenOCDManager instance is null");
        return;
    }
    
    // STEP 1: Attempt Telnet connection
    emit progressUpdated("Connecting to OpenOCD...", 10);
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    while (!m_openocd->isConnected() && !m_cancelRequested) {
        // Try to connect
        m_openocd->connectToTelnet(m_host, m_port);
        
        // Check timeout
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count();
        
        if (elapsed >= m_timeoutSeconds) {
            emit connectionFailed(
                QString("OpenOCD connection timeout after %1 seconds. "
                        "Make sure:\n"
                        "1. OpenOCD is running\n"
                        "2. Telnet port %2 is open\n"
                        "3. Board is connected\n"
                        "4. USB drivers are installed")
                .arg(m_timeoutSeconds)
                .arg(m_port)
            );
            return;
        }
        
        if (m_openocd->isConnected()) {
            break;
        }
        
        // Wait 500ms before retry
        QThread::msleep(500);
    }
    
    if (m_cancelRequested) {
        emit connectionFailed("Connection cancelled by user");
        return;
    }
    
    if (!m_openocd->isConnected()) {
        emit connectionFailed("Failed to connect to OpenOCD");
        return;
    }
    
    // STEP 2: Auto-detect board
    emit progressUpdated("Detecting board...", 40);
    
    if (!m_openocd->autoDetectAndInitializeBoard()) {
        // Fallback to STM32 if auto-detect fails
        qWarning() << "Board auto-detection failed. Falling back to STM32F030R8.";
        m_openocd->setBoard("STM32F030R8");
    }
    
    emit progressUpdated("Board detected", 80);
    
    // STEP 3: Verify connection
    emit progressUpdated("Verifying connection...", 90);
    
    QString testCommand = m_openocd->sendCommand("targets");
    if (testCommand.isEmpty()) {
        emit connectionFailed("Board detected but verification failed");
        return;
    }
    
    // Success!
    QString boardName = m_openocd->getDetectedBoardName();
    if (boardName.isEmpty()) {
        boardName = "Unknown";
    }
    
    emit progressUpdated("Connected!", 100);
    emit connectionSucceeded(boardName);
}

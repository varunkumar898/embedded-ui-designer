#ifndef OPENOCD_WORKER_H
#define OPENOCD_WORKER_H

#include <QObject>
#include <QThread>
#include <QString>
#include <memory>

class OpenOCDManager;

/**
 * @class OpenOCDWorker
 * @brief Runs OpenOCD connection in background thread to prevent UI blocking.
 * 
 * Usage:
 * - QThread creates this worker
 * - Worker emits signals (connectionSucceeded, connectionFailed, progress)
 * - Main thread updates UI based on signals
 * - User can cancel connection anytime
 */
class OpenOCDWorker : public QObject {
    Q_OBJECT

public:
    explicit OpenOCDWorker(OpenOCDManager* openocd);
    ~OpenOCDWorker() override;

    /**
     * @brief Connect to OpenOCD (runs in background thread).
     * @param host Telnet host (default "127.0.0.1")
     * @param port Telnet port (default 4444)
     * @param timeoutSeconds Max seconds to wait (default 5)
     */
    void connectToOpenOCD(const QString& host = "127.0.0.1", 
                         int port = 4444, 
                         int timeoutSeconds = 5);

    /**
     * @brief Cancel ongoing connection attempt.
     */
    void cancelConnection();

signals:
    /**
     * @brief Emitted when connection succeeds.
     * @param boardName Auto-detected board name (e.g., "STM32F030R8")
     */
    void connectionSucceeded(const QString& boardName);

    /**
     * @brief Emitted when connection fails.
     * @param errorMessage Reason for failure
     */
    void connectionFailed(const QString& errorMessage);

    /**
     * @brief Progress update (0-100).
     * @param message Status message ("Connecting...", "Auto-detecting board...", etc.)
     * @param progress Percentage (0-100)
     */
    void progressUpdated(const QString& message, int progress);

private slots:
    /**
     * @brief Internal slot called when thread starts.
     * Performs actual connection work.
     */
    void doConnect();

private:
    OpenOCDManager* m_openocd = nullptr;
    QString m_host = "127.0.0.1";
    int m_port = 4444;
    int m_timeoutSeconds = 5;
    bool m_cancelRequested = false;
};

#endif // OPENOCD_WORKER_H

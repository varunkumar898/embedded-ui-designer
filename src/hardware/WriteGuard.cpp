#include "WriteGuard.h"

namespace Hardware {

bool WriteGuard::allow(quint32 address,
                      const QSet<quint32>& allowedAddresses,
                      bool profileLiveWriteAllowed,
                      bool sessionOverride,
                      QString* reason) {
    if (sessionOverride) {
        if (reason) *reason = QStringLiteral("allowed by per-session override");
        return true;
    }
    if (!profileLiveWriteAllowed) {
        if (reason) *reason = QStringLiteral("live writes are not allowed for this board (only STM32F030R8 is verified)");
        return false;
    }
    if (!allowedAddresses.contains(address)) {
        if (reason) *reason = QStringLiteral("address 0x%1 is not in the board profile / signal table").arg(address, 8, 16, QChar('0'));
        return false;
    }
    if (reason) reason->clear();
    return true;
}

} // namespace Hardware
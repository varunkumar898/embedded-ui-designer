#pragma once

#include <QString>
#include <QSet>
#include <cstdint>

namespace Hardware {

// Central choke point for every live register write. Default-deny: a write is
// allowed only when (a) the address belongs to the active board profile /
// signal table AND (b) the profile is marked live-write allowed (currently
// STM32F030R8 only). An explicit per-session override bypasses (a)/(b).
class WriteGuard {
public:
    static bool allow(quint32 address,
                      const QSet<quint32>& allowedAddresses,
                      bool profileLiveWriteAllowed,
                      bool sessionOverride,
                      QString* reason = nullptr);
};

} // namespace Hardware
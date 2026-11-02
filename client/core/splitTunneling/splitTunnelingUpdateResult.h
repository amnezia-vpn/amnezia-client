#ifndef SPLITTUNNELINGUPDATERESULT_H
#define SPLITTUNNELINGUPDATERESULT_H

#include <QMetaType>

enum class SplitTunnelingUpdateResult {
    Applied = 1,
    NotApplied = 2,
    StateUnknown = 0
};

Q_DECLARE_METATYPE(SplitTunnelingUpdateResult)

inline SplitTunnelingUpdateResult splitTunnelingUpdateResultFromInt(int value)
{
    switch (value) {
    case static_cast<int>(SplitTunnelingUpdateResult::Applied): return SplitTunnelingUpdateResult::Applied;
    case static_cast<int>(SplitTunnelingUpdateResult::NotApplied): return SplitTunnelingUpdateResult::NotApplied;
    default: return SplitTunnelingUpdateResult::StateUnknown;
    }
}

#endif

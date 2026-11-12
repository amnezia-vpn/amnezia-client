#ifndef SPLITTUNNELINGFIREWALL_H
#define SPLITTUNNELINGFIREWALL_H

#include <QByteArray>
#include <QStringList>
#include <functional>

#include "core/splitTunneling/splitTunnelingUpdateResult.h"

class InterfaceConfig;

class SplitTunnelingFirewall
{
public:
    enum class Platform {
        MacOS,
        Linux
    };

    struct Command
    {
        QString program;
        QStringList arguments;
        QByteArray input;
    };

    using Executor = std::function<SplitTunnelingUpdateResult(const Command &)>;

    static SplitTunnelingUpdateResult apply(Platform platform, const InterfaceConfig &config,
                                            const Executor &execute = runCommand);

private:
    static SplitTunnelingUpdateResult runCommand(const Command &command);
};

#endif

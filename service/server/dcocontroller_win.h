#ifndef DCOCONTROLLERWIN_H
#define DCOCONTROLLERWIN_H

#include <QString>

/**
 * @brief The DcoController class - installs the ovpn-dco-win driver and
 * provisions a Data Channel Offload adapter for OpenVPN 2.6+.
 *
 * OpenVPN 2.7 uses ovpn-dco by default (wintun support was removed) and, when
 * launched without OpenVPN's own Interactive Service, expects the adapter to
 * already exist. The signed driver flavours are shipped in <appdir>/dco/
 * {win10,win11}; adapters are created with the bundled tapctl.exe.
 * tap-windows6 (see TapController) remains the fallback data path for configs
 * that are not DCO-compatible.
 */
class DcoController
{
public:
    static DcoController &Instance();

    static constexpr const char *kAdapterName = "AmneziaDCO";

    // ensures the driver is installed and a DCO adapter exists
    bool checkAndSetup();

    bool isDriverInstalled();
    bool installDriver();
    bool adapterExists();
    bool createAdapter();

private:
    DcoController() = default;
    DcoController(DcoController const &) = delete;
    DcoController &operator=(DcoController const &) = delete;

    QString getDcoDriverDir();
    QString getTapctlPath();
};

#endif // DCOCONTROLLERWIN_H

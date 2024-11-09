/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "windowsdaemon.h"

#include <Windows.h>

#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QLocalSocket>
#include <QNetworkInterface>
#include <QTextStream>
#include <QtGlobal>

#include "core/networkUtilities.h"
#include "dnsutilswindows.h"
#include "leakdetector.h"
#include "logger.h"
#include "platforms/windows/windowscommons.h"
#include "platforms/windows/windowsservicemanager.h"
#include "splitTunnelService.h"
#include "windowsfirewall.h"

namespace {
    Logger logger("WindowsDaemon");
}

WindowsDaemon::WindowsDaemon() : Daemon(nullptr) {
    MZ_COUNT_CTOR(WindowsDaemon);

    m_wgutils = new WireguardUtilsWindows(this);
    m_dnsutils = new DnsUtilsWindows(this);

    connect(m_wgutils, &WireguardUtilsWindows::backendFailure, this, &WindowsDaemon::monitorBackendFailure);
    connect(this, &WindowsDaemon::activationFailure, []() { WindowsFirewall::instance()->disableKillSwitch(); });
}

WindowsDaemon::~WindowsDaemon() {
    MZ_COUNT_DTOR(WindowsDaemon);
    logger.debug() << "Daemon released";
}

void WindowsDaemon::prepareActivation(const InterfaceConfig &config, int inetAdapterIndex) {
    // Before creating the interface we need to check which adapter
    // routes to the server endpoint
    if (inetAdapterIndex == 0) {
        auto serveraddr = QHostAddress(config.m_serverIpv4AddrIn);
        m_inetAdapterIndex = NetworkUtilities::AdapterIndexTo(serveraddr);
    } else {
        m_inetAdapterIndex = inetAdapterIndex;
    }
}

bool WindowsDaemon::activateSplitTunnel(const InterfaceConfig &config, int vpnAdapterIndex) {
    bool shouldError = config.m_vpnDisabledApps.length() > 0;

    if (!WinSplitTunnelDriver::CheckLoaded()) {
        auto error = WinSplitTunnelService::InstallService();
        if (error != WinSplitTunnelService::InstallError::None &&
            error != WinSplitTunnelService::InstallError::AlreadyInstalled) {
            if (shouldError)
                return false;
        }
    }

    auto initResult = m_splitTunnelDriver.init();
    if (initResult != WinSplitTunnelDriver::InitError::None &&
        initResult != WinSplitTunnelDriver::InitError::AlreadyInitialized) {
        if (shouldError)
            return false;
    }

    if (!WindowsFirewall::instance()->init() && shouldError) {
        logger.error() << "Init WFP-Sublayer failed, driver won't be functional";
        return false;
    }

    if (!m_splitTunnelDriver.reconfigureDriver(m_inetAdapterIndex, vpnAdapterIndex) && shouldError) {
        return false;
    }

    if (config.m_vpnDisabledApps.length() > 0) {
        if (!m_splitTunnelDriver.setConfig(config.m_vpnDisabledApps)) {
            return false;
        }
    } else {
        m_splitTunnelDriver.clearConfig();
    }

    return true;
}

bool WindowsDaemon::run(Op op, const InterfaceConfig &config) {
    if (op == Down) {
        m_splitTunnelDriver.clearConfig();
        return false;
    }

    if (op == Up) {
        logger.debug() << "Tunnel UP";
    }

    if (!activateSplitTunnel(config)) {
        return false;
    }

    return true;
}

void WindowsDaemon::monitorBackendFailure() {
    logger.warning() << "Tunnel service is down";

    emit backendFailure();
    deactivate();
}

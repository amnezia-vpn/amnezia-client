/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "macosdaemonserver.h"
#include "commandlineparser.h"
#include "daemon/daemonlocalserver.h"
#include "leakdetector.h"
#include "logger.h"
#include "macosdaemon.h"
#include "mozillavpn.h"
#include "signalhandler.h"

#include <QCoreApplication>

namespace {
Logger logger(LOG_MACOS, "MacOSDaemonServer");
}

MacOSDaemonServer::MacOSDaemonServer(QObject* parent)
    : Command(parent, "macosdaemon", "Activate the macos daemon") {
  MVPN_COUNT_CTOR(MacOSDaemonServer);
}

MacOSDaemonServer::~MacOSDaemonServer() { MVPN_COUNT_DTOR(MacOSDaemonServer); }

int MacOSDaemonServer::run(QStringList& tokens) {
  Q_ASSERT(!tokens.isEmpty());
  QString appName = tokens[0];

  QCoreApplication app(CommandLineParser::argc(), CommandLineParser::argv());

  QCoreApplication::setApplicationName("Mozilla VPN Daemon");
  QCoreApplication::setApplicationVersion(APP_VERSION);

  if (tokens.length() > 1) {
    QList<CommandLineParser::Option*> options;
    return CommandLineParser::unknownOption(this, appName, tokens[1], options,
                                            false);
  }

  MacOSDaemon daemon;

  DaemonLocalServer server(qApp);
  if (!server.initialize()) {
    logger.error() << "Failed to initialize the server";
    return 1;
  }

  // Signal handling for a proper shutdown.
  SignalHandler sh;
  QObject::connect(&sh, &SignalHandler::quitRequested,
                   []() { MacOSDaemon::instance()->deactivate(); });
  QObject::connect(&sh, &SignalHandler::quitRequested, &app,
                   &QCoreApplication::quit, Qt::QueuedConnection);

  return app.exec();
}

static Command::RegistrationProxy<MacOSDaemonServer> s_commandMacOSDaemon;

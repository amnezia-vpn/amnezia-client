/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "macosdaemonserver.h"

#include <QCoreApplication>

#include "commandlineparser.h"
#include "constants.h"
#include "daemon/daemonlocalserver.h"
#include "leakdetector.h"
#include "logger.h"
#include "macosdaemon.h"
#include "signalhandler.h"

namespace {
Logger logger("MacOSDaemonServer");
}

MacOSDaemonServer::MacOSDaemonServer(QObject* parent)
    : Command(parent, "macosdaemon", "Activate the macos daemon") {
  MZ_COUNT_CTOR(MacOSDaemonServer);
}

MacOSDaemonServer::~MacOSDaemonServer() { MZ_COUNT_DTOR(MacOSDaemonServer); }

int MacOSDaemonServer::run(QStringList& tokens) {
  Q_ASSERT(!tokens.isEmpty());
  QString appName = tokens[0];

  char* argv[] = { "Amnezia VPN Daemon" };
  int argc = sizeof(argv) / sizeof(argv[0]);

  QCoreApplication app(argc, argv);

  //QCoreApplication app(CommandLineParser::argc(), CommandLineParser::argv());

  QCoreApplication::setApplicationName("Amnezia VPN Daemon");
  QCoreApplication::setApplicationVersion(Constants::versionString());

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

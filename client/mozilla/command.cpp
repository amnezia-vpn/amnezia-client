/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "command.h"

#include "commandlineparser.h"
#include "leakdetector.h"
#include "logger.h"

#ifdef MZ_WINDOWS
#  include <Windows.h>

#  include <QQuickWindow>
#  include <QSGRendererInterface>

#  include "platforms/windows/windowscommons.h"
#endif

#ifdef MZ_MACOS
#  include "platforms/macos/macosutils.h"
#endif

#include <QApplication>
#include <QIcon>
#include <QTextStream>

namespace {
Logger logger("Command");
}  // namespace

QVector<std::function<Command*(QObject*)>> Command::s_commandCreators;

Command::Command(QObject* parent, const QString& name,
                 const QString& description)
    : QObject(parent), m_name(name), m_description(description) {
  MZ_COUNT_CTOR(Command);
}

Command::~Command() { MZ_COUNT_DTOR(Command); }

// static
QVector<Command*> Command::commands(QObject* parent) {
  QVector<Command*> list;
  for (auto i = s_commandCreators.begin(); i != s_commandCreators.end(); ++i) {
    list.append((*i)(parent));
  }
  return list;
}

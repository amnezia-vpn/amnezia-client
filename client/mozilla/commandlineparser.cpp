/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "commandlineparser.h"

#include <QTextStream>

#include "command.h"
#include "constants.h"
#include "leakdetector.h"

constexpr const char* CLP_DEFAULT_COMMAND = "ui";

namespace {
int s_argc = 0;
char** s_argv = nullptr;
}  // namespace

// static
int& CommandLineParser::argc() {
  Q_ASSERT(s_argc > 0);
  return s_argc;
}

// static
char** CommandLineParser::argv() {
  Q_ASSERT(s_argv);
  return s_argv;
}

CommandLineParser::CommandLineParser() { MZ_COUNT_CTOR(CommandLineParser); }

CommandLineParser::~CommandLineParser() { MZ_COUNT_DTOR(CommandLineParser); }

int CommandLineParser::parse(int argc, char* argv[]) {
  Q_ASSERT(argc >= 1);

  s_argc = argc;
  s_argv = argv;

  QStringList tokens;
  for (int i = 0; i < argc; ++i) {
#ifdef MZ_DEBUG
    if (QString(argv[i]).startsWith("-qmljsdebugger")) {
      continue;
    }
#endif
    tokens.append(QString(argv[i]));
  }

  QList<Option*> options;

  Option hOption = helpOption();
  options.append(&hOption);

  Option versionOption("v", "version", "Displays version information.");
  options.append(&versionOption);

  if (parse(tokens, options, true)) {
    return 1;
  }

  if (hOption.m_set) {
    showHelp(this, argv[0], options, true, false);
    return 0;
  }

  if (versionOption.m_set) {
    QTextStream stream(stdout);
    stream << argv[0] << " " << Constants::versionString() << Qt::endl;
    return 0;
  }

  if (tokens.isEmpty()) {
    tokens.append(CLP_DEFAULT_COMMAND);
  }

  QVector<Command*> commands = Command::commands(this);
  for (Command* command : commands) {
    if (command->name() == tokens[0]) {
      tokens[0] = QString("%1 %2").arg(argv[0], tokens[0]);
      return command->run(tokens);
    }
  }

  QTextStream stream(stderr);
  stream << argv[0] << ": '" << tokens[0] << "' is not a valid command. See '"
         << argv[0] << " --help'." << Qt::endl;
  return 1;
}

int CommandLineParser::parse(QStringList& tokens, QList<Option*>& options,
                             bool hasCommands) {
  Q_ASSERT(!tokens.isEmpty());

  QString app = tokens.takeFirst();

  if (!parseOptions(tokens, options)) {
    Q_ASSERT(!tokens.isEmpty());
    return unknownOption(this, tokens[0], app, options, hasCommands);
  }

  return 0;
}

// static
bool CommandLineParser::parseOptions(QStringList& tokens,
                                     QList<Option*>& options) {
  while (!tokens.isEmpty()) {
    if (!tokens[0].startsWith("-")) {
      return true;
    }

    QString opt = tokens[0];

    bool shortOption = true;
    opt.remove(0, 1);

    if (opt.startsWith("-")) {
      opt.remove(0, 1);
      shortOption = false;
    }

    if (!parseOption(opt, shortOption, options)) {
      return false;
    }

    tokens.removeFirst();
  }

  return true;
}

// static
bool CommandLineParser::parseOption(const QString& option, bool shortOption,
                                    QList<Option*>& options) {
  for (Option* o : options) {
    if (shortOption && o->m_short == option) {
      o->m_set = true;
      return true;
    }

    if (!shortOption && o->m_long == option) {
      o->m_set = true;
      return true;
    }
  }

  return false;
}

// static
int CommandLineParser::unknownOption(QObject* parent, const QString& option,
                                     const QString& app,
                                     QList<Option*>& options,
                                     bool hasCommands) {
  QTextStream stream(stderr);
  stream << "unknown option: " << option << Qt::endl;
  showHelp(parent, app, options, hasCommands, true);
  return 1;
}

// static
void CommandLineParser::showHelp(QObject* parent, const QString& app,
                                 const QList<Option*>& options,
                                 bool hasCommands, bool compact) {
  QTextStream stream(stdout);
  stream << "usage: " << app;

  for (const Option* o : options) {
    stream << " [-" << o->m_short << " | --" << o->m_long << "]";
  }

  if (hasCommands) {
    stream << " <commands> [<args>]";
  }

  stream << Qt::endl;

  if (compact) {
    return;
  }

  if (!options.isEmpty()) {
    stream << Qt::endl;
    stream << "List of options:" << Qt::endl;
    for (const Option* o : options) {
      QString desc = QString("-%1 | --%2").arg(o->m_short, o->m_long);
      stream << "  " << desc << " ";

      for (qsizetype i = desc.length(); i < 20; ++i) {
        stream << " ";
      }

      stream << o->m_description << Qt::endl;
    }
  }

  if (!hasCommands) {
    return;
  }

  stream << Qt::endl;
  stream << "List of commands:" << Qt::endl;

  QVector<Command*> commands = Command::commands(parent);
  for (Command* command : commands) {
    stream << "  " << command->name() << " ";

    for (qsizetype i = command->name().length(); i < 20; ++i) {
      stream << " ";
    }

    stream << command->description() << Qt::endl;
  }
}

// static
CommandLineParser::Option CommandLineParser::helpOption() {
  return Option("h", "help", "Displays help on commandline options.");
}

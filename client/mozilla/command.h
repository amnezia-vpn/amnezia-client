/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef COMMAND_H
#define COMMAND_H

#include <QObject>
#include <QScopedPointer>
#include <QStringList>
#include <QThread>
#include <QVector>
#include <functional>

class Command : public QObject {
  Q_OBJECT

 public:
  static QVector<Command*> commands(QObject* parent);

  Command(QObject* parent, const QString& name, const QString& description);
  virtual ~Command();

  virtual int run(QStringList& tokens) = 0;

  const QString& name() const { return m_name; }

  const QString& description() const { return m_description; }

 private:
  QString m_name;
  QString m_description;

 protected:
  static QVector<std::function<Command*(QObject*)>> s_commandCreators;

 public:
  template <class T>
  struct RegistrationProxy {
    RegistrationProxy() {
        s_commandCreators.append(RegistrationProxy::create);
    }

    static Command* create(QObject* parent) {
        return new T(parent);
    }
  };
};

#endif  // COMMAND_H

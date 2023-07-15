/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DAEMONLOCALSERVERCONNECTION_H
#define DAEMONLOCALSERVERCONNECTION_H

#include <QObject>

class QLocalSocket;

class DaemonLocalServerConnection final : public QObject {
  Q_DISABLE_COPY_MOVE(DaemonLocalServerConnection)

 public:
  DaemonLocalServerConnection(QObject* parent, QLocalSocket* socket);
  ~DaemonLocalServerConnection();

 private:
  void readData();

  void parseCommand(const QByteArray& json);

  void connected(const QString& pubkey);
  void disconnected();
  void backendFailure();

  void write(const QJsonObject& obj);

 private:
  QLocalSocket* m_socket = nullptr;

  QByteArray m_buffer;
};

#endif  // DAEMONLOCALSERVERCONNECTION_H

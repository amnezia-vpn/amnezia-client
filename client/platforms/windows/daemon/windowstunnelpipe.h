/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef WINDOWSTUNNELPIPE_H
#define WINDOWSTUNNELPIPE_H

#include <QDeadlineTimer>
#include <QDebug>
#include <QLocalSocket>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace WindowsTunnelPipe {

// Use one deadline for the entire exchange, including fragmented replies.
inline QString request(const QString& pipeName, const QString& command,
                       int timeoutMsec = 5000) {
  QDeadlineTimer deadline(timeoutMsec);
  QLocalSocket socket;
#ifdef Q_OS_WIN
  // QLocalSocket::connectToServer() retries busy Windows pipes with its own
  // five-second waits. Open the overlapped handle using our deadline instead.
  const QString fullName = pipeName.startsWith(QStringLiteral("\\\\.\\pipe\\"))
                              ? pipeName : QStringLiteral("\\\\.\\pipe\\") + pipeName;
  HANDLE pipe = INVALID_HANDLE_VALUE;
  while (!deadline.hasExpired()) {
    pipe = CreateFileW(reinterpret_cast<LPCWSTR>(fullName.utf16()),
                       GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING,
                       FILE_FLAG_OVERLAPPED, nullptr);
    if (pipe != INVALID_HANDLE_VALUE) {
      break;
    }
    if (GetLastError() != ERROR_PIPE_BUSY) {
      break;
    }
    const qint64 remaining = deadline.remainingTime();
    // Zero means "use the pipe's default timeout" to WaitNamedPipeW.
    if (remaining <= 0 ||
        !WaitNamedPipeW(reinterpret_cast<LPCWSTR>(fullName.utf16()), static_cast<DWORD>(remaining))) {
      break;
    }
  }
  if (pipe == INVALID_HANDLE_VALUE) {
    qWarning() << "Failed to open the tunnel pipe before the deadline";
    return {};
  }
  if (!socket.setSocketDescriptor(reinterpret_cast<qintptr>(pipe))) {
    CloseHandle(pipe);
    return {};
  }
#else
  socket.connectToServer(pipeName);
  if (!socket.waitForConnected(deadline.remainingTime())) {
    qWarning() << "Failed to connect to the tunnel pipe:" << socket.errorString();
    return {};
  }
#endif

  QByteArray message = command.toUtf8();
  while (!message.endsWith("\n\n")) {
    message.append('\n');
  }
  if (socket.write(message) != message.size()) {
    qWarning() << "Failed to write to the tunnel pipe:" << socket.errorString();
    return {};
  }
  while (socket.bytesToWrite() > 0) {
    if (deadline.hasExpired() ||
        !socket.waitForBytesWritten(deadline.remainingTime())) {
      qWarning() << "Tunnel pipe write failed or timed out:" << socket.errorString();
      return {};
    }
  }

  QByteArray reply;
  for (;;) {
    reply.append(socket.readAll());
    if (reply.contains("\n\n")) {
      return QString::fromUtf8(reply).trimmed();
    }
    if (deadline.hasExpired() ||
        !socket.waitForReadyRead(deadline.remainingTime())) {
      qWarning() << "Tunnel pipe reply failed or timed out:" << socket.errorString();
      return {};
    }
  }
}

}  // namespace WindowsTunnelPipe

#endif  // WINDOWSTUNNELPIPE_H

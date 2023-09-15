/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "windowspingsender.h"

#include <WS2tcpip.h>
#include <Windows.h>
#include <iphlpapi.h>
// Note: This important must come after the previous three.
// clang-format off
#include <IcmpAPI.h>
// clang-format on

#include <QtEndian>

#include "leakdetector.h"
#include "logger.h"
#include "windowscommons.h"
#include "platforms/windows/windowsutils.h"

#pragma comment(lib, "Ws2_32")

constexpr WORD WindowsPingPayloadSize = sizeof(quint16);

struct WindowsPingSenderPrivate {
  HANDLE m_handle;
  HANDLE m_event;
  unsigned char m_buffer[sizeof(ICMP_ECHO_REPLY) + WindowsPingPayloadSize + 8];
};

namespace {
Logger logger("WindowsPingSender");
}

static DWORD icmpCleanupHelper(LPVOID data) {
  struct WindowsPingSenderPrivate* p = (struct WindowsPingSenderPrivate*)data;
  if (p->m_event != INVALID_HANDLE_VALUE) {
    CloseHandle(p->m_event);
  }
  if (p->m_handle != INVALID_HANDLE_VALUE) {
    IcmpCloseHandle(p->m_handle);
  }
  delete p;
  return 0;
}

WindowsPingSender::WindowsPingSender(const QHostAddress& source,
                                     QObject* parent)
    : PingSender(parent) {
  MZ_COUNT_CTOR(WindowsPingSender);
  m_source = source;
  m_private = new struct WindowsPingSenderPrivate;
  m_private->m_handle = IcmpCreateFile();
  m_private->m_event = CreateEventA(NULL, FALSE, FALSE, NULL);

  m_notifier = new QWinEventNotifier(m_private->m_event, this);
  QObject::connect(m_notifier, &QWinEventNotifier::activated, this,
                   &WindowsPingSender::pingEventReady);

  memset(m_private->m_buffer, 0, sizeof(m_private->m_buffer));
}

WindowsPingSender::~WindowsPingSender() {
  MZ_COUNT_DTOR(WindowsPingSender);
  if (m_notifier) {
    delete m_notifier;
  }
  // Closing the ICMP handle can hang if there are lost ping replies. Moving
  // the cleanup into a separate thread avoids deadlocking the application.
  HANDLE h = CreateThread(NULL, 0, icmpCleanupHelper, m_private, 0, NULL);
  if (h == NULL) {
    icmpCleanupHelper(m_private);
  } else {
    CloseHandle(h);
  }
}

void WindowsPingSender::sendPing(const QHostAddress& dest, quint16 sequence) {
  if (m_private->m_handle == INVALID_HANDLE_VALUE) {
    return;
  }
  if (m_private->m_event == INVALID_HANDLE_VALUE) {
    return;
  }

  quint32 v4dst = dest.toIPv4Address();
  if (m_source.isNull()) {
    IcmpSendEcho2(m_private->m_handle, m_private->m_event, nullptr, nullptr,
                  qToBigEndian<quint32>(v4dst), &sequence, sizeof(sequence),
                  nullptr, m_private->m_buffer, sizeof(m_private->m_buffer),
                  10000);
  } else {
    quint32 v4src = m_source.toIPv4Address();
    IcmpSendEcho2Ex(m_private->m_handle, m_private->m_event, nullptr, nullptr,
                    qToBigEndian<quint32>(v4src), qToBigEndian<quint32>(v4dst),
                    &sequence, sizeof(sequence), nullptr, m_private->m_buffer,
                    sizeof(m_private->m_buffer), 10000);
  }

  DWORD status = GetLastError();
  if (status != ERROR_IO_PENDING) {
    QString errmsg = WindowsUtils::getErrorMessage();
    logger.error() << "failed to start Code: " << status
                   << " Message: " << errmsg
                   << " dest:" << logger.sensitive(dest.toString());
  }
}

void WindowsPingSender::pingEventReady() {
  DWORD replyCount =
      IcmpParseReplies(m_private->m_buffer, sizeof(m_private->m_buffer));
  if (replyCount == 0) {
    DWORD error = GetLastError();
    if (error == IP_REQ_TIMED_OUT) {
      return;
    }
    QString errmsg = WindowsUtils::getErrorMessage();
    logger.error() << "No ping reply. Code: " << error
                   << " Message: " << errmsg;
    return;
  }

  const ICMP_ECHO_REPLY* replies = (const ICMP_ECHO_REPLY*)m_private->m_buffer;
  for (DWORD i = 0; i < replyCount; i++) {
    if (replies[i].DataSize < sizeof(quint16)) {
      continue;
    }
    quint16 sequence;
    memcpy(&sequence, replies[i].Data, sizeof(quint16));
    emit recvPing(sequence);
  }
}

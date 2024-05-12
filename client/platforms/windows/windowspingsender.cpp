/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "windowspingsender.h"

#include <WS2tcpip.h>
#include <Windows.h>
#include <iphlpapi.h>
#include <winternl.h>

// Note: This important must come after the previous three.
// clang-format off
#include <IcmpAPI.h>
// clang-format on

#include <QtEndian>

#include "leakdetector.h"
#include "logger.h"
#include "platforms/windows/windowsutils.h"
#include "windowscommons.h"

#pragma comment(lib, "Ws2_32")

/*
 * On 64 Bit systems we need to use another struct.
 */
#ifdef _WIN64
using MZ_ICMP_ECHO_REPLY = ICMP_ECHO_REPLY32;
#else
using MZ_ICMP_ECHO_REPLY = ICMP_ECHO_REPLY;
#endif

constexpr WORD WindowsPingPayloadSize = sizeof(quint16);
constexpr size_t ICMP_ERR_SIZE = 8;
/*
 * IcmpSendEcho2 expects us to provide a Buffer that is
 * at least this size
 */
constexpr size_t MinimumReplyBufferSize =
    sizeof(ICMP_ECHO_REPLY) + WindowsPingPayloadSize + ICMP_ERR_SIZE +
    sizeof(IO_STATUS_BLOCK);
/**
 * ICMP_ECHO_REPLY32 is smaller than ICMP_ECHO_REPLY, so if we use that due to
 * binary compat Windows will add some padding.
 */
constexpr auto reply_padding =
    sizeof(ICMP_ECHO_REPLY) - sizeof(MZ_ICMP_ECHO_REPLY);

// Disable Packing, so the compiler does not add padding in this struct between
// different sized types.
#pragma pack(push, 1)
struct ICMP_ECHO_REPLY_BUFFER {
  MZ_ICMP_ECHO_REPLY reply;
  std::array<uint8_t, reply_padding> padding;
  quint16 payload;
  std::array<char8_t, ICMP_ERR_SIZE> icmp_error;
  IO_STATUS_BLOCK status;
};
#pragma pack(pop)

// If the Size is not the MinimumReplyBufferSize, the compiler added
// padding, so the fields will not be properly aligned with
// what IcmpSendEcho2 will write.
static_assert(sizeof(ICMP_ECHO_REPLY_BUFFER) == MinimumReplyBufferSize,
              "Fulfills the size requirements");

struct WindowsPingSenderPrivate {
  HANDLE m_handle;
  HANDLE m_event;
  ICMP_ECHO_REPLY_BUFFER m_replyBuffer;
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

  m_private->m_replyBuffer = {};
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
    IcmpSendEcho2(m_private->m_handle,               // IcmpHandle,
                  m_private->m_event,                // Event
                  nullptr,                           // ApcRoutine
                  nullptr,                           // ApcContext
                  qToBigEndian<quint32>(v4dst),      // DestinationAddress
                  &sequence,                         // RequestData
                  sizeof(sequence),                  // RequestSize
                  nullptr,                           // RequestOptions
                  &m_private->m_replyBuffer,         // [OUT] ReplyBuffer
                  sizeof(m_private->m_replyBuffer),  // ReplySize
                  10000                              // Timeout
    );
  } else {
    quint32 v4src = m_source.toIPv4Address();
    IcmpSendEcho2Ex(m_private->m_handle,               // IcmpHandle
                    m_private->m_event,                // Event
                    nullptr,                           // ApcRoutine
                    nullptr,                           // ApcContext
                    qToBigEndian<quint32>(v4src),      // SourceAddress
                    qToBigEndian<quint32>(v4dst),      // DestinationAddress
                    &sequence,                         // RequestData
                    sizeof(sequence),                  // RequestSize
                    nullptr,                           // RequestOptions
                    &m_private->m_replyBuffer,         // [OUT] ReplyBuffer
                    sizeof(m_private->m_replyBuffer),  // ReplySize
                    10000                              // Timeout
    );
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
  // Cleanup all data once we're done with m_replyBuffer.
  const auto guard = qScopeGuard([this]() { m_private->m_replyBuffer = {}; });

  DWORD replyCount = IcmpParseReplies(&m_private->m_replyBuffer,
                                      sizeof(m_private->m_replyBuffer));
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
  // We only allocated for one reply, so more should be impossible.
  if (replyCount != 1) {
    logger.error() << "Invalid amount of responses recieved";
    return;
  }
  if (m_private->m_replyBuffer.reply.Data == nullptr) {
    logger.error() << "Did get a ping response without payload";
    return;
  }
  // Assert that the (void*) pointer of Data is pointing
  // to our ReplyBuffer payload.
  if (m_private->m_replyBuffer.reply.Data == nullptr) {
    logger.error() << "Did get a ping response without payload";
    return;
  }
  // Assert that the (void*) pointer of Data is pointing
  // to our ReplyBuffer payload.
  assert(m_private->m_replyBuffer.reply.Data ==
         static_cast<PVOID>(&m_private->m_replyBuffer.payload));

  emit recvPing(m_private->m_replyBuffer.payload);
}

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "macospingsender.h"

#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <sys/errno.h>
#include <unistd.h>

#include <QSocketNotifier>
#include <QtEndian>

#include "leakdetector.h"
#include "logger.h"

namespace {

Logger logger("MacOSPingSender");

int identifier() { return (getpid() & 0xFFFF); }

};  // namespace

MacOSPingSender::MacOSPingSender(const QHostAddress& source, QObject* parent)
    : PingSender(parent) {
  MZ_COUNT_CTOR(MacOSPingSender);

  if (getuid()) {
    m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
  } else {
    m_socket = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
  }
  if (m_socket < 0) {
    logger.error() << "Socket creation failed";
    return;
  }

  quint32 ipv4addr = INADDR_ANY;
  if (!source.isNull()) {
    ipv4addr = source.toIPv4Address();
  }
  struct sockaddr_in addr;
  bzero(&addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_len = sizeof(addr);
  addr.sin_addr.s_addr = qToBigEndian<quint32>(ipv4addr);

  if (bind(m_socket, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
    logger.error() << "bind error:" << strerror(errno);
    return;
  }

  m_notifier = new QSocketNotifier(m_socket, QSocketNotifier::Read, this);
  connect(m_notifier, &QSocketNotifier::activated, this,
          &MacOSPingSender::socketReady);
}

MacOSPingSender::~MacOSPingSender() {
  MZ_COUNT_DTOR(MacOSPingSender);
  if (m_socket >= 0) {
    close(m_socket);
  }
}

void MacOSPingSender::sendPing(const QHostAddress& dest, quint16 sequence) {
  quint32 ipv4dest = dest.toIPv4Address();
  struct sockaddr_in addr;
  bzero(&addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_len = sizeof(addr);
  addr.sin_addr.s_addr = qToBigEndian<quint32>(ipv4dest);

  struct icmp packet;
  bzero(&packet, sizeof packet);
  packet.icmp_type = ICMP_ECHO;
  packet.icmp_id = identifier();
  packet.icmp_seq = htons(sequence);
  packet.icmp_cksum = inetChecksum(&packet, sizeof(packet));

  if (sendto(m_socket, (char*)&packet, sizeof(packet), MSG_NOSIGNAL,
             (struct sockaddr*)&addr, sizeof(addr)) != sizeof(packet)) {
    logger.error() << "ping sending failed:" << strerror(errno);
    emit criticalPingError();
    return;
  }
}

void MacOSPingSender::socketReady() {
  struct msghdr msg;
  bzero(&msg, sizeof(msg));

  struct sockaddr_in addr;
  msg.msg_name = (caddr_t)&addr;
  msg.msg_namelen = sizeof(addr);

  struct iovec iov;
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;

  u_char packet[IP_MAXPACKET];
  iov.iov_base = packet;
  iov.iov_len = IP_MAXPACKET;

  ssize_t rc = recvmsg(m_socket, &msg, MSG_DONTWAIT | MSG_NOSIGNAL);
  if (rc <= 0) {
    logger.error() << "Recvmsg failed:" << strerror(errno);
    return;
  }

  struct ip* ip = (struct ip*)packet;
  int hlen = ip->ip_hl << 2;
  struct icmp* icmp = (struct icmp*)(((char*)packet) + hlen);

  if (icmp->icmp_type == ICMP_ECHOREPLY && icmp->icmp_id == identifier()) {
    emit recvPing(htons(icmp->icmp_seq));
  }
}

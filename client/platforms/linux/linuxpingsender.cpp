/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "linuxpingsender.h"

#include <arpa/inet.h>
#include <errno.h>
#include <linux/filter.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <sys/socket.h>
#include <unistd.h>

#include <QSocketNotifier>
#include <QtEndian>

#include "leakdetector.h"
#include "logger.h"
#include "qhostaddress.h"

namespace {
Logger logger("LinuxPingSender");
}

int LinuxPingSender::createSocket() {
  // Try creating an ICMP socket. This would be the ideal choice, but it can
  // fail depending on the kernel config (see: sys.net.ipv4.ping_group_range)
  m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
  if (m_socket >= 0) {
    m_ident = 0;
    return m_socket;
  }
  if ((errno != EPERM) && (errno != EACCES)) {
    return -1;
  }

  // As a fallback, create a raw socket, which requires root permissions
  // or CAP_NET_RAW to be granted to the VPN client.
  m_socket = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
  if (m_socket < 0) {
    return -1;
  }
  m_ident = getpid() & 0xffff;

  // Attach a BPF filter to discard everything but replies to our echo.
  struct sock_filter bpf_prog[] = {
      BPF_STMT(BPF_LDX | BPF_B | BPF_MSH, 0), /* Skip IP header. */
      BPF_STMT(BPF_LD | BPF_H | BPF_IND, 4),  /* Load icmp echo ident */
      BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, m_ident, 1, 0), /* Ours? */
      BPF_STMT(BPF_RET | BPF_K, 0), /* Unexpected identifier. Reject. */
      BPF_STMT(BPF_LD | BPF_B | BPF_IND, 0), /* Load icmp type */
      BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, ICMP_ECHOREPLY, 1, 0), /* Echo? */
      BPF_STMT(BPF_RET | BPF_K, 0),   /* Unexpected type. Reject. */
      BPF_STMT(BPF_RET | BPF_K, ~0U), /* Packet passes the filter. */
  };
  struct sock_fprog filter = {
      .len = sizeof(bpf_prog) / sizeof(struct sock_filter),
      .filter = bpf_prog,
  };
  setsockopt(m_socket, SOL_SOCKET, SO_ATTACH_FILTER, &filter, sizeof(filter));

  return m_socket;
}

LinuxPingSender::LinuxPingSender(const QHostAddress& source, QObject* parent)
    : PingSender(parent) {
  MZ_COUNT_CTOR(LinuxPingSender);

  logger.debug() << "LinuxPingSender(" + logger.sensitive(source.toString()) +
                        ") created";

  m_socket = createSocket();
  if (m_socket < 0) {
    logger.error() << "Socket creation error: " << strerror(errno);
    return;
  }

  quint32 ipv4addr = INADDR_ANY;
  if (!source.isNull()) {
    ipv4addr = source.toIPv4Address();
  }
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof addr);
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = qToBigEndian<quint32>(ipv4addr);

  if (bind(m_socket, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
    close(m_socket);
    m_socket = -1;
    logger.error() << "bind error:" << strerror(errno);
    return;
  }

  m_notifier = new QSocketNotifier(m_socket, QSocketNotifier::Read, this);
  if (m_ident) {
    connect(m_notifier, &QSocketNotifier::activated, this,
            &LinuxPingSender::rawSocketReady);
  } else {
    connect(m_notifier, &QSocketNotifier::activated, this,
            &LinuxPingSender::icmpSocketReady);
  }
}

LinuxPingSender::~LinuxPingSender() {
  MZ_COUNT_DTOR(LinuxPingSender);
  if (m_socket >= 0) {
    close(m_socket);
  }
}

void LinuxPingSender::sendPing(const QHostAddress& dest, quint16 sequence) {
  quint32 ipv4dest = dest.toIPv4Address();
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = qToBigEndian<quint32>(ipv4dest);

  struct icmphdr packet;
  memset(&packet, 0, sizeof(packet));
  packet.type = ICMP_ECHO;
  packet.un.echo.id = htons(m_ident);
  packet.un.echo.sequence = htons(sequence);
  packet.checksum = inetChecksum(&packet, sizeof(packet));

  int rc = sendto(m_socket, &packet, sizeof(packet), 0, (struct sockaddr*)&addr,
                  sizeof(addr));
  if (rc < 0) {
    logger.error() << "failed to send:" << strerror(errno);
    if (errno == ENETUNREACH) {
        emit criticalPingError();
    }
  }
}

void LinuxPingSender::icmpSocketReady() {
  socklen_t slen = 0;
  unsigned char data[2048];
  int rc = recvfrom(m_socket, data, sizeof(data), MSG_DONTWAIT, NULL, &slen);
  if (rc <= 0) {
    logger.error() << "recvfrom failed:" << strerror(errno);
    return;
  }

  struct icmphdr packet;
  if (rc >= (int)sizeof(packet)) {
    memcpy(&packet, data, sizeof(packet));
    if (packet.type == ICMP_ECHOREPLY) {
      emit recvPing(htons(packet.un.echo.sequence));
    }
  }
}

void LinuxPingSender::rawSocketReady() {
  socklen_t slen = 0;
  unsigned char data[2048];
  int rc = recvfrom(m_socket, data, sizeof(data), MSG_DONTWAIT, NULL, &slen);
  if (rc <= 0) {
    logger.error() << "recvfrom failed:" << strerror(errno);
    return;
  }

  // Check the IP header
  const struct iphdr* ip = (struct iphdr*)data;
  int iphdrlen = ip->ihl * 4;
  if (rc < iphdrlen || iphdrlen < (int)sizeof(struct iphdr)) {
    logger.error() << "malformed IP packet:" << strerror(errno);
    return;
  }

  // Check the ICMP packet
  struct icmphdr packet;
  if (inetChecksum(data + iphdrlen, rc - iphdrlen) != 0) {
    logger.warning() << "invalid checksum";
    return;
  }
  if (rc >= (iphdrlen + (int)sizeof(packet))) {
    memcpy(&packet, data + iphdrlen, sizeof(packet));
    quint16 id = htons(m_ident);
    if ((packet.type == ICMP_ECHOREPLY) && (packet.un.echo.id == id)) {
      emit recvPing(htons(packet.un.echo.sequence));
    }
  }
}

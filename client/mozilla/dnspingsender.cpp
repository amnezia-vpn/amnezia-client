/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "dnspingsender.h"

#include <string.h>

#include <QNetworkDatagram>
#include <QtEndian>

#include "leakdetector.h"
#include "logger.h"

constexpr const quint16 DNS_PORT = 53;

// A quick and dirty DNS Header structure definition from RFC1035,
// Section 4.1.1: Header Section format.
struct dnsHeader {
  quint16 id;
  quint16 flags;
  quint16 qdcount;
  quint16 ancount;
  quint16 nscount;
  quint16 arcount;
};

// Bit definitions for the DNS flags field.
#define DNS_FLAG_QR 0x8000
#define DNS_FLAG_OPCODE 0x7800
#define DNS_FLAG_OPCODE_QUERY (0x0 << 11)
#define DNS_FLAG_OPCODE_IQUERY (0x1 << 11)
#define DNS_FLAG_OPCODE_STATUS (0x2 << 11)
#define DNS_FLAG_AA 0x0400
#define DNS_FLAG_TC 0x0200
#define DNS_FLAG_RD 0x0100
#define DNS_FLAG_RA 0x0080
#define DNS_FLAG_Z 0x0070
#define DNS_FLAG_RCODE 0x000F
#define DNS_FLAG_RCODE_NO_ERROR (0x0 << 0)
#define DNS_FLAG_RCODE_FORMAT_ERROR (0x1 << 0)
#define DNS_FLAG_RCODE_SERVER_FAILURE (0x2 << 0)
#define DNS_FLAG_RCODE_NAME_ERROR (0x3 << 0)
#define DNS_FLAG_RCODE_NOT_IMPLEMENTED (0x4 << 0)
#define DNS_FLAG_RCODE_REFUSED (0x5 << 0)

namespace {
Logger logger("DnsPingSender");
}

DnsPingSender::DnsPingSender(const QHostAddress& source, QObject* parent)
    : PingSender(parent) {
  MZ_COUNT_CTOR(DnsPingSender);

  if (source.isNull()) {
    m_socket.bind();
  } else {
    m_socket.bind(source);
  }

  connect(&m_socket, &QUdpSocket::readyRead, this, &DnsPingSender::readData);
}

DnsPingSender::~DnsPingSender() { MZ_COUNT_DTOR(DnsPingSender); }

void DnsPingSender::sendPing(const QHostAddress& dest, quint16 sequence) {
  QByteArray packet;

  // Assemble a DNS query header.
  struct dnsHeader header;
  memset(&header, 0, sizeof(header));
  header.id = qToBigEndian<quint16>(sequence);
  header.flags = qToBigEndian<quint16>(DNS_FLAG_OPCODE_QUERY);
  header.qdcount = qToBigEndian<quint16>(1);
  header.ancount = 0;
  header.nscount = 0;
  header.arcount = 0;
  packet.append(reinterpret_cast<char*>(&header), sizeof(header));

  // Add a query for the root nameserver: {<root>, type A, class IN}
  const char query[] = {0x00, 0x00, 0x01, 0x00, 0x01};
  packet.append(query, sizeof(query));

  // Send the datagram.
  m_socket.writeDatagram(packet, dest, DNS_PORT);
}

void DnsPingSender::readData() {
  while (m_socket.hasPendingDatagrams()) {
    QNetworkDatagram reply = m_socket.receiveDatagram();
    if (!reply.isValid()) {
      break;
    }

    // Extract the header from the DNS response.
    QByteArray payload = reply.data();
    struct dnsHeader header;
    if (payload.length() < static_cast<int>(sizeof(header))) {
      logger.debug() << "Received bogus DNS reply: truncated header";
      continue;
    }
    memcpy(&header, payload.constData(), sizeof(header));

    // Perfom some checks to ensure this is the reply we were expecting.
    quint16 flags = qFromBigEndian<quint16>(header.flags);
    if ((flags & DNS_FLAG_QR) == 0) {
      logger.debug() << "Received bogus DNS reply: QR == query";
      continue;
    }
    if ((flags & DNS_FLAG_OPCODE) != DNS_FLAG_OPCODE_QUERY) {
      logger.debug() << "Received bogus DNS reply: OPCODE != query";
      continue;
    }

    emit recvPing(qFromBigEndian<quint16>(header.id));
  }
}

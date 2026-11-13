/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "dnswildcardrouter.h"

#include <chrono>
#include <limits>
#include <memory>

#include <QNetworkInterface>
#include <QNetworkProxy>
#include <QTimer>

#include "leakdetector.h"
#include "logger.h"
#include "wildcardsite.h"

namespace {

Logger logger("DnsWildcardRouter");

constexpr int DNS_HEADER_SIZE = 12;
constexpr int DNS_RR_FIXED_SIZE = 10;
constexpr quint16 DNS_TYPE_A = 1;
constexpr quint16 DNS_TYPE_CNAME = 5;
constexpr quint16 DNS_TYPE_AAAA = 28;
constexpr quint16 DNS_TYPE_OPT = 41;
constexpr quint16 DNS_CLASS_IN = 1;
constexpr quint8 DNS_RCODE_NOERROR = 0;
constexpr quint8 DNS_RCODE_SERVFAIL = 2;
constexpr quint8 DNS_RCODE_EXTENDED_ERROR = 0xFF;
constexpr quint8 DNS_LABEL_POINTER_MASK = 0xC0;
constexpr int DNS_MAX_POINTER_DEPTH = 16;
constexpr int DNS_MAX_LABELS = 128;
constexpr int DNS_MAX_UDP_SIZE = 65535;
constexpr int DNS_MAX_TCP_SIZE = 65535;
constexpr int DNS_MAX_PENDING = 512;
constexpr int DNS_MAX_TCP_SESSIONS = 64;
constexpr int DNS_MAX_OUTSTANDING_PER_SESSION = 32;
// A UDP query is retransmitted, cycling through the upstreams, every
// attempt interval until one answers or the budget runs out. The answer to
// any attempt is taken, so a slow resolver is not cut off by a retransmit.
constexpr int DNS_UDP_ATTEMPT_MSEC = 1000;
constexpr int DNS_UDP_BUDGET_MSEC = 5000;
constexpr int DNS_UPSTREAM_TIMEOUT_MSEC = 3000;
constexpr int DNS_LOCAL_SNAPSHOT_MIN_AGE_MSEC = 5000;
constexpr int DNS_TCP_IDLE_TIMEOUT_MSEC = 10000;

qint64 monotonicNowMsec() {
  const auto now = std::chrono::steady_clock::now().time_since_epoch();
  return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
}

quint16 readU16(const QByteArray& buf, int offset) {
  return static_cast<quint16>((static_cast<quint8>(buf.at(offset)) << 8) |
                              static_cast<quint8>(buf.at(offset + 1)));
}

quint32 readU32(const QByteArray& buf, int offset) {
  return (static_cast<quint32>(static_cast<quint8>(buf.at(offset))) << 24) |
         (static_cast<quint32>(static_cast<quint8>(buf.at(offset + 1))) << 16) |
         (static_cast<quint32>(static_cast<quint8>(buf.at(offset + 2))) << 8) |
         static_cast<quint32>(static_cast<quint8>(buf.at(offset + 3)));
}

bool readName(const QByteArray& buf, int offset, DnsLabels* out, int* consumed,
              int depth = 0) {
  if (depth > DNS_MAX_POINTER_DEPTH || offset < 0) {
    return false;
  }

  DnsLabels labels;
  int cursor = offset;
  int used = 0;

  while (true) {
    if (cursor >= buf.size()) {
      return false;
    }
    const quint8 length = static_cast<quint8>(buf.at(cursor));

    if (length == 0) {
      used = cursor + 1 - offset;
      break;
    }

    if ((length & DNS_LABEL_POINTER_MASK) == DNS_LABEL_POINTER_MASK) {
      if (cursor + 1 >= buf.size()) {
        return false;
      }
      const int target = ((length & ~DNS_LABEL_POINTER_MASK) << 8) |
                         static_cast<quint8>(buf.at(cursor + 1));
      if (target >= buf.size()) {
        return false;
      }
      used = cursor + 2 - offset;
      DnsLabels suffix;
      int ignored = 0;
      if (!readName(buf, target, &suffix, &ignored, depth + 1)) {
        return false;
      }
      labels.append(suffix);
      break;
    }

    if (length & DNS_LABEL_POINTER_MASK) {
      return false;
    }

    cursor += 1;
    if (cursor + length > buf.size()) {
      return false;
    }
    labels.append(buf.mid(cursor, length));
    cursor += length;
    if (labels.size() > DNS_MAX_LABELS) {
      return false;
    }
  }

  *out = labels;
  *consumed = used;
  return true;
}

QByteArray nameKey(const DnsLabels& labels) {
  QByteArray key;
  for (const QByteArray& label : labels) {
    const QByteArray folded = WildcardSite::foldLabel(label);
    key.append(static_cast<char>(folded.size() & 0xFF));
    key.append(folded);
  }
  return key;
}

struct RawRecord {
  DnsLabels owner;
  quint16 type = 0;
  quint16 klass = 0;
  quint32 ttl = 0;
  int dataOffset = 0;
  quint16 dataLength = 0;
};

bool readRecord(const QByteArray& wire, int* offset, RawRecord* out) {
  int consumed = 0;
  RawRecord record;
  if (!readName(wire, *offset, &record.owner, &consumed)) {
    return false;
  }
  int cursor = *offset + consumed;
  if (cursor + DNS_RR_FIXED_SIZE > wire.size()) {
    return false;
  }
  record.type = readU16(wire, cursor);
  record.klass = readU16(wire, cursor + 2);
  record.ttl = readU32(wire, cursor + 4);
  record.dataLength = readU16(wire, cursor + 8);
  cursor += DNS_RR_FIXED_SIZE;
  if (cursor + record.dataLength > wire.size()) {
    return false;
  }
  record.dataOffset = cursor;
  *offset = cursor + record.dataLength;
  *out = record;
  return true;
}

}

DnsWildcardRouter::DnsWildcardRouter(QObject* parent) : QObject(parent) {
  MZ_COUNT_CTOR(DnsWildcardRouter);
}

DnsWildcardRouter::~DnsWildcardRouter() {
  MZ_COUNT_DTOR(DnsWildcardRouter);
  stop();
}

bool DnsWildcardRouter::sameName(const DnsLabels& left,
                                 const DnsLabels& right) {
  if (left.size() != right.size()) {
    return false;
  }
  for (int i = 0; i < left.size(); ++i) {
    if (WildcardSite::foldLabel(left.at(i)) !=
        WildcardSite::foldLabel(right.at(i))) {
      return false;
    }
  }
  return true;
}

QString DnsWildcardRouter::presentation(const DnsLabels& name) {
  QStringList parts;
  for (const QByteArray& label : name) {
    parts.append(QString::fromLatin1(label));
  }
  return parts.join(QLatin1Char('.'));
}

bool DnsWildcardRouter::parseEnvelope(const QByteArray& wire, Envelope* out) {
  if (wire.size() < DNS_HEADER_SIZE) {
    return false;
  }

  Envelope envelope;
  envelope.id = readU16(wire, 0);
  const quint16 flags = readU16(wire, 2);
  envelope.isResponse = (flags & 0x8000) != 0;
  envelope.opcode = static_cast<quint8>((flags >> 11) & 0x0F);
  envelope.truncated = (flags & 0x0200) != 0;
  envelope.rcode = static_cast<quint8>(flags & 0x000F);

  if (readU16(wire, 4) != 1) {
    return false;
  }

  int consumed = 0;
  if (!readName(wire, DNS_HEADER_SIZE, &envelope.qname, &consumed)) {
    return false;
  }
  const int cursor = DNS_HEADER_SIZE + consumed;
  if (cursor + 4 > wire.size()) {
    return false;
  }
  envelope.qtype = readU16(wire, cursor);
  envelope.qclass = readU16(wire, cursor + 2);
  envelope.questionEnd = cursor + 4;

  *out = envelope;
  return true;
}

bool DnsWildcardRouter::parseMessage(const QByteArray& wire, Message* out) {
  Envelope envelope;
  if (!parseEnvelope(wire, &envelope)) {
    return false;
  }

  const quint16 answerCount = readU16(wire, 6);
  const quint16 authorityCount = readU16(wire, 8);
  const quint16 additionalCount = readU16(wire, 10);
  const int total = static_cast<int>(answerCount) + authorityCount +
                    additionalCount;
  if (total > wire.size()) {
    return false;
  }

  int offset = envelope.questionEnd;
  QList<RawRecord> answers;
  answers.reserve(answerCount);

  for (int i = 0; i < total; ++i) {
    RawRecord record;
    if (!readRecord(wire, &offset, &record)) {
      return false;
    }
    if (record.type == DNS_TYPE_OPT) {
      const quint8 extended = static_cast<quint8>((record.ttl >> 24) & 0xFF);
      if (extended != 0) {
        envelope.rcode = DNS_RCODE_EXTENDED_ERROR;
      }
      continue;
    }
    if (i < answerCount) {
      answers.append(record);
    }
  }

  Message message;
  message.envelope = envelope;

  QHash<QByteArray, QList<int>> byOwner;
  for (int i = 0; i < answers.size(); ++i) {
    byOwner[nameKey(answers.at(i).owner)].append(i);
  }

  QHash<QByteArray, quint32> reached;
  reached.insert(nameKey(envelope.qname), std::numeric_limits<quint32>::max());

  QList<QByteArray> queue;
  queue.append(nameKey(envelope.qname));
  int budget = 4 * answers.size() + 64;

  for (int head = 0; head < queue.size() && budget > 0; ++head) {
    const QByteArray owner = queue.at(head);
    const quint32 inherited = reached.value(owner);
    for (int index : byOwner.value(owner)) {
      if (--budget <= 0) {
        break;
      }
      const RawRecord& record = answers.at(index);
      if (record.klass != DNS_CLASS_IN || record.type != DNS_TYPE_CNAME) {
        continue;
      }
      DnsLabels target;
      int consumed = 0;
      if (!readName(wire, record.dataOffset, &target, &consumed)) {
        return false;
      }
      if (consumed != record.dataLength) {
        return false;
      }
      const QByteArray key = nameKey(target);
      if (key.isEmpty() || reached.contains(key)) {
        continue;
      }
      reached.insert(key, qMin(inherited, record.ttl));
      queue.append(key);
    }
  }

  for (const RawRecord& record : answers) {
    if (record.klass != DNS_CLASS_IN) {
      continue;
    }
    const QByteArray key = nameKey(record.owner);
    if (!reached.contains(key)) {
      continue;
    }
    const quint32 ttl = qMin(reached.value(key), record.ttl);

    if (record.type == DNS_TYPE_A && record.dataLength == 4) {
      message.addresses.append(
          Record { QHostAddress(readU32(wire, record.dataOffset)), ttl });
    } else if (record.type == DNS_TYPE_AAAA && record.dataLength == 16) {
      Q_IPV6ADDR raw;
      for (int b = 0; b < 16; ++b) {
        raw[b] = static_cast<quint8>(wire.at(record.dataOffset + b));
      }
      message.addresses.append(Record { QHostAddress(raw), ttl });
    }
  }

  *out = message;
  return true;
}

bool DnsWildcardRouter::answers(const Envelope& reply, const Envelope& query) {
  return reply.isResponse && !query.isResponse && reply.id == query.id &&
         reply.opcode == query.opcode && reply.qtype == query.qtype &&
         reply.qclass == query.qclass && sameName(reply.qname, query.qname);
}

bool DnsWildcardRouter::mayLearnFrom(const Envelope& reply) {
  return reply.isResponse && reply.opcode == 0 &&
         reply.rcode == DNS_RCODE_NOERROR && !reply.truncated;
}

QByteArray DnsWildcardRouter::buildServFail(const QByteArray& query) {
  Envelope envelope;
  if (!parseEnvelope(query, &envelope)) {
    return QByteArray();
  }
  if (envelope.questionEnd <= 0 || envelope.questionEnd > query.size()) {
    return QByteArray();
  }

  QByteArray reply = query.left(envelope.questionEnd);
  quint16 flags = readU16(query, 2);
  flags |= 0x8000;
  flags |= 0x0080;
  flags &= ~0x0200;
  flags = static_cast<quint16>((flags & ~0x000F) | DNS_RCODE_SERVFAIL);
  reply[2] = static_cast<char>((flags >> 8) & 0xFF);
  reply[3] = static_cast<char>(flags & 0xFF);
  reply[4] = 0;
  reply[5] = 1;
  for (int i = 6; i < DNS_HEADER_SIZE; ++i) {
    reply[i] = 0;
  }
  return reply;
}

DnsWildcardRouter::FrameResult DnsWildcardRouter::takeFrame(
    QByteArray& buffer, QByteArray* message) {
  if (buffer.size() < 2) {
    return FrameResult::NeedMore;
  }
  const int length = readU16(buffer, 0);
  if (length == 0) {
    return FrameResult::Broken;
  }
  if (buffer.size() < length + 2) {
    return FrameResult::NeedMore;
  }
  *message = buffer.mid(2, length);
  buffer.remove(0, length + 2);
  return FrameResult::Taken;
}

QByteArray DnsWildcardRouter::frame(const QByteArray& message) {
  QByteArray framed;
  framed.append(static_cast<char>((message.size() >> 8) & 0xFF));
  framed.append(static_cast<char>(message.size() & 0xFF));
  framed.append(message);
  return framed;
}

bool DnsWildcardRouter::start(const QStringList& canonicalPatterns,
                              const QList<QHostAddress>& upstreams,
                              const QHostAddress& listenAddress, quint16 port,
                              quint16 upstreamPort) {
  stop();

  QStringList patterns;
  for (const QString& entry : canonicalPatterns) {
    const QString canonical = WildcardSite::canonicalize(entry);
    if (!canonical.isEmpty() && !patterns.contains(canonical)) {
      patterns.append(canonical);
    }
  }
  if (patterns.isEmpty()) {
    logger.debug() << "No usable wildcard patterns to watch for";
    return false;
  }
  if (upstreams.isEmpty()) {
    logger.warning() << "No upstream resolver to forward to";
    return false;
  }
  if (listenAddress.isNull()) {
    logger.warning() << "No address to listen on";
    return false;
  }

  auto udp = new QUdpSocket(this);
  if (!udp->bind(listenAddress, port, QAbstractSocket::DontShareAddress)) {
    logger.error() << "Failed to bind the DNS listener:" << udp->errorString();
    delete udp;
    return false;
  }

  auto tcp = new QTcpServer(this);
  if (!tcp->listen(listenAddress, port)) {
    logger.error() << "Failed to listen on TCP:" << tcp->errorString();
    delete tcp;
    udp->close();
    delete udp;
    return false;
  }

  m_udp = udp;
  m_tcp = tcp;
  m_upstreams = upstreams;
  m_patterns = patterns;
  m_listenAddress = listenAddress;
  m_upstreamPort = upstreamPort;
  m_preferredUpstream = 0;
  refreshLocalSnapshot();

  connect(m_udp, &QUdpSocket::readyRead, this, &DnsWildcardRouter::onUdpQuery);
  connect(m_tcp, &QTcpServer::newConnection, this,
          &DnsWildcardRouter::onTcpConnection);

  logger.debug() << "Watching DNS for" << patterns.count() << "pattern(s)";
  return true;
}

void DnsWildcardRouter::stop() {
  const QList<UdpRequest*> requests = m_udpRequests.values();
  for (UdpRequest* request : requests) {
    finishUdp(request);
  }
  m_udpRequests.clear();
  m_udpSockets.clear();

  const QList<QTcpSocket*> clients = m_tcpSessions.keys();
  for (QTcpSocket* client : clients) {
    closeTcp(m_tcpSessions.value(client, nullptr));
  }
  m_tcpSessions.clear();

  if (m_udp) {
    m_udp->close();
    m_udp->deleteLater();
    m_udp = nullptr;
  }
  if (m_tcp) {
    m_tcp->close();
    m_tcp->deleteLater();
    m_tcp = nullptr;
  }
  m_upstreams.clear();
  m_patterns.clear();
  m_listenAddress = QHostAddress();
  m_upstreamPort = 53;
  m_preferredUpstream = 0;
  m_localAddresses.clear();
  m_attachedSubnets.clear();
  m_localSnapshotMsec = 0;
}

bool DnsWildcardRouter::matchesConfiguration(
    const QStringList& canonicalPatterns, const QList<QHostAddress>& upstreams,
    const QHostAddress& listenAddress) const {
  if (!isRunning()) {
    return false;
  }
  QStringList patterns;
  for (const QString& entry : canonicalPatterns) {
    const QString canonical = WildcardSite::canonicalize(entry);
    if (!canonical.isEmpty() && !patterns.contains(canonical)) {
      patterns.append(canonical);
    }
  }
  return patterns == m_patterns && upstreams == m_upstreams &&
         listenAddress == m_listenAddress;
}

// static
bool DnsWildcardRouter::isLocalSource(const QHostAddress& source,
                                      const QHostAddress& listenAddress,
                                      const QList<QHostAddress>& localAddresses,
                                      const QList<Subnet>& attachedSubnets) {
  if (source.isNull()) {
    return false;
  }
  if (source.isLoopback() || source.isEqual(listenAddress)) {
    return true;
  }
  for (const QHostAddress& candidate : localAddresses) {
    if (candidate.isEqual(source)) {
      return true;
    }
  }
  for (const Subnet& subnet : attachedSubnets) {
    if (subnet.first.protocol() != source.protocol()) {
      continue;
    }
    if (source.isInSubnet(subnet.first, subnet.second)) {
      return true;
    }
  }
  return false;
}

void DnsWildcardRouter::refreshLocalSnapshot() {
  m_localAddresses.clear();
  m_attachedSubnets.clear();
  m_localSnapshotMsec = monotonicNowMsec();

  const QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
  for (const QNetworkInterface& iface : interfaces) {
    const QList<QNetworkAddressEntry> entries = iface.addressEntries();

    bool ownsListenAddress = false;
    for (const QNetworkAddressEntry& entry : entries) {
      if (entry.ip().isEqual(m_listenAddress)) {
        ownsListenAddress = true;
        break;
      }
    }

    // The interface that owns the listen address is the tunnel: its subnet
    // is the far side, and a peer there is not this machine.
    const bool attachable =
        !ownsListenAddress &&
        iface.flags().testFlag(QNetworkInterface::IsUp) &&
        !iface.flags().testFlag(QNetworkInterface::IsLoopBack);

    for (const QNetworkAddressEntry& entry : entries) {
      const QHostAddress ip = entry.ip();
      if (ip.isNull()) {
        continue;
      }
      m_localAddresses.append(ip);
      if (!attachable) {
        continue;
      }
      const int prefix = entry.prefixLength();
      const int width =
          ip.protocol() == QAbstractSocket::IPv6Protocol ? 128 : 32;
      // A host route says nothing about a neighbour, and an unknown or zero
      // prefix would make the whole Internet "attached".
      if (prefix <= 0 || prefix >= width) {
        continue;
      }
      m_attachedSubnets.append(Subnet(ip, prefix));
    }
  }
}

bool DnsWildcardRouter::isAcceptableClient(const QHostAddress& address) {
  if (isLocalSource(address, m_listenAddress, m_localAddresses,
                    m_attachedSubnets)) {
    return true;
  }
  // An interface that came up after start, a container bridge for example,
  // is picked up on the first query from it, at most once per interval.
  if (monotonicNowMsec() - m_localSnapshotMsec <
      DNS_LOCAL_SNAPSHOT_MIN_AGE_MSEC) {
    return false;
  }
  refreshLocalSnapshot();
  return isLocalSource(address, m_listenAddress, m_localAddresses,
                       m_attachedSubnets);
}

void DnsWildcardRouter::learn(const Message& message) {
  if (!mayLearnFrom(message.envelope)) {
    return;
  }
  if (message.addresses.isEmpty() || message.envelope.qname.isEmpty()) {
    return;
  }
  for (const QString& pattern : m_patterns) {
    if (!WildcardSite::matchesLabels(message.envelope.qname, pattern)) {
      continue;
    }
    logger.debug() << "A watched name resolved to"
                   << message.addresses.count() << "address(es)";
    emit addressesResolved(presentation(message.envelope.qname),
                           message.addresses);
    return;
  }
}

void DnsWildcardRouter::onUdpQuery() {
  while (m_udp && m_udp->hasPendingDatagrams()) {
    QByteArray query;
    query.resize(static_cast<int>(m_udp->pendingDatagramSize()));
    QHostAddress clientAddress;
    quint16 clientPort = 0;
    const qint64 read = m_udp->readDatagram(query.data(), query.size(),
                                            &clientAddress, &clientPort);
    if (read <= 0) {
      continue;
    }
    query.truncate(static_cast<int>(read));

    if (!isAcceptableClient(clientAddress)) {
      logger.warning() << "Ignoring a DNS query from outside this machine";
      continue;
    }

    if (query.size() > DNS_MAX_UDP_SIZE) {
      continue;
    }
    if (m_udpRequests.count() >= DNS_MAX_PENDING) {
      logger.warning() << "Too many DNS queries in flight";
      const QByteArray servfail = buildServFail(query);
      if (!servfail.isEmpty()) {
        m_udp->writeDatagram(servfail, clientAddress, clientPort);
      }
      continue;
    }

    Envelope envelope;
    if (!parseEnvelope(query, &envelope) || envelope.isResponse ||
        envelope.opcode != 0) {
      continue;
    }

    auto request = new UdpRequest;
    request->transaction.envelope = envelope;
    request->transaction.query = query;
    request->clientAddress = clientAddress;
    request->clientPort = clientPort;
    request->budgetDeadlineMsec = monotonicNowMsec() + DNS_UDP_BUDGET_MSEC;
    request->nextUpstream =
        m_upstreams.isEmpty() ? 0 : m_preferredUpstream % m_upstreams.count();

    request->timer = new QTimer(this);
    request->timer->setSingleShot(true);
    connect(request->timer, &QTimer::timeout, this,
            [this, request]() { onUdpAttemptTimeout(request); });
    m_udpRequests.insert(request);

    if (!sendUdpAttempt(request)) {
      failUdp(request);
      continue;
    }
    armUdpAttempt(request);
  }
}

bool DnsWildcardRouter::sendUdpAttempt(UdpRequest* request) {
  const int count = m_upstreams.count();
  for (int tried = 0; tried < count; ++tried) {
    const int index = request->nextUpstream % count;
    request->nextUpstream = (index + 1) % count;

    auto socket = new QUdpSocket(this);
    socket->setProxy(QNetworkProxy::NoProxy);
    socket->connectToHost(m_upstreams.at(index), m_upstreamPort);
    if (socket->write(request->transaction.query) !=
        request->transaction.query.size()) {
      logger.warning() << "Failed to send a query upstream";
      socket->deleteLater();
      continue;
    }

    request->attempts.insert(socket, index);
    m_udpSockets.insert(socket, request);
    connect(socket, &QUdpSocket::readyRead, this,
            [this, socket]() { onUdpReply(socket); });
    return true;
  }
  return false;
}

void DnsWildcardRouter::armUdpAttempt(UdpRequest* request) {
  const qint64 remaining = request->budgetDeadlineMsec - monotonicNowMsec();
  const qint64 wait = qMin<qint64>(DNS_UDP_ATTEMPT_MSEC, remaining);
  request->timer->start(wait > 0 ? static_cast<int>(wait) : 0);
}

void DnsWildcardRouter::onUdpAttemptTimeout(UdpRequest* request) {
  if (!m_udpRequests.contains(request)) {
    return;
  }
  if (request->budgetDeadlineMsec - monotonicNowMsec() <= 0) {
    failUdp(request);
    return;
  }
  // Retransmit to the next upstream, or to the same one when it is the only
  // one; the earlier attempts stay open for a late answer.
  if (!sendUdpAttempt(request) && request->attempts.isEmpty()) {
    failUdp(request);
    return;
  }
  armUdpAttempt(request);
}

void DnsWildcardRouter::finishUdp(UdpRequest* request) {
  if (!m_udpRequests.remove(request)) {
    return;
  }
  for (auto i = request->attempts.constBegin();
       i != request->attempts.constEnd(); ++i) {
    QUdpSocket* socket = i.key();
    m_udpSockets.remove(socket);
    socket->disconnect(this);
    socket->deleteLater();
  }
  request->attempts.clear();
  if (request->timer) {
    request->timer->stop();
    request->timer->disconnect(this);
    request->timer->deleteLater();
    request->timer = nullptr;
  }
  delete request;
}

void DnsWildcardRouter::failUdp(UdpRequest* request) {
  const QByteArray servfail = buildServFail(request->transaction.query);
  if (m_udp && !servfail.isEmpty()) {
    m_udp->writeDatagram(servfail, request->clientAddress,
                         request->clientPort);
  }
  finishUdp(request);
}

void DnsWildcardRouter::onUdpReply(QUdpSocket* socket) {
  UdpRequest* request = m_udpSockets.value(socket, nullptr);
  if (!request) {
    return;
  }

  while (socket->hasPendingDatagrams()) {
    QByteArray response;
    response.resize(static_cast<int>(socket->pendingDatagramSize()));
    const qint64 read = socket->readDatagram(response.data(), response.size());
    if (read <= 0) {
      continue;
    }
    response.truncate(static_cast<int>(read));

    Envelope replyEnvelope;
    if (!parseEnvelope(response, &replyEnvelope) ||
        !answers(replyEnvelope, request->transaction.envelope)) {
      logger.warning() << "Discarding a DNS reply that does not match";
      continue;
    }

    // The upstream that answered is asked first next time.
    m_preferredUpstream = request->attempts.value(socket, m_preferredUpstream);
    const QHostAddress clientAddress = request->clientAddress;
    const quint16 clientPort = request->clientPort;
    finishUdp(request);

    Message message;
    if (parseMessage(response, &message)) {
      learn(message);
    }

    if (m_udp) {
      m_udp->writeDatagram(response, clientAddress, clientPort);
    }
    return;
  }
}

void DnsWildcardRouter::onTcpConnection() {
  while (m_tcp && m_tcp->hasPendingConnections()) {
    QTcpSocket* client = m_tcp->nextPendingConnection();
    if (!client) {
      continue;
    }
    if (!isAcceptableClient(client->peerAddress()) ||
        m_tcpSessions.count() >= DNS_MAX_TCP_SESSIONS) {
      client->abort();
      client->deleteLater();
      continue;
    }

    auto session = new TcpSession;
    session->client = client;
    session->idle = new QTimer(client);
    session->idle->setSingleShot(true);
    session->idle->setInterval(DNS_TCP_IDLE_TIMEOUT_MSEC);
    session->deadline = new QTimer(client);
    session->deadline->setSingleShot(true);
    m_tcpSessions.insert(client, session);

    connect(client, &QTcpSocket::readyRead, this, [this, client]() {
      TcpSession* s = sessionFor(client);
      if (s) {
        s->idle->start();
        onTcpClientData(s);
      }
    });
    connect(client, &QTcpSocket::disconnected, this,
            [this, client]() { closeTcp(sessionFor(client)); });
    connect(session->idle, &QTimer::timeout, this,
            [this, client]() { closeTcp(sessionFor(client)); });
    connect(session->deadline, &QTimer::timeout, this, [this, client]() {
      TcpSession* s = sessionFor(client);
      if (s) {
        onTcpDeadline(s);
      }
    });
    session->idle->start();
  }
}

DnsWildcardRouter::TcpSession* DnsWildcardRouter::sessionFor(
    QTcpSocket* client) const {
  return m_tcpSessions.value(client, nullptr);
}

void DnsWildcardRouter::onTcpClientData(TcpSession* session) {
  session->fromClient.append(session->client->readAll());
  if (session->fromClient.size() > DNS_MAX_TCP_SIZE + 2) {
    closeTcp(session);
    return;
  }

  QByteArray query;
  while (true) {
    const FrameResult result = takeFrame(session->fromClient, &query);
    if (result == FrameResult::NeedMore) {
      return;
    }
    if (result == FrameResult::Broken) {
      failTcp(session);
      return;
    }

    Envelope envelope;
    if (!parseEnvelope(query, &envelope) || envelope.isResponse ||
        envelope.opcode != 0) {
      failTcp(session);
      return;
    }
    if (session->outstanding.size() >= DNS_MAX_OUTSTANDING_PER_SESSION) {
      failTcp(session);
      return;
    }

    Transaction transaction;
    transaction.envelope = envelope;
    transaction.query = query;
    transaction.upstreamIndex = session->upstreamIndex;
    transaction.deadlineMsec = monotonicNowMsec() + DNS_UPSTREAM_TIMEOUT_MSEC;
    session->outstanding.append(transaction);
    armTcpDeadline(session);

    QTcpSocket* client = session->client;
    if (!session->upstream) {
      if (!openTcpUpstream(session)) {
        if (sessionFor(client)) {
          failTcp(session);
        }
        return;
      }
      continue;
    }
    if (session->upstream->state() == QAbstractSocket::ConnectedState) {
      session->upstream->write(frame(query));
    }
  }
}

void DnsWildcardRouter::onTcpDeadline(TcpSession* session) {
  if (session->outstanding.isEmpty()) {
    return;
  }
  logger.warning() << "A TCP resolver went silent, trying the next one";
  failoverTcp(session);
}

void DnsWildcardRouter::resetTcpDeadlines(TcpSession* session) {
  const qint64 next = monotonicNowMsec() + DNS_UPSTREAM_TIMEOUT_MSEC;
  for (Transaction& transaction : session->outstanding) {
    transaction.deadlineMsec = next;
    transaction.upstreamIndex = session->upstreamIndex;
  }
}

void DnsWildcardRouter::failoverTcp(TcpSession* session) {
  QTcpSocket* client = session->client;
  dropTcpUpstream(session);
  ++session->upstreamIndex;
  session->reconnected = false;
  resetTcpDeadlines(session);

  if (!openTcpUpstream(session)) {
    if (sessionFor(client)) {
      failTcp(session);
    }
    return;
  }
  armTcpDeadline(session);
}

void DnsWildcardRouter::onTcpUpstreamError(QTcpSocket* client, int attempt,
                                           QAbstractSocket::SocketError error) {
  TcpSession* session = sessionFor(client);
  if (!session || session->attempt != attempt) {
    return;
  }
  if (session->outstanding.isEmpty()) {
    dropTcpUpstream(session);
    return;
  }

  // A resolver may close a connection with queries outstanding when it runs
  // short of resources; that is a reason to reconnect to it, not to give up
  // on it (RFC 7766, section 6.2.4). One reconnect per round of progress, so
  // a server that closes without ever answering is still failed over.
  if (error == QAbstractSocket::RemoteHostClosedError &&
      !session->reconnected) {
    logger.debug() << "A TCP resolver closed the connection, reconnecting";
    dropTcpUpstream(session);
    session->reconnected = true;
    resetTcpDeadlines(session);
    if (!openTcpUpstream(session)) {
      if (sessionFor(client)) {
        failTcp(session);
      }
      return;
    }
    armTcpDeadline(session);
    return;
  }

  failoverTcp(session);
}

bool DnsWildcardRouter::openTcpUpstream(TcpSession* session) {
  if (session->upstreamIndex >= m_upstreams.count()) {
    return false;
  }

  ++session->attempt;
  const int attempt = session->attempt;
  session->fromUpstream.clear();

  auto upstream = new QTcpSocket(this);
  upstream->setProxy(QNetworkProxy::NoProxy);
  session->upstream = upstream;
  QTcpSocket* client = session->client;

  connect(upstream, &QTcpSocket::connected, this, [this, client]() {
    TcpSession* s = sessionFor(client);
    if (!s || !s->upstream) {
      return;
    }
    for (const Transaction& transaction : s->outstanding) {
      s->upstream->write(frame(transaction.query));
    }
  });
  connect(upstream, &QTcpSocket::readyRead, this, [this, client]() {
    TcpSession* s = sessionFor(client);
    if (s) {
      s->idle->start();
      onTcpUpstreamData(s);
    }
  });
  connect(upstream, &QTcpSocket::errorOccurred, this,
          [this, client, attempt](QAbstractSocket::SocketError error) {
            // Deferred, so answers that arrived together with the close are
            // read before the outstanding list is judged.
            QTimer::singleShot(0, this, [this, client, attempt, error]() {
              onTcpUpstreamError(client, attempt, error);
            });
          });

  upstream->connectToHost(m_upstreams.at(session->upstreamIndex),
                          m_upstreamPort);
  return sessionFor(client) == session;
}

void DnsWildcardRouter::onTcpUpstreamData(TcpSession* session) {
  session->fromUpstream.append(session->upstream->readAll());
  if (session->fromUpstream.size() > DNS_MAX_TCP_SIZE + 2) {
    closeTcp(session);
    return;
  }

  QByteArray response;
  while (true) {
    const FrameResult result = takeFrame(session->fromUpstream, &response);
    if (result == FrameResult::NeedMore) {
      break;
    }
    if (result == FrameResult::Broken) {
      failTcp(session);
      return;
    }

    Envelope replyEnvelope;
    if (!parseEnvelope(response, &replyEnvelope)) {
      continue;
    }

    int matched = -1;
    for (int i = 0; i < session->outstanding.size(); ++i) {
      if (answers(replyEnvelope, session->outstanding.at(i).envelope)) {
        matched = i;
        break;
      }
    }
    if (matched < 0) {
      logger.warning() << "Discarding a TCP reply nobody asked for";
      continue;
    }
    session->outstanding.removeAt(matched);
    session->reconnected = false;

    if (mayLearnFrom(replyEnvelope)) {
      Message message;
      if (parseMessage(response, &message)) {
        learn(message);
      }
    }

    if (session->client->state() == QAbstractSocket::ConnectedState) {
      session->client->write(frame(response));
    }
  }

  armTcpDeadline(session);
}

void DnsWildcardRouter::armTcpDeadline(TcpSession* session) {
  if (!session->deadline) {
    return;
  }
  if (session->outstanding.isEmpty()) {
    session->deadline->stop();
    return;
  }

  qint64 earliest = session->outstanding.first().deadlineMsec;
  for (const Transaction& transaction : session->outstanding) {
    earliest = qMin(earliest, transaction.deadlineMsec);
  }
  const qint64 remaining = earliest - monotonicNowMsec();
  session->deadline->start(remaining > 0 ? static_cast<int>(remaining) : 0);
}

void DnsWildcardRouter::dropTcpUpstream(TcpSession* session) {
  if (session->upstream) {
    session->upstream->disconnect(this);
    session->upstream->abort();
    session->upstream->deleteLater();
    session->upstream = nullptr;
  }
  session->fromUpstream.clear();
}

void DnsWildcardRouter::failTcp(TcpSession* session) {
  if (!session) {
    return;
  }
  for (const Transaction& transaction : session->outstanding) {
    const QByteArray servfail = buildServFail(transaction.query);
    if (!servfail.isEmpty() && session->client &&
        session->client->state() == QAbstractSocket::ConnectedState) {
      session->client->write(frame(servfail));
    }
  }
  session->outstanding.clear();
  retireTcp(session);
}

void DnsWildcardRouter::closeTcp(TcpSession* session) { retireTcp(session); }

void DnsWildcardRouter::retireTcp(TcpSession* session) {
  if (!session) {
    return;
  }
  m_tcpSessions.remove(session->client);

  if (session->idle) {
    session->idle->stop();
  }
  if (session->deadline) {
    session->deadline->stop();
  }

  dropTcpUpstream(session);

  QTcpSocket* client = session->client;
  if (client) {
    client->disconnect(this);
    if (client->state() == QAbstractSocket::ConnectedState &&
        client->bytesToWrite() > 0) {
      connect(client, &QTcpSocket::disconnected, client,
              &QObject::deleteLater);
      client->disconnectFromHost();
    } else {
      client->abort();
      client->deleteLater();
    }
  }
  delete session;
}

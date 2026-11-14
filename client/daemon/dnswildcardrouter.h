/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DNSWILDCARDROUTER_H
#define DNSWILDCARDROUTER_H

#include <memory>

#include <QByteArray>
#include <QHash>
#include <QHostAddress>
#include <QList>
#include <QObject>
#include <QPair>
#include <QSet>
#include <QStringList>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>

class QTimer;

using DnsLabels = QList<QByteArray>;

class DnsWildcardRouter final : public QObject {
  Q_OBJECT

 public:
  struct Record {
    QHostAddress address;
    quint32 ttl = 0;
  };

  struct Envelope {
    quint16 id = 0;
    bool isResponse = false;
    quint8 opcode = 0;
    bool truncated = false;
    quint8 rcode = 0;
    DnsLabels qname;
    quint16 qtype = 0;
    quint16 qclass = 0;
    int questionEnd = 0;
  };

  struct Message {
    Envelope envelope;
    QList<Record> addresses;
  };

  explicit DnsWildcardRouter(QObject* parent = nullptr);
  ~DnsWildcardRouter();

  static bool sameName(const DnsLabels& left, const DnsLabels& right);

  static QString presentation(const DnsLabels& name);

  static bool parseEnvelope(const QByteArray& wire, Envelope* out);

  static bool parseMessage(const QByteArray& wire, Message* out);

  static bool answers(const Envelope& reply, const Envelope& query);

  static bool mayLearnFrom(const Envelope& reply);

  static QByteArray buildServFail(const QByteArray& query);

  enum class FrameResult {
    NeedMore,
    Taken,
    Broken,
  };

  static FrameResult takeFrame(QByteArray& buffer, QByteArray* message);

  static QByteArray frame(const QByteArray& message);

  using Subnet = QPair<QHostAddress, int>;

  // A query is served only when it comes from this machine: loopback, one of
  // its own addresses, or a host on a subnet directly attached to one of its
  // non-tunnel interfaces (a container bridge, for example). The subnet of
  // the interface that owns the listen address is the tunnel, and a peer on
  // the far side of it is refused.
  static bool isLocalSource(const QHostAddress& source,
                            const QHostAddress& listenAddress,
                            const QList<QHostAddress>& localAddresses,
                            const QList<Subnet>& attachedSubnets);

  bool start(const QStringList& canonicalPatterns,
             const QList<QHostAddress>& upstreams,
             const QHostAddress& listenAddress, quint16 port = 53,
             quint16 upstreamPort = 53);
  void stop();

  bool isRunning() const { return m_udp != nullptr; }
  QHostAddress listenAddress() const { return m_listenAddress; }

  bool matchesConfiguration(const QStringList& canonicalPatterns,
                            const QList<QHostAddress>& upstreams,
                            const QHostAddress& listenAddress) const;

 signals:
  void addressesResolved(const QString& name, const QList<Record>& records);

 private:
  struct Transaction {
    Envelope envelope;
    QByteArray query;
    int upstreamIndex = 0;
    qint64 deadlineMsec = 0;
  };

  // One client query. Every attempt has its own socket connected to the
  // upstream it was sent to, and stays open until the query completes, so a
  // late answer to an earlier attempt is still taken.
  struct UdpRequest {
    Transaction transaction;
    QHostAddress clientAddress;
    quint16 clientPort = 0;
    QHash<QUdpSocket*, int> attempts;
    QTimer* timer = nullptr;
    qint64 budgetDeadlineMsec = 0;
    int nextUpstream = 0;
  };

  struct TcpSession {
    QTcpSocket* client = nullptr;
    QTcpSocket* upstream = nullptr;
    QByteArray fromClient;
    QByteArray fromUpstream;
    QList<Transaction> outstanding;
    int upstreamIndex = 0;
    QTimer* idle = nullptr;
    QTimer* deadline = nullptr;
    int attempt = 0;
    bool reconnected = false;
  };

  void onUdpQuery();
  void onUdpReply(QUdpSocket* socket);
  void onUdpAttemptTimeout(UdpRequest* request);
  bool sendUdpAttempt(UdpRequest* request);
  void armUdpAttempt(UdpRequest* request);
  void finishUdp(UdpRequest* request);
  void failUdp(UdpRequest* request);

  void onTcpConnection();
  void onTcpClientData(TcpSession* session);
  void onTcpUpstreamData(TcpSession* session);
  void onTcpDeadline(TcpSession* session);
  void onTcpUpstreamError(QTcpSocket* client, int attempt,
                          QAbstractSocket::SocketError error);
  void failoverTcp(TcpSession* session);
  void resetTcpDeadlines(TcpSession* session);
  void armTcpDeadline(TcpSession* session);
  void dropTcpUpstream(TcpSession* session);
  bool openTcpUpstream(TcpSession* session);
  void failTcp(TcpSession* session);
  void closeTcp(TcpSession* session);
  void retireTcp(TcpSession* session);
  TcpSession* sessionFor(QTcpSocket* client) const;

  bool isAcceptableClient(const QHostAddress& address);
  void refreshLocalSnapshot();
  void learn(const Message& message);

  QUdpSocket* m_udp = nullptr;
  QTcpServer* m_tcp = nullptr;
  QHash<QUdpSocket*, UdpRequest*> m_udpSockets;
  QSet<UdpRequest*> m_udpRequests;
  QHash<QTcpSocket*, TcpSession*> m_tcpSessions;
  QList<QHostAddress> m_upstreams;
  QStringList m_patterns;
  QHostAddress m_listenAddress;
  quint16 m_upstreamPort = 53;
  int m_preferredUpstream = 0;
  QList<QHostAddress> m_localAddresses;
  QList<Subnet> m_attachedSubnets;
  qint64 m_localSnapshotMsec = 0;
};

#endif  // DNSWILDCARDROUTER_H

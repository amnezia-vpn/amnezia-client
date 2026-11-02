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

  struct UdpRequest {
    Transaction transaction;
    QHostAddress clientAddress;
    quint16 clientPort = 0;
    QUdpSocket* socket = nullptr;
    QTimer* timer = nullptr;
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
  };

  void onUdpQuery();
  void onUdpReply(QUdpSocket* socket);
  void onUdpTimeout(QUdpSocket* socket);
  bool sendUdp(std::unique_ptr<UdpRequest>& request);
  std::unique_ptr<UdpRequest> takeUdp(QUdpSocket* socket);
  void failUdp(std::unique_ptr<UdpRequest> request);

  void onTcpConnection();
  void onTcpClientData(TcpSession* session);
  void onTcpUpstreamData(TcpSession* session);
  void onTcpDeadline(TcpSession* session);
  void armTcpDeadline(TcpSession* session);
  void dropTcpUpstream(TcpSession* session);
  bool openTcpUpstream(TcpSession* session);
  void failTcp(TcpSession* session);
  void closeTcp(TcpSession* session);
  void retireTcp(TcpSession* session);
  TcpSession* sessionFor(QTcpSocket* client) const;

  bool isAcceptableClient(const QHostAddress& address) const;
  void learn(const Message& message);

  QUdpSocket* m_udp = nullptr;
  QTcpServer* m_tcp = nullptr;
  QHash<QUdpSocket*, UdpRequest*> m_udpRequests;
  QHash<QTcpSocket*, TcpSession*> m_tcpSessions;
  QList<QHostAddress> m_upstreams;
  QStringList m_patterns;
  QHostAddress m_listenAddress;
  quint16 m_upstreamPort = 53;
  QList<QHostAddress> m_localAddresses;
};

#endif  // DNSWILDCARDROUTER_H

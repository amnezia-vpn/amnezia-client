/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "wireguardutilslinux.h"
#include "leakdetector.h"
#include "logger.h"
#include "platforms/linux/linuxdependencies.h"

#include <QHostAddress>
#include <QScopeGuard>

#include <arpa/inet.h>
#include <linux/fib_rules.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <mntent.h>
#include <net/if.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

// Import wireguard C library for Linux
#if defined(__cplusplus)
extern "C" {
#endif
#include "../../3rdparty/wireguard-tools/contrib/embeddable-wg-library/wireguard.h"
#include "../../linux/netfilter/netfilter.h"
#if defined(__cplusplus)
}
#endif
// End import wireguard

/* Packets sent outside the VPN need to be marked for the routing policy
 * to direct them appropriately. The value of the mark and the table ID
 * aren't important, so long as they are unique.
 */
constexpr uint32_t WG_FIREWALL_MARK = 0xca6c;
constexpr uint32_t WG_ROUTE_TABLE = 0xca6c;

/* Traffic classifiers can be used to mark packets which should be either
 * excluded from the VPN tunnel, or blocked entirely. The values of these
 * classifiers aren't important so long as they are unique.
 */
constexpr const char* VPN_EXCLUDE_CGROUP = "/mozvpn.exclude";
constexpr const char* VPN_BLOCK_CGROUP = "/mozvpn.block";
constexpr uint32_t VPN_EXCLUDE_CLASS_ID = 0x00110011;
constexpr uint32_t VPN_BLOCK_CLASS_ID = 0x00220022;

static void nlmsg_append_attr(char* buf, size_t maxlen, int attrtype,
                              const void* attrdata, size_t attrlen);
static void nlmsg_append_attr32(char* buf, size_t maxlen, int attrtype,
                                uint32_t value);

namespace {
Logger logger(LOG_LINUX, "WireguardUtilsLinux");

void NetfilterLogger(int level, const char* msg) {
  Q_UNUSED(level);
  logger.debug() << "NetfilterGo:" << msg;
}
}  // namespace

WireguardUtilsLinux::WireguardUtilsLinux(QObject* parent)
    : WireguardUtils(parent) {
  MVPN_COUNT_CTOR(WireguardUtilsLinux);
  NetfilterSetLogger((GoUintptr)&NetfilterLogger);
  NetfilterCreateTables();

  m_nlsock = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);
  if (m_nlsock < 0) {
    logger.warning() << "Failed to create netlink socket:" << strerror(errno);
  }

  struct sockaddr_nl nladdr;
  memset(&nladdr, 0, sizeof(nladdr));
  nladdr.nl_family = AF_NETLINK;
  nladdr.nl_pid = getpid();
  if (bind(m_nlsock, (struct sockaddr*)&nladdr, sizeof(nladdr)) != 0) {
    logger.warning() << "Failed to bind netlink socket:" << strerror(errno);
  }

  m_notifier = new QSocketNotifier(m_nlsock, QSocketNotifier::Read, this);
  connect(m_notifier, &QSocketNotifier::activated, this,
          &WireguardUtilsLinux::nlsockReady);

  /* Create control groups for split tunnelling */
  m_cgroups = LinuxDependencies::findCgroupPath("net_cls");
  if (!m_cgroups.isNull()) {
    if (!setupCgroupClass(m_cgroups + VPN_EXCLUDE_CGROUP,
                          VPN_EXCLUDE_CLASS_ID)) {
      m_cgroups.clear();
    } else if (!setupCgroupClass(m_cgroups + VPN_BLOCK_CGROUP,
                                 VPN_BLOCK_CLASS_ID)) {
      m_cgroups.clear();
    }
  }

  logger.debug() << "WireguardUtilsLinux created.";
}

WireguardUtilsLinux::~WireguardUtilsLinux() {
  MVPN_COUNT_DTOR(WireguardUtilsLinux);
  NetfilterRemoveTables();
  if (m_nlsock >= 0) {
    close(m_nlsock);
  }
  logger.debug() << "WireguardUtilsLinux destroyed.";
}

bool WireguardUtilsLinux::interfaceExists() {
  // As currentInterfaces only gets wireguard interfaces, this method
  // also confirms an interface as being a wireguard interface.
  return currentInterfaces().contains(WG_INTERFACE);
};

bool WireguardUtilsLinux::addInterface(const InterfaceConfig& config) {
  int code = wg_add_device(WG_INTERFACE);
  if (code != 0) {
    logger.error() << "Adding interface failed:" << strerror(-code);
    return false;
  }

  wg_device* device = static_cast<wg_device*>(calloc(1, sizeof(*device)));
  if (!device) {
    logger.error() << "Allocation failure";
    return false;
  }
  auto guard = qScopeGuard([&] { wg_free_device(device); });

  // Name
  strncpy(device->name, WG_INTERFACE, IFNAMSIZ);
  // Private Key
  wg_key_from_base64(device->private_key, config.m_privateKey.toLocal8Bit());

  // Set/update device
  device->fwmark = WG_FIREWALL_MARK;
  device->flags = (wg_device_flags)(
      WGDEVICE_HAS_PRIVATE_KEY | WGDEVICE_REPLACE_PEERS | WGDEVICE_HAS_FWMARK);
  if (wg_set_device(device) != 0) {
    logger.error() << "Failed to setup the device";
    return false;
  }

  // Create routing policy rules
  if (!rtmSendRule(RTM_NEWRULE,
                   NLM_F_REQUEST | NLM_F_CREATE | NLM_F_REPLACE | NLM_F_ACK,
                   AF_INET)) {
    return false;
  }
  if (!rtmSendRule(RTM_NEWRULE,
                   NLM_F_REQUEST | NLM_F_CREATE | NLM_F_REPLACE | NLM_F_ACK,
                   AF_INET6)) {
    return false;
  }

  // Configure firewall rules
  GoString goIfname = {.p = device->name, .n = (ptrdiff_t)strlen(device->name)};
  if (NetfilterIfup(goIfname, device->fwmark) != 0) {
    return false;
  }
  if (!m_cgroups.isNull()) {
    NetfilterMarkCgroup(VPN_EXCLUDE_CLASS_ID, device->fwmark);
    NetfilterBlockCgroup(VPN_BLOCK_CLASS_ID);
  }

  int slashPos = config.m_deviceIpv6Address.indexOf('/');
  GoString goIpv6Address = {.p = qPrintable(config.m_deviceIpv6Address),
                            .n = config.m_deviceIpv6Address.length()};
  if (slashPos != -1) {
    goIpv6Address.n = slashPos;
  }
  NetfilterIsolateIpv6(goIfname, goIpv6Address);

  return true;
}

bool WireguardUtilsLinux::updatePeer(const InterfaceConfig& config) {
  wg_device* device = static_cast<wg_device*>(calloc(1, sizeof(*device)));
  if (!device) {
    logger.error() << "Allocation failure";
    return false;
  }
  auto guard = qScopeGuard([&] { wg_free_device(device); });

  wg_peer* peer = static_cast<wg_peer*>(calloc(1, sizeof(*peer)));
  if (!peer) {
    logger.error() << "Allocation failure";
    return false;
  }
  device->first_peer = device->last_peer = peer;

  logger.debug() << "Adding peer" << printablePubkey(config.m_serverPublicKey);

  // Public Key
  wg_key_from_base64(peer->public_key, qPrintable(config.m_serverPublicKey));
  // Endpoint
  if (!setPeerEndpoint(&peer->endpoint.addr, config.m_serverIpv4AddrIn,
                       config.m_serverPort)) {
    logger.error() << "Failed to set peer endpoint for hop"
                   << config.m_hopindex;
    return false;
  }

  // HACK: We are running into a crash on Linux due to the address list being
  // *WAAAY* too long, which we aren't really using anways since the routing
  // tables are doing all the work for us anyways.
  //
  // To work around the issue, just set default routes for hopindex zero.
  if (config.m_hopindex == 0) {
    if (!config.m_deviceIpv4Address.isNull()) {
      addPeerPrefix(peer, IPAddressRange("0.0.0.0", 0, IPAddressRange::IPv4));
    }
    if (!config.m_deviceIpv6Address.isNull()) {
      addPeerPrefix(peer, IPAddressRange("::", 0, IPAddressRange::IPv6));
    }
  } else {
    for (const IPAddressRange& ip : config.m_allowedIPAddressRanges) {
      bool ok = addPeerPrefix(peer, ip);
      if (!ok) {
        logger.error() << "Invalid IP address:" << ip.ipAddress();
        return false;
      }
    }
  }

  // Set/update peer
  strncpy(device->name, WG_INTERFACE, IFNAMSIZ);
  device->flags = (wg_device_flags)0;
  peer->flags =
      (wg_peer_flags)(WGPEER_HAS_PUBLIC_KEY | WGPEER_REPLACE_ALLOWEDIPS);
  if (wg_set_device(device) != 0) {
    logger.error() << "Failed to set the new peer hop" << config.m_hopindex;
    return false;
  }

  return true;
}

bool WireguardUtilsLinux::deletePeer(const QString& pubkey) {
  wg_device* device = static_cast<wg_device*>(calloc(1, sizeof(*device)));
  if (!device) {
    logger.error() << "Allocation failure";
    return false;
  }
  auto guard = qScopeGuard([&] { wg_free_device(device); });

  wg_peer* peer = static_cast<wg_peer*>(calloc(1, sizeof(*peer)));
  if (!peer) {
    logger.error() << "Allocation failure";
    return false;
  }
  device->first_peer = device->last_peer = peer;

  logger.debug() << "Removing peer" << printablePubkey(pubkey);

  // Public Key
  peer->flags = (wg_peer_flags)(WGPEER_HAS_PUBLIC_KEY | WGPEER_REMOVE_ME);
  wg_key_from_base64(peer->public_key, qPrintable(pubkey));

  // Set/update device
  strncpy(device->name, WG_INTERFACE, IFNAMSIZ);
  device->flags = (wg_device_flags)0;
  if (wg_set_device(device) != 0) {
    logger.error() << "Failed to remove the peer";
    return false;
  }

  return true;
}

bool WireguardUtilsLinux::deleteInterface() {
  // Clear firewall rules
  NetfilterClearTables();

  // Clear routing policy rules
  if (!rtmSendRule(RTM_DELRULE, NLM_F_REQUEST | NLM_F_ACK, AF_INET)) {
    return false;
  }
  if (!rtmSendRule(RTM_DELRULE, NLM_F_REQUEST | NLM_F_ACK, AF_INET6)) {
    return false;
  }

  // Delete the interface
  int returnCode = wg_del_device(WG_INTERFACE);
  if (returnCode != 0) {
    logger.error() << "Deleting interface failed:" << strerror(-returnCode);
    return false;
  }

  return true;
}

WireguardUtils::peerStatus WireguardUtilsLinux::getPeerStatus(
    const QString& pubkey) {
  wg_device* device = nullptr;
  wg_peer* peer = nullptr;
  peerStatus status = {0, 0};

  if (wg_get_device(&device, WG_INTERFACE) != 0) {
    logger.warning() << "Unable to get stats for" << WG_INTERFACE;
    return status;
  }

  wg_key key;
  wg_key_from_base64(key, qPrintable(pubkey));
  wg_for_each_peer(device, peer) {
    if (memcmp(&key, &peer->public_key, sizeof(key)) != 0) {
      continue;
    }
    status.txBytes = peer->tx_bytes;
    status.rxBytes = peer->rx_bytes;
    break;
  }
  wg_free_device(device);
  return status;
}

bool WireguardUtilsLinux::updateRoutePrefix(const IPAddressRange& prefix,
                                            int hopindex) {
  logger.debug() << "Adding route to" << prefix.toString();
  int flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_REPLACE | NLM_F_ACK;
  return rtmSendRoute(RTM_NEWROUTE, flags, prefix, hopindex);
}

bool WireguardUtilsLinux::deleteRoutePrefix(const IPAddressRange& prefix,
                                            int hopindex) {
  logger.debug() << "Removing route to" << prefix.toString();
  int flags = NLM_F_REQUEST | NLM_F_ACK;
  return rtmSendRoute(RTM_DELROUTE, flags, prefix, hopindex);
}

bool WireguardUtilsLinux::rtmSendRoute(int action, int flags,
                                       const IPAddressRange& prefix,
                                       int hopindex) {
  constexpr size_t rtm_max_size = sizeof(struct rtmsg) +
                                  2 * RTA_SPACE(sizeof(uint32_t)) +
                                  RTA_SPACE(sizeof(struct in6_addr));
  int index = if_nametoindex(WG_INTERFACE);
  if (index <= 0) {
    logger.error() << "if_nametoindex() failed:" << strerror(errno);
    return false;
  }

  wg_allowedip ip;
  if (!buildAllowedIp(&ip, prefix)) {
    logger.warning() << "Invalid destination prefix";
    return false;
  }

  char buf[NLMSG_SPACE(rtm_max_size)];
  struct nlmsghdr* nlmsg = (struct nlmsghdr*)buf;
  struct rtmsg* rtm = (struct rtmsg*)NLMSG_DATA(nlmsg);

  memset(buf, 0, sizeof(buf));
  nlmsg->nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
  nlmsg->nlmsg_type = action;
  nlmsg->nlmsg_flags = flags;
  nlmsg->nlmsg_pid = getpid();
  nlmsg->nlmsg_seq = m_nlseq++;
  rtm->rtm_dst_len = ip.cidr;
  rtm->rtm_family = ip.family;
  rtm->rtm_type = RTN_UNICAST;
  rtm->rtm_protocol = RTPROT_BOOT;
  rtm->rtm_scope = RT_SCOPE_UNIVERSE;

  // Routes for the main hop should be placed into their own table.
  if (hopindex == 0) {
    rtm->rtm_table = RT_TABLE_UNSPEC;
    nlmsg_append_attr32(buf, sizeof(buf), RTA_TABLE, WG_ROUTE_TABLE);
  } else {
    rtm->rtm_table = RT_TABLE_MAIN;
  }

  if (rtm->rtm_family == AF_INET6) {
    nlmsg_append_attr(buf, sizeof(buf), RTA_DST, &ip.ip6, sizeof(ip.ip6));
  } else {
    nlmsg_append_attr(buf, sizeof(buf), RTA_DST, &ip.ip4, sizeof(ip.ip4));
  }
  nlmsg_append_attr32(buf, sizeof(buf), RTA_OIF, index);

  struct sockaddr_nl nladdr;
  memset(&nladdr, 0, sizeof(nladdr));
  nladdr.nl_family = AF_NETLINK;
  size_t result = sendto(m_nlsock, buf, nlmsg->nlmsg_len, 0,
                         (struct sockaddr*)&nladdr, sizeof(nladdr));
  return (result == nlmsg->nlmsg_len);
}

// PRIVATE METHODS
QStringList WireguardUtilsLinux::currentInterfaces() {
  char* deviceNames = wg_list_device_names();
  QStringList devices;
  if (!deviceNames) {
    return devices;
  }
  char* deviceName;
  size_t len;
  wg_for_each_device_name(deviceNames, deviceName, len) {
    devices.append(deviceName);
  }
  free(deviceNames);
  return devices;
}

bool WireguardUtilsLinux::setPeerEndpoint(struct sockaddr* sa,
                                          const QString& address, int port) {
  QString portString = QString::number(port);

  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_protocol = IPPROTO_UDP;

  struct addrinfo* resolved = nullptr;
  auto guard = qScopeGuard([&] { freeaddrinfo(resolved); });
  int retries = 15;

  for (unsigned int timeout = 1000000;;
       timeout = std::min((unsigned int)20000000, timeout * 6 / 5)) {
    int rv = getaddrinfo(address.toLocal8Bit(), portString.toLocal8Bit(),
                         &hints, &resolved);
    if (!rv) {
      break;
    }

    /* The set of return codes that are "permanent failures". All other
     * possibilities are potentially transient.
     *
     * This is according to https://sourceware.org/glibc/wiki/NameResolver which
     * states: "From the perspective of the application that calls getaddrinfo()
     * it perhaps doesn't matter that much since EAI_FAIL, EAI_NONAME and
     * EAI_NODATA are all permanent failure codes and the causes are all
     * permanent failures in the sense that there is no point in retrying
     * later."
     *
     * So this is what we do, except FreeBSD removed EAI_NODATA some time ago,
     * so that's conditional.
     */
    if (rv == EAI_NONAME || rv == EAI_FAIL ||
#ifdef EAI_NODATA
        rv == EAI_NODATA ||
#endif
        (retries >= 0 && !retries--)) {
      logger.error() << "Failed to resolve the address endpoint";
      return false;
    }

    logger.warning() << "Trying again in" << (timeout / 1000000.0) << "seconds";
    usleep(timeout);
  }

  if ((resolved->ai_family == AF_INET &&
       resolved->ai_addrlen == sizeof(struct sockaddr_in)) ||
      (resolved->ai_family == AF_INET6 &&
       resolved->ai_addrlen == sizeof(struct sockaddr_in6))) {
    memcpy(sa, resolved->ai_addr, resolved->ai_addrlen);
    return true;
  }

  logger.error() << "Invalid endpoint" << address;
  return false;
}

bool WireguardUtilsLinux::addPeerPrefix(wg_peer* peer,
                                        const IPAddressRange& prefix) {
  Q_ASSERT(peer);

  wg_allowedip* allowedip =
      static_cast<wg_allowedip*>(calloc(1, sizeof(*allowedip)));
  if (!allowedip) {
    logger.error() << "Allocation failure";
    return false;
  }

  if (!peer->first_allowedip) {
    peer->first_allowedip = allowedip;
  } else {
    peer->last_allowedip->next_allowedip = allowedip;
  }
  peer->last_allowedip = allowedip;

  return buildAllowedIp(allowedip, prefix);
}

static void nlmsg_append_attr(char* buf, size_t maxlen, int attrtype,
                              const void* attrdata, size_t attrlen) {
  struct nlmsghdr* nlmsg = (struct nlmsghdr*)buf;
  size_t newlen = NLMSG_ALIGN(nlmsg->nlmsg_len) + RTA_SPACE(attrlen);
  if (newlen <= maxlen) {
    struct rtattr* attr = (struct rtattr*)(buf + NLMSG_ALIGN(nlmsg->nlmsg_len));
    attr->rta_type = attrtype;
    attr->rta_len = RTA_LENGTH(attrlen);
    memcpy(RTA_DATA(attr), attrdata, attrlen);
    nlmsg->nlmsg_len = newlen;
  }
}

static void nlmsg_append_attr32(char* buf, size_t maxlen, int attrtype,
                                uint32_t value) {
  nlmsg_append_attr(buf, maxlen, attrtype, &value, sizeof(value));
}

bool WireguardUtilsLinux::rtmSendRule(int action, int flags, int addrfamily) {
  constexpr size_t fib_max_size =
      sizeof(struct fib_rule_hdr) + 2 * RTA_SPACE(sizeof(uint32_t));

  char buf[NLMSG_SPACE(fib_max_size)];
  struct nlmsghdr* nlmsg = (struct nlmsghdr*)buf;
  struct fib_rule_hdr* rule = (struct fib_rule_hdr*)NLMSG_DATA(nlmsg);
  struct sockaddr_nl nladdr;
  memset(&nladdr, 0, sizeof(nladdr));
  nladdr.nl_family = AF_NETLINK;

  /* Create a routing policy rule to select the wireguard routing table for
   * unmarked packets. This is equivalent to:
   *     ip rule add not fwmark $WG_FIREWALL_MARK table $WG_ROUTE_TABLE
   */
  memset(buf, 0, sizeof(buf));
  nlmsg->nlmsg_len = NLMSG_LENGTH(sizeof(struct fib_rule_hdr));
  nlmsg->nlmsg_type = action;
  nlmsg->nlmsg_flags = flags;
  nlmsg->nlmsg_pid = getpid();
  nlmsg->nlmsg_seq = m_nlseq++;
  rule->family = addrfamily;
  rule->table = RT_TABLE_UNSPEC;
  rule->action = FR_ACT_TO_TBL;
  rule->flags = FIB_RULE_INVERT;
  nlmsg_append_attr32(buf, sizeof(buf), FRA_FWMARK, WG_FIREWALL_MARK);
  nlmsg_append_attr32(buf, sizeof(buf), FRA_TABLE, WG_ROUTE_TABLE);
  ssize_t result = sendto(m_nlsock, buf, nlmsg->nlmsg_len, 0,
                          (struct sockaddr*)&nladdr, sizeof(nladdr));
  if (result != nlmsg->nlmsg_len) {
    return false;
  }

  /* Create a routing policy rule to suppress zero-length prefix lookups from
   * in the main routing table. This is equivalent to:
   *     ip rule add table main suppress_prefixlength 0
   */
  memset(buf, 0, sizeof(buf));
  nlmsg->nlmsg_len = NLMSG_LENGTH(sizeof(struct fib_rule_hdr));
  nlmsg->nlmsg_type = action;
  nlmsg->nlmsg_flags = flags;
  nlmsg->nlmsg_pid = getpid();
  nlmsg->nlmsg_seq = m_nlseq++;
  rule->family = addrfamily;
  rule->table = RT_TABLE_MAIN;
  rule->action = FR_ACT_TO_TBL;
  rule->flags = 0;
  nlmsg_append_attr32(buf, sizeof(buf), FRA_SUPPRESS_PREFIXLEN, 0);
  result = sendto(m_nlsock, buf, nlmsg->nlmsg_len, 0, (struct sockaddr*)&nladdr,
                  sizeof(nladdr));
  if (result != nlmsg->nlmsg_len) {
    return false;
  }

  return true;
}

void WireguardUtilsLinux::nlsockReady() {
  char buf[1024];
  ssize_t len = recv(m_nlsock, buf, sizeof(buf), MSG_DONTWAIT);
  if (len <= 0) {
    return;
  }

  struct nlmsghdr* nlmsg = (struct nlmsghdr*)buf;
  while (NLMSG_OK(nlmsg, len)) {
    if (nlmsg->nlmsg_type == NLMSG_DONE) {
      return;
    }
    if (nlmsg->nlmsg_type != NLMSG_ERROR) {
      nlmsg = NLMSG_NEXT(nlmsg, len);
      continue;
    }
    struct nlmsgerr* err = (struct nlmsgerr*)NLMSG_DATA(nlmsg);
    if (err->error != 0) {
      logger.debug() << "Netlink request failed:" << strerror(-err->error);
    }
    nlmsg = NLMSG_NEXT(nlmsg, len);
  }
}

// static
bool WireguardUtilsLinux::setupCgroupClass(const QString& path,
                                           unsigned long classid) {
  logger.debug() << "Creating control group:" << path;
  int flags = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
  int err = mkdir(qPrintable(path), flags);
  if ((err < 0) && (errno != EEXIST)) {
    logger.error() << "Failed to create" << path + ":" << strerror(errno);
    return false;
  }

  QString netClassPath = path + "/net_cls.classid";
  FILE* fp = fopen(qPrintable(netClassPath), "w");
  if (!fp) {
    logger.error() << "Failed to set classid:" << strerror(errno);
    return false;
  }
  fprintf(fp, "%lu", classid);
  fclose(fp);
  return true;
}

QString WireguardUtilsLinux::getExcludeCgroup() const {
  if (m_cgroups.isNull()) {
    return QString();
  }
  return m_cgroups + VPN_EXCLUDE_CGROUP;
}

QString WireguardUtilsLinux::getBlockCgroup() const {
  if (m_cgroups.isNull()) {
    return QString();
  }
  return m_cgroups + VPN_BLOCK_CGROUP;
}

// static
bool WireguardUtilsLinux::buildAllowedIp(wg_allowedip* ip,
                                         const IPAddressRange& prefix) {
  if (prefix.type() == IPAddressRange::IPv4) {
    ip->family = AF_INET;
    ip->cidr = prefix.range();
    return inet_pton(AF_INET, qPrintable(prefix.ipAddress()), &ip->ip4) == 1;
  }
  if (prefix.type() == IPAddressRange::IPv6) {
    ip->family = AF_INET6;
    ip->cidr = prefix.range();
    return inet_pton(AF_INET6, qPrintable(prefix.ipAddress()), &ip->ip6) == 1;
  }
  return false;
}

// static
QString WireguardUtilsLinux::printablePubkey(const QString& pubkey) {
  if (pubkey.length() < 12) {
    return pubkey;
  } else {
    return pubkey.left(6) + "..." + pubkey.right(6);
  }
}

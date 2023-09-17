/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "pidtracker.h"

#include <errno.h>
#include <limits.h>
#include <linux/cn_proc.h>
#include <linux/connector.h>
#include <linux/netlink.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include "leakdetector.h"
#include "logger.h"

constexpr size_t CN_MCAST_MSG_SIZE =
    sizeof(struct cn_msg) + sizeof(enum proc_cn_mcast_op);

namespace {
Logger logger("PidTracker");
}

PidTracker::PidTracker(QObject* parent) : QObject(parent) {
  MZ_COUNT_CTOR(PidTracker);
  logger.debug() << "PidTracker created.";

  m_nlsock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_CONNECTOR);
  if (m_nlsock < 0) {
    logger.error() << "Failed to create netlink socket:" << strerror(errno);
    return;
  }

  struct sockaddr_nl nladdr;
  nladdr.nl_family = AF_NETLINK;
  nladdr.nl_groups = CN_IDX_PROC;
  nladdr.nl_pid = getpid();
  nladdr.nl_pad = 0;
  if (bind(m_nlsock, (struct sockaddr*)&nladdr, sizeof(nladdr)) < 0) {
    logger.error() << "Failed to bind netlink socket:" << strerror(errno);
    close(m_nlsock);
    m_nlsock = -1;
    return;
  }

  char buf[NLMSG_SPACE(CN_MCAST_MSG_SIZE)];
  struct nlmsghdr* nlmsg = (struct nlmsghdr*)buf;
  struct cn_msg* cnmsg = (struct cn_msg*)NLMSG_DATA(nlmsg);
  enum proc_cn_mcast_op mcast_op = PROC_CN_MCAST_LISTEN;

  memset(buf, 0, sizeof(buf));
  nlmsg->nlmsg_len = NLMSG_LENGTH(CN_MCAST_MSG_SIZE);
  nlmsg->nlmsg_type = NLMSG_DONE;
  nlmsg->nlmsg_flags = 0;
  nlmsg->nlmsg_seq = 0;
  nlmsg->nlmsg_pid = getpid();

  cnmsg->id.idx = CN_IDX_PROC;
  cnmsg->id.val = CN_VAL_PROC;
  cnmsg->seq = 0;
  cnmsg->ack = 0;
  cnmsg->len = sizeof(mcast_op);
  memcpy(cnmsg->data, &mcast_op, sizeof(mcast_op));

  if (send(m_nlsock, nlmsg, sizeof(buf), 0) != sizeof(buf)) {
    logger.error() << "Failed to send netlink message:" << strerror(errno);
    close(m_nlsock);
    m_nlsock = -1;
    return;
  }

  m_socket = new QSocketNotifier(m_nlsock, QSocketNotifier::Read, this);
  connect(m_socket, &QSocketNotifier::activated, this, &PidTracker::readData);
}

PidTracker::~PidTracker() {
  MZ_COUNT_DTOR(PidTracker);
  logger.debug() << "PidTracker destroyed.";

  m_processTree.clear();
  while (!m_processGroups.isEmpty()) {
    ProcessGroup* group = m_processGroups.takeFirst();
    delete group;
  }

  if (m_nlsock > 0) {
    close(m_nlsock);
  }
}

ProcessGroup* PidTracker::track(const QString& name, int rootpid) {
  ProcessGroup* group = m_processTree.value(rootpid, nullptr);
  if (group) {
    logger.warning() << "Ignoring attempt to track duplicate PID";
    return group;
  }
  group = new ProcessGroup(name, rootpid);
  group->kthreads[rootpid] = 1;
  group->refcount = 1;

  m_processGroups.append(group);
  m_processTree[rootpid] = group;

  return group;
}

void PidTracker::handleProcEvent(struct cn_msg* cnmsg) {
  struct proc_event* ev = (struct proc_event*)cnmsg->data;

  if (ev->what == proc_event::PROC_EVENT_FORK) {
    auto forkdata = &ev->event_data.fork;
    /* If the child process already exists, track a new kernel thread. */
    ProcessGroup* group = m_processTree.value(forkdata->child_tgid, nullptr);
    if (group) {
      group->kthreads[forkdata->child_tgid]++;
      return;
    }

    /* Track a new userspace process if was forked from a known parent. */
    group = m_processTree.value(forkdata->parent_tgid, nullptr);
    if (!group) {
      return;
    }
    m_processTree[forkdata->child_tgid] = group;
    group->kthreads[forkdata->child_tgid] = 1;
    group->refcount++;
    emit pidForked(group->name, forkdata->parent_tgid, forkdata->child_tgid);
  }

  if (ev->what == proc_event::PROC_EVENT_EXIT) {
    auto exitdata = &ev->event_data.exit;
    ProcessGroup* group = m_processTree.value(exitdata->process_tgid, nullptr);
    if (!group) {
      return;
    }

    /* Decrement the number of kernel threads in this userspace process. */
    uint threadcount = group->kthreads.value(exitdata->process_tgid, 0);
    if (threadcount == 0) {
      return;
    }
    if (threadcount > 1) {
      group->kthreads[exitdata->process_tgid] = threadcount - 1;
      return;
    }
    group->kthreads.remove(exitdata->process_tgid);

    /* A userspace process exits when all of its kernel threads exit. */
    Q_ASSERT(group->refcount > 0);
    group->refcount--;
    if (group->refcount == 0) {
      emit terminated(group->name, group->rootpid);
      m_processGroups.removeAll(group);
      delete group;
    }
  }
}

void PidTracker::readData() {
  struct sockaddr_nl src;
  socklen_t srclen = sizeof(src);
  ssize_t recvlen;

  recvlen = recvfrom(m_nlsock, m_readBuf, sizeof(m_readBuf), MSG_DONTWAIT,
                     (struct sockaddr*)&src, &srclen);
  if (recvlen == ENOBUFS) {
    logger.error()
        << "Failed to read netlink socket: buffer full, message dropped";
    return;
  }
  if (recvlen < 0) {
    logger.error() << "Failed to read netlink socket:" << strerror(errno);
    return;
  }
  if (srclen != sizeof(src)) {
    logger.error() << "Failed to read netlink socket: invalid address length";
    return;
  }

  /* We are only interested in process-control messages from the kernel */
  if ((src.nl_groups != CN_IDX_PROC) || (src.nl_pid != 0)) {
    return;
  }

  /* Handle the process-control messages. */
  struct nlmsghdr* msg;
  for (msg = (struct nlmsghdr*)m_readBuf; NLMSG_OK(msg, recvlen);
       msg = NLMSG_NEXT(msg, recvlen)) {
    struct cn_msg* cnmsg = (struct cn_msg*)NLMSG_DATA(msg);
    if (msg->nlmsg_type == NLMSG_NOOP) {
      continue;
    }
    if ((msg->nlmsg_type == NLMSG_ERROR) ||
        (msg->nlmsg_type == NLMSG_OVERRUN)) {
      break;
    }
    handleProcEvent(cnmsg);
    if (msg->nlmsg_type == NLMSG_DONE) {
      break;
    }
  }
}

bool ProcessGroup::moveToCgroup(const QString& name) {
  /* Do nothing if Cgroups are not supported. */
  if (name.isNull()) {
    return true;
  }

  QString cgProcsFile = name + "/cgroup.procs";
  FILE* fp = fopen(qPrintable(cgProcsFile), "w");
  if (!fp) {
    return false;
  }

  for (auto iterator = kthreads.constBegin(); iterator != kthreads.constEnd();
       ++iterator) {
    fprintf(fp, "%d\n", iterator.key());
    fflush(fp);
  }
  fclose(fp);
  return true;
}

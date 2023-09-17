/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef PIDTRACKER_H
#define PIDTRACKER_H

#include <QHash>
#include <QList>
#include <QSocketNotifier>
#include <QString>

#include "leakdetector.h"

struct cn_msg;

class ProcessGroup {
 public:
  ProcessGroup(const QString& groupName, int groupRootPid,
               const QString& groupState = "active") {
    MZ_COUNT_CTOR(ProcessGroup);
    name = groupName;
    rootpid = groupRootPid;
    state = groupState;
    refcount = 0;
  }
  ~ProcessGroup() { MZ_COUNT_DTOR(ProcessGroup); }

  bool moveToCgroup(const QString& name);

  QHash<int, uint> kthreads;
  QString name;
  QString state;
  int rootpid;
  int refcount;
};

class PidTracker final : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(PidTracker)

 public:
  explicit PidTracker(QObject* parent);
  ~PidTracker();

  ProcessGroup* track(const QString& name, int rootpid);

  QList<int> pids() { return m_processTree.keys(); }
  QList<ProcessGroup*>::iterator begin() { return m_processGroups.begin(); }
  QList<ProcessGroup*>::iterator end() { return m_processGroups.end(); }
  ProcessGroup* group(int pid) { return m_processTree.value(pid); }

 signals:
  void pidForked(const QString& name, int parent, int child);
  void pidExited(const QString& name, int pid);
  void terminated(const QString& name, int rootpid);

 private:
  void handleProcEvent(struct cn_msg*);

 private slots:
  void readData();

 private:
  int m_nlsock;
  char m_readBuf[2048];
  QSocketNotifier* m_socket = nullptr;
  QHash<int, ProcessGroup*> m_processTree;
  QList<ProcessGroup*> m_processGroups;
};

#endif  // PIDTRACKER_H

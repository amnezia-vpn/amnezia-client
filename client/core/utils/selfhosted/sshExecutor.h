#ifndef SSHEXECUTOR_H
#define SSHEXECUTOR_H

#include <QHash>
#include <QMutex>
#include <QString>
#include <QThreadPool>
#include <QtConcurrent>

#include <utility>

// Per-server serial executor for long-running self-hosted SSH operations.
//
// All SSH work for a given serverId is queued onto a dedicated single-thread
// pool, so operations to the same server run strictly one at a time (no
// concurrent SSH sessions to one host => no races, and a natural guard against
// double-run). Operations to different servers still run in parallel.
//
// NOTE: do NOT route nested workers that are awaited from within an already
// queued operation (e.g. InstallController::isServerDpkgBusy) through the same
// per-server pool — the single worker thread would block waiting on itself.
// Such nested helpers must keep using the global QThreadPool.
class SshExecutor
{
public:
    static SshExecutor &instance();

    SshExecutor(const SshExecutor &) = delete;
    SshExecutor &operator=(const SshExecutor &) = delete;

    template <typename Functor>
    auto run(const QString &serverId, Functor &&functor)
    {
        return QtConcurrent::run(poolFor(serverId), std::forward<Functor>(functor));
    }

private:
    SshExecutor() = default;
    ~SshExecutor();

    QThreadPool *poolFor(const QString &serverId);

    QHash<QString, QThreadPool *> m_pools;
    QMutex m_mutex;
};

#endif // SSHEXECUTOR_H

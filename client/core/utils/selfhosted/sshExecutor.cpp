#include "sshExecutor.h"

SshExecutor &SshExecutor::instance()
{
    static SshExecutor executor;
    return executor;
}

SshExecutor::~SshExecutor()
{
    QMutexLocker lock(&m_mutex);
    for (QThreadPool *pool : std::as_const(m_pools)) {
        pool->waitForDone();
        delete pool;
    }
    m_pools.clear();
}

QThreadPool *SshExecutor::poolFor(const QString &serverId)
{
    QMutexLocker lock(&m_mutex);

    auto it = m_pools.find(serverId);
    if (it != m_pools.end()) {
        return it.value();
    }

    auto *pool = new QThreadPool();
    pool->setMaxThreadCount(1);   // serialize all SSH ops for this server
    pool->setExpiryTimeout(-1);   // keep the single worker thread alive between ops
    m_pools.insert(serverId, pool);
    return pool;
}

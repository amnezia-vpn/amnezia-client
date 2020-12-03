#ifndef RUNGUARD_H
#define RUNGUARD_H

#include <QObject>
#include <QSharedMemory>
#include <QSystemSemaphore>
#include <QDebug>

/**
 * @brief The RunGuard class - The application single instance (via shared memory)
 */
class RunGuard
{

public:
    static RunGuard &instance(const QString& key = QString());

    ~RunGuard();

    void activate();
    bool isAnotherRunning() const;
    bool tryToRun();
    void release();

private:
    RunGuard(const QString& key);
    Q_DISABLE_COPY( RunGuard )

    const QString key;
    const QString memLockKey;
    const QString sharedmemKey;

    mutable QSharedMemory sharedMem;
    mutable QSystemSemaphore memLock;

};
#endif // RUNGUARD_H

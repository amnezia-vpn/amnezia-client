#ifndef ANDROID_UTILS_H
#define ANDROID_UTILS_H

#include <QEventLoop>
#include <QFutureWatcher>
#include <QJniObject>
#include <QtConcurrent>

namespace AndroidUtils
{
QJniObject getActivity();

QString convertJString(JNIEnv *env, jstring data);

void runOnAndroidThreadSync(const std::function<void()> &runnable);
void runOnAndroidThreadAsync(const std::function<void()> &runnable);

template <typename Func>
auto runOnWorkerThread(Func &&func)
{
    using Result = std::invoke_result_t<std::decay_t<Func>>;
    QFutureWatcher<Result> watcher;
    QEventLoop loop;
    QObject::connect(&watcher, &QFutureWatcher<Result>::finished, &loop, &QEventLoop::quit);
    watcher.setFuture(QtConcurrent::run(std::forward<Func>(func)));
    loop.exec();
    return watcher.result();
}
};

#endif // ANDROID_UTILS_H

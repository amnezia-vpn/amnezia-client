#ifndef LOGSUICONTROLLER_H
#define LOGSUICONTROLLER_H

#include <QObject>
#include <QTimer>

#include <functional>

class LogsUiController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString appLogSize READ appLogSize NOTIFY sizesChanged)
    Q_PROPERTY(QString tunnelLogSize READ tunnelLogSize NOTIFY sizesChanged)
    Q_PROPERTY(QString viewerStream READ viewerStream WRITE setViewerStream NOTIFY viewerStreamChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(bool canShare READ canShare CONSTANT)

public:
    explicit LogsUiController(QObject *parent = nullptr);

    QString appLogSize() const;
    QString tunnelLogSize() const;

    QString viewerStream() const;
    void setViewerStream(const QString &stream);

    bool busy() const;
    bool canShare() const;

    Q_INVOKABLE void refreshSizes();

    Q_INVOKABLE void load(const QString &stream);

    Q_INVOKABLE void startLiveTail();
    Q_INVOKABLE void stopLiveTail();

    Q_INVOKABLE void copyLoaded();

    Q_INVOKABLE void exportStream(const QString &stream, const QString &fileName);
    Q_INVOKABLE void saveAll(const QString &fileName);
    Q_INVOKABLE void shareAll();
    Q_INVOKABLE void shareStream(const QString &stream);

    Q_INVOKABLE QString defaultFileName(const QString &kind) const;

signals:
    void sizesChanged();
    void viewerStreamChanged();
    void busyChanged();

    void streamLoaded(const QString &stream, const QString &text);
    void appended(const QString &text);
    void reloadRequired();

    void exportFinished(bool success);

private:
    struct LoadResult
    {
        QString text;
        QString tailFile;
        qint64 tailSize = 0;
    };

    void onLiveTailTimeout();
    void onApplicationStateChanged(Qt::ApplicationState state);

    void runExport(const std::function<QByteArray()> &build, const std::function<bool(const QByteArray &)> &deliver);
    void setBusy(bool busy);

    QString m_viewerStream = QStringLiteral("app");
    QString m_lastText;
    quint64 m_loadId = 0;
    bool m_busy = false;

    QTimer m_liveTailTimer;
    bool m_liveTailEnabled = false;
    QString m_tailFile;
    qint64 m_tailSize = 0;
};

#endif // LOGSUICONTROLLER_H

#ifndef SYSTEMCONTROLLER_H
#define SYSTEMCONTROLLER_H

#include <QByteArray>
#include <QObject>

class SystemController : public QObject
{
    Q_OBJECT
public:
    explicit SystemController(QObject *parent = nullptr);
    ~SystemController() override;

    enum class SaveFileResult {
        Saved,
        Cancelled, // the user dismissed the save/share dialog without picking a destination
        Unknown,   // a destination was picked but it did not report success (some iOS share extensions do this)
        Failed
    };

    static SaveFileResult saveFileEx(const QString &fileName, const QString &data);
    static SaveFileResult saveFileEx(const QString &fileName, const QByteArray &data);

    static bool saveFile(const QString &fileName, const QString &data);
    static bool saveFile(const QString &fileName, const QByteArray &data);
    static bool readFile(const QString &fileName, QByteArray &data);
    static bool readFile(const QString &fileName, QString &data);

public slots:
    QString getFileName(const QString &acceptLabel, const QString &nameFilter, const QString &selectedFile = "",
                        const bool isSaveMode = false, const QString &defaultSuffix = "");

    void setQmlRoot(QObject *qmlRoot);

    bool isAuthenticated();
    void sendTouch(float x, float y);

signals:
    void fileDialogClosed(const bool isAccepted);
    // emitted whenever the user dismisses a save/share dialog without choosing a destination
    void saveCancelledByUser();

private:
    static void notifySaveCancelled();

    static SystemController *s_instance;

    QObject *m_qmlRoot;
};

#endif // SYSTEMCONTROLLER_H

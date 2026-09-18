#ifndef SYSTEMCONTROLLER_H
#define SYSTEMCONTROLLER_H

#include <QByteArray>
#include <QObject>

class SystemController : public QObject
{
    Q_OBJECT
public:
    explicit SystemController(QObject *parent = nullptr);

    static bool saveFile(const QString &fileName, const QString &data);
    static bool saveFile(const QString &fileName, const QByteArray &data);
    static bool readFile(const QString &fileName, QByteArray &data);
    static bool readFile(const QString &fileName, QString &data);

    static bool encryptFile(const QString &filePath, const QString &password, const QString &hint);

    Q_INVOKABLE bool QEncryptFile(const QString &filePath, const QString &password, const QString &hint)
    {
        return encryptFile(filePath, password, hint);
    }

public slots:
    QString getFileName(const QString &acceptLabel, const QString &nameFilter, const QString &selectedFile = "",
                        const bool isSaveMode = false, const QString &defaultSuffix = "");

    QByteArray getDecryptedData(const QString &filePath, const QString &password);

    bool isFileEncrypted(const QString &filePath);
    bool isPasswordValid(const QString &filePath, const QString &password);
    QString readHint(const QString &filePath);

    void setQmlRoot(QObject *qmlRoot);

    bool isAuthenticated();
    void sendTouch(float x, float y);

signals:
    void fileDialogClosed(const bool isAccepted);

private:
    QObject *m_qmlRoot;
};

#endif // SYSTEMCONTROLLER_H

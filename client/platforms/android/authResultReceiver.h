#ifndef AUTHRESULTRECEIVER_H
#define AUTHRESULTRECEIVER_H

#include <QJniObject>

#include <private/qandroidextras_p.h>

class AuthResultNotifier : public QObject
{
    Q_OBJECT

public:
    AuthResultNotifier(QObject *parent = nullptr) : QObject(parent) {};

signals:
    void authFailed();
    void authSuccessful();
};

/* Auth result handler for Android */
class AuthResultReceiver final : public QAndroidActivityResultReceiver
{
public:
    AuthResultReceiver(QSharedPointer<AuthResultNotifier> &notifier);

    void handleActivityResult(int receiverRequestCode, int resultCode, const QJniObject &data) override;

private:
    QSharedPointer<AuthResultNotifier> m_notifier;
};

#endif // AUTHRESULTRECEIVER_H

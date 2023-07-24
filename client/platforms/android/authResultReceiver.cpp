#include "authResultReceiver.h"

AuthResultReceiver::AuthResultReceiver(QSharedPointer<AuthResultNotifier> &notifier) : m_notifier(notifier)
{
}

void AuthResultReceiver::handleActivityResult(int receiverRequestCode, int resultCode, const QJniObject &data)
{
    qDebug() << "receiverRequestCode" << receiverRequestCode << "resultCode" << resultCode;

    if (resultCode == -1) { // ResultOK
        emit m_notifier->authSuccessful();
    } else {
        emit m_notifier->authFailed();
    }
}

#ifndef SERVER_SETTINGS_LOGIC_H
#define SERVER_SETTINGS_LOGIC_H

#include "PageLogicBase.h"

#if defined(Q_OS_ANDROID)
#include <QJniObject>
#include <private/qandroidextras_p.h>
#endif

class UiLogic;

class ServerSettingsLogic : public PageLogicBase
{
    Q_OBJECT

    AUTO_PROPERTY(bool, labelWaitInfoVisible)
    AUTO_PROPERTY(QString, labelWaitInfoText)
    AUTO_PROPERTY(QString, pushButtonClearClientCacheText)
    AUTO_PROPERTY(bool, pushButtonClearClientCacheVisible)
    AUTO_PROPERTY(bool, pushButtonShareFullVisible)
    AUTO_PROPERTY(QString, labelServerText)
    AUTO_PROPERTY(QString, lineEditDescriptionText)
    AUTO_PROPERTY(QString, labelCurrentVpnProtocolText)

public:
    Q_INVOKABLE void onUpdatePage() override;

    Q_INVOKABLE void onPushButtonForgetServer();
    Q_INVOKABLE void onPushButtonShareFullClicked();
    Q_INVOKABLE void onPushButtonClearClientCacheClicked();
    Q_INVOKABLE void onLineEditDescriptionEditingFinished();

    Q_INVOKABLE bool isCurrentServerHasCredentials();

public:
    explicit ServerSettingsLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~ServerSettingsLogic() = default;

};

#if defined(Q_OS_ANDROID)
/* Auth result handler for Android */
class authResultReceiver  final : public PageLogicBase, public QAndroidActivityResultReceiver
{
Q_OBJECT

public:
    authResultReceiver(UiLogic *uiLogic, int serverIndex , QObject *parent = nullptr) : PageLogicBase(uiLogic, parent) {
        m_serverIndex = serverIndex;
    }
    ~authResultReceiver() {}

public:
    void handleActivityResult(int receiverRequestCode, int resultCode, const QJniObject &data) override;

private:
    int  m_serverIndex;
};
#endif

#endif // SERVER_SETTINGS_LOGIC_H

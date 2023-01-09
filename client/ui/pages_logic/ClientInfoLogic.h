#ifndef CLIENTINFOLOGIC_H
#define CLIENTINFOLOGIC_H

#include "PageLogicBase.h"

class UiLogic;

class ClientInfoLogic : public PageLogicBase
{
    Q_OBJECT

    AUTO_PROPERTY(QString, lineEditNameAliasText)
    AUTO_PROPERTY(QString, labelCertId)
    AUTO_PROPERTY(QString, textAreaCertificate)
    AUTO_PROPERTY(QString, labelCurrentVpnProtocolText)

public:
    ClientInfoLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~ClientInfoLogic() = default;

    void setCurrentClientId(int index);

public slots:
    void onUpdatePage() override;
    void onLineEditNameAliasEditingFinished();
    void onRevokeCertificateClicked();

private:
    int m_currentClientIndex;
};

#endif // CLIENTINFOLOGIC_H

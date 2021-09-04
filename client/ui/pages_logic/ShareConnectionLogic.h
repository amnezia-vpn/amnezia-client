#ifndef SHARE_CONNECTION_LOGIC_H
#define SHARE_CONNECTION_LOGIC_H

#include "../pages.h"
#include "settings.h"
#include "3rd/QRCodeGenerator/QRCodeGenerator.h"

class UiLogic;

class ShareConnectionLogic: public QObject
{
    Q_OBJECT

public:
    Q_PROPERTY(bool pageShareAmneziaVisible READ getPageShareAmneziaVisible WRITE setPageShareAmneziaVisible NOTIFY pageShareAmneziaVisibleChanged)
    Q_PROPERTY(bool pageShareOpenvpnVisible READ getPageShareOpenvpnVisible WRITE setPageShareOpenvpnVisible NOTIFY pageShareOpenvpnVisibleChanged)
    Q_PROPERTY(bool pageShareShadowsocksVisible READ getPageShareShadowsocksVisible WRITE setPageShareShadowsocksVisible NOTIFY pageShareShadowsocksVisibleChanged)
    Q_PROPERTY(bool pageShareCloakVisible READ getPageShareCloakVisible WRITE setPageShareCloakVisible NOTIFY pageShareCloakVisibleChanged)
    Q_PROPERTY(bool pageShareFullAccessVisible READ getPageShareFullAccessVisible WRITE setPageShareFullAccessVisible NOTIFY pageShareFullAccessVisibleChanged)
    Q_PROPERTY(QString textEditShareOpenvpnCodeText READ getTextEditShareOpenvpnCodeText WRITE setTextEditShareOpenvpnCodeText NOTIFY textEditShareOpenvpnCodeTextChanged)
    Q_PROPERTY(bool pushButtonShareOpenvpnCopyEnabled READ getPushButtonShareOpenvpnCopyEnabled WRITE setPushButtonShareOpenvpnCopyEnabled NOTIFY pushButtonShareOpenvpnCopyEnabledChanged)
    Q_PROPERTY(bool pushButtonShareOpenvpnSaveEnabled READ getPushButtonShareOpenvpnSaveEnabled WRITE setPushButtonShareOpenvpnSaveEnabled NOTIFY pushButtonShareOpenvpnSaveEnabledChanged)
    Q_PROPERTY(int toolBoxShareConnectionCurrentIndex READ getToolBoxShareConnectionCurrentIndex WRITE setToolBoxShareConnectionCurrentIndex NOTIFY toolBoxShareConnectionCurrentIndexChanged)
    Q_PROPERTY(bool pushButtonShareSsCopyEnabled READ getPushButtonShareSsCopyEnabled WRITE setPushButtonShareSsCopyEnabled NOTIFY pushButtonShareSsCopyEnabledChanged)
    Q_PROPERTY(QString lineEditShareSsStringText READ getLineEditShareSsStringText WRITE setLineEditShareSsStringText NOTIFY lineEditShareSsStringTextChanged)
    Q_PROPERTY(QString labelShareSsQrCodeText READ getLabelShareSsQrCodeText WRITE setLabelShareSsQrCodeText NOTIFY labelShareSsQrCodeTextChanged)
    Q_PROPERTY(QString labelShareSsServerText READ getLabelShareSsServerText WRITE setLabelShareSsServerText NOTIFY labelShareSsServerTextChanged)
    Q_PROPERTY(QString labelShareSsPortText READ getLabelShareSsPortText WRITE setLabelShareSsPortText NOTIFY labelShareSsPortTextChanged)
    Q_PROPERTY(QString labelShareSsMethodText READ getLabelShareSsMethodText WRITE setLabelShareSsMethodText NOTIFY labelShareSsMethodTextChanged)
    Q_PROPERTY(QString labelShareSsPasswordText READ getLabelShareSsPasswordText WRITE setLabelShareSsPasswordText NOTIFY labelShareSsPasswordTextChanged)
    Q_PROPERTY(QString plainTextEditShareCloakText READ getPlainTextEditShareCloakText WRITE setPlainTextEditShareCloakText NOTIFY plainTextEditShareCloakTextChanged)
    Q_PROPERTY(bool pushButtonShareCloakCopyEnabled READ getPushButtonShareCloakCopyEnabled WRITE setPushButtonShareCloakCopyEnabled NOTIFY pushButtonShareCloakCopyEnabledChanged)
    Q_PROPERTY(QString textEditShareFullCodeText READ getTextEditShareFullCodeText WRITE setTextEditShareFullCodeText NOTIFY textEditShareFullCodeTextChanged)
    Q_PROPERTY(QString textEditShareAmneziaCodeText READ getTextEditShareAmneziaCodeText WRITE setTextEditShareAmneziaCodeText NOTIFY textEditShareAmneziaCodeTextChanged)
    Q_PROPERTY(QString pushButtonShareFullCopyText READ getPushButtonShareFullCopyText WRITE setPushButtonShareFullCopyText NOTIFY pushButtonShareFullCopyTextChanged)
    Q_PROPERTY(QString pushButtonShareAmneziaCopyText READ getPushButtonShareAmneziaCopyText WRITE setPushButtonShareAmneziaCopyText NOTIFY pushButtonShareAmneziaCopyTextChanged)
    Q_PROPERTY(QString pushButtonShareOpenvpnCopyText READ getPushButtonShareOpenvpnCopyText WRITE setPushButtonShareOpenvpnCopyText NOTIFY pushButtonShareOpenvpnCopyTextChanged)
    Q_PROPERTY(QString pushButtonShareSsCopyText READ getPushButtonShareSsCopyText WRITE setPushButtonShareSsCopyText NOTIFY pushButtonShareSsCopyTextChanged)
    Q_PROPERTY(QString pushButtonShareCloakCopyText READ getPushButtonShareCloakCopyText WRITE setPushButtonShareCloakCopyText NOTIFY pushButtonShareCloakCopyTextChanged)
    Q_PROPERTY(bool pushButtonShareAmneziaGenerateEnabled READ getPushButtonShareAmneziaGenerateEnabled WRITE setPushButtonShareAmneziaGenerateEnabled NOTIFY pushButtonShareAmneziaGenerateEnabledChanged)
    Q_PROPERTY(bool pushButtonShareAmneziaCopyEnabled READ getPushButtonShareAmneziaCopyEnabled WRITE setPushButtonShareAmneziaCopyEnabled NOTIFY pushButtonShareAmneziaCopyEnabledChanged)
    Q_PROPERTY(QString pushButtonShareAmneziaGenerateText READ getPushButtonShareAmneziaGenerateText WRITE setPushButtonShareAmneziaGenerateText NOTIFY pushButtonShareAmneziaGenerateTextChanged)
    Q_PROPERTY(bool pushButtonShareOpenvpnGenerateEnabled READ getPushButtonShareOpenvpnGenerateEnabled WRITE setPushButtonShareOpenvpnGenerateEnabled NOTIFY pushButtonShareOpenvpnGenerateEnabledChanged)
    Q_PROPERTY(QString pushButtonShareOpenvpnGenerateText READ getPushButtonShareOpenvpnGenerateText WRITE setPushButtonShareOpenvpnGenerateText NOTIFY pushButtonShareOpenvpnGenerateTextChanged)

public:
    explicit ShareConnectionLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~ShareConnectionLogic() = default;

    void updateSharingPage(int serverIndex, const ServerCredentials &credentials,
                           DockerContainer container);
    void updateQRCodeImage(const QString &text, const std::function<void(const QString&)>& setLabelFunc);

    bool getPageShareAmneziaVisible() const;
    void setPageShareAmneziaVisible(bool pageShareAmneziaVisible);
    bool getPageShareOpenvpnVisible() const;
    void setPageShareOpenvpnVisible(bool pageShareOpenvpnVisible);
    bool getPageShareShadowsocksVisible() const;
    void setPageShareShadowsocksVisible(bool pageShareShadowsocksVisible);
    bool getPageShareCloakVisible() const;
    void setPageShareCloakVisible(bool pageShareCloakVisible);
    bool getPageShareFullAccessVisible() const;
    void setPageShareFullAccessVisible(bool pageShareFullAccessVisible);
    QString getTextEditShareOpenvpnCodeText() const;
    void setTextEditShareOpenvpnCodeText(const QString &textEditShareOpenvpnCodeText);
    bool getPushButtonShareOpenvpnCopyEnabled() const;
    void setPushButtonShareOpenvpnCopyEnabled(bool pushButtonShareOpenvpnCopyEnabled);
    bool getPushButtonShareOpenvpnSaveEnabled() const;
    void setPushButtonShareOpenvpnSaveEnabled(bool pushButtonShareOpenvpnSaveEnabled);
    int getToolBoxShareConnectionCurrentIndex() const;
    void setToolBoxShareConnectionCurrentIndex(int toolBoxShareConnectionCurrentIndex);
    bool getPushButtonShareSsCopyEnabled() const;
    void setPushButtonShareSsCopyEnabled(bool pushButtonShareSsCopyEnabled);
    QString getLineEditShareSsStringText() const;
    void setLineEditShareSsStringText(const QString &lineEditShareSsStringText);
    QString getLabelShareSsQrCodeText() const;
    void setLabelShareSsQrCodeText(const QString &labelShareSsQrCodeText);
    QString getLabelShareSsServerText() const;
    void setLabelShareSsServerText(const QString &labelShareSsServerText);
    QString getLabelShareSsPortText() const;
    void setLabelShareSsPortText(const QString &labelShareSsPortText);
    QString getLabelShareSsMethodText() const;
    void setLabelShareSsMethodText(const QString &labelShareSsMethodText);
    QString getLabelShareSsPasswordText() const;
    void setLabelShareSsPasswordText(const QString &labelShareSsPasswordText);
    QString getPlainTextEditShareCloakText() const;
    void setPlainTextEditShareCloakText(const QString &plainTextEditShareCloakText);
    bool getPushButtonShareCloakCopyEnabled() const;
    void setPushButtonShareCloakCopyEnabled(bool pushButtonShareCloakCopyEnabled);
    QString getTextEditShareFullCodeText() const;
    void setTextEditShareFullCodeText(const QString &textEditShareFullCodeText);
    QString getTextEditShareAmneziaCodeText() const;
    void setTextEditShareAmneziaCodeText(const QString &textEditShareAmneziaCodeText);
    QString getPushButtonShareFullCopyText() const;
    void setPushButtonShareFullCopyText(const QString &pushButtonShareFullCopyText);
    QString getPushButtonShareAmneziaCopyText() const;
    void setPushButtonShareAmneziaCopyText(const QString &pushButtonShareAmneziaCopyText);
    QString getPushButtonShareOpenvpnCopyText() const;
    void setPushButtonShareOpenvpnCopyText(const QString &pushButtonShareOpenvpnCopyText);
    QString getPushButtonShareSsCopyText() const;
    void setPushButtonShareSsCopyText(const QString &pushButtonShareSsCopyText);
    QString getPushButtonShareCloakCopyText() const;
    void setPushButtonShareCloakCopyText(const QString &pushButtonShareCloakCopyText);
    bool getPushButtonShareAmneziaGenerateEnabled() const;
    void setPushButtonShareAmneziaGenerateEnabled(bool pushButtonShareAmneziaGenerateEnabled);
    bool getPushButtonShareAmneziaCopyEnabled() const;
    void setPushButtonShareAmneziaCopyEnabled(bool pushButtonShareAmneziaCopyEnabled);
    QString getPushButtonShareAmneziaGenerateText() const;
    void setPushButtonShareAmneziaGenerateText(const QString &pushButtonShareAmneziaGenerateText);
    bool getPushButtonShareOpenvpnGenerateEnabled() const;
    void setPushButtonShareOpenvpnGenerateEnabled(bool pushButtonShareOpenvpnGenerateEnabled);
    QString getPushButtonShareOpenvpnGenerateText() const;
    void setPushButtonShareOpenvpnGenerateText(const QString &pushButtonShareOpenvpnGenerateText);


    Q_INVOKABLE void onPushButtonShareFullCopyClicked();
    Q_INVOKABLE void onPushButtonShareFullSaveClicked();
    Q_INVOKABLE void onPushButtonShareAmneziaCopyClicked();
    Q_INVOKABLE void onPushButtonShareAmneziaSaveClicked();
    Q_INVOKABLE void onPushButtonShareOpenvpnCopyClicked();
    Q_INVOKABLE void onPushButtonShareSsCopyClicked();
    Q_INVOKABLE void onPushButtonShareCloakCopyClicked();
    Q_INVOKABLE void onPushButtonShareAmneziaGenerateClicked();
    Q_INVOKABLE void onPushButtonShareOpenvpnGenerateClicked();
    Q_INVOKABLE void onPushButtonShareOpenvpnSaveClicked();

signals:
    void pageShareAmneziaVisibleChanged();
    void pageShareOpenvpnVisibleChanged();
    void pageShareShadowsocksVisibleChanged();
    void pageShareCloakVisibleChanged();
    void pageShareFullAccessVisibleChanged();
    void textEditShareOpenvpnCodeTextChanged();
    void pushButtonShareOpenvpnCopyEnabledChanged();
    void pushButtonShareOpenvpnSaveEnabledChanged();
    void toolBoxShareConnectionCurrentIndexChanged();
    void pushButtonShareSsCopyEnabledChanged();
    void lineEditShareSsStringTextChanged();
    void labelShareSsQrCodeTextChanged();
    void labelShareSsServerTextChanged();
    void labelShareSsPortTextChanged();
    void labelShareSsMethodTextChanged();
    void labelShareSsPasswordTextChanged();
    void plainTextEditShareCloakTextChanged();
    void pushButtonShareCloakCopyEnabledChanged();
    void textEditShareFullCodeTextChanged();
    void textEditShareAmneziaCodeTextChanged();
    void pushButtonShareFullCopyTextChanged();
    void pushButtonShareAmneziaCopyTextChanged();
    void pushButtonShareOpenvpnCopyTextChanged();
    void pushButtonShareSsCopyTextChanged();
    void pushButtonShareCloakCopyTextChanged();
    void pushButtonShareAmneziaGenerateEnabledChanged();
    void pushButtonShareAmneziaCopyEnabledChanged();
    void pushButtonShareAmneziaGenerateTextChanged();
    void pushButtonShareOpenvpnGenerateEnabledChanged();
    void pushButtonShareOpenvpnGenerateTextChanged();

private:


private slots:



private:
    Settings m_settings;
    UiLogic *m_uiLogic;
    CQR_Encode m_qrEncode;

    bool m_pageShareAmneziaVisible;
    bool m_pageShareOpenvpnVisible;
    bool m_pageShareShadowsocksVisible;
    bool m_pageShareCloakVisible;
    bool m_pageShareFullAccessVisible;
    QString m_textEditShareOpenvpnCodeText;
    bool m_pushButtonShareOpenvpnCopyEnabled;
    bool m_pushButtonShareOpenvpnSaveEnabled;
    int m_toolBoxShareConnectionCurrentIndex;
    bool m_pushButtonShareSsCopyEnabled;
    QString m_lineEditShareSsStringText;
    QString m_labelShareSsQrCodeText;
    QString m_labelShareSsServerText;
    QString m_labelShareSsPortText;
    QString m_labelShareSsMethodText;
    QString m_labelShareSsPasswordText;
    QString m_plainTextEditShareCloakText;
    bool m_pushButtonShareCloakCopyEnabled;
    QString m_textEditShareFullCodeText;
    QString m_textEditShareAmneziaCodeText;
    QString m_pushButtonShareFullCopyText;
    QString m_pushButtonShareAmneziaCopyText;
    QString m_pushButtonShareOpenvpnCopyText;
    QString m_pushButtonShareSsCopyText;
    QString m_pushButtonShareCloakCopyText;
    bool m_pushButtonShareAmneziaGenerateEnabled;
    bool m_pushButtonShareAmneziaCopyEnabled;
    QString m_pushButtonShareAmneziaGenerateText;
    bool m_pushButtonShareOpenvpnGenerateEnabled;
    QString m_pushButtonShareOpenvpnGenerateText;

};
#endif // SHARE_CONNECTION_LOGIC_H

#ifndef START_PAGE_LOGIC_H
#define START_PAGE_LOGIC_H

#include "../pages.h"
#include "settings.h"

class UiLogic;

class StartPageLogic : public QObject
{
    Q_OBJECT

public:
    Q_INVOKABLE void updateStartPage();

    Q_PROPERTY(bool pushButtonNewServerConnectEnabled READ getPushButtonNewServerConnectEnabled WRITE setPushButtonNewServerConnectEnabled NOTIFY pushButtonNewServerConnectEnabledChanged)
    Q_PROPERTY(bool pushButtonNewServerConnectKeyChecked READ getPushButtonNewServerConnectKeyChecked WRITE setPushButtonNewServerConnectKeyChecked NOTIFY pushButtonNewServerConnectKeyCheckedChanged)
    Q_PROPERTY(QString pushButtonNewServerConnectText READ getPushButtonNewServerConnectText WRITE setPushButtonNewServerConnectText NOTIFY pushButtonNewServerConnectTextChanged)
    Q_PROPERTY(QString lineEditStartExistingCodeText READ getLineEditStartExistingCodeText WRITE setLineEditStartExistingCodeText NOTIFY lineEditStartExistingCodeTextChanged)
    Q_PROPERTY(QString textEditNewServerSshKeyText READ getTextEditNewServerSshKeyText WRITE setTextEditNewServerSshKeyText NOTIFY textEditNewServerSshKeyTextChanged)
    Q_PROPERTY(QString lineEditNewServerIpText READ getLineEditNewServerIpText WRITE setLineEditNewServerIpText NOTIFY lineEditNewServerIpTextChanged)
    Q_PROPERTY(QString lineEditNewServerPasswordText READ getLineEditNewServerPasswordText WRITE setLineEditNewServerPasswordText NOTIFY lineEditNewServerPasswordTextChanged)
    Q_PROPERTY(QString lineEditNewServerLoginText READ getLineEditNewServerLoginText WRITE setLineEditNewServerLoginText NOTIFY lineEditNewServerLoginTextChanged)
    Q_PROPERTY(bool labelNewServerWaitInfoVisible READ getLabelNewServerWaitInfoVisible WRITE setLabelNewServerWaitInfoVisible NOTIFY labelNewServerWaitInfoVisibleChanged)
    Q_PROPERTY(QString labelNewServerWaitInfoText READ getLabelNewServerWaitInfoText WRITE setLabelNewServerWaitInfoText NOTIFY labelNewServerWaitInfoTextChanged)
    Q_PROPERTY(bool pushButtonBackFromStartVisible READ getPushButtonBackFromStartVisible WRITE setPushButtonBackFromStartVisible NOTIFY pushButtonBackFromStartVisibleChanged)
    Q_PROPERTY(bool pushButtonNewServerConnectVisible READ getPushButtonNewServerConnectVisible WRITE setPushButtonNewServerConnectVisible NOTIFY pushButtonNewServerConnectVisibleChanged)

    Q_INVOKABLE void onPushButtonNewServerConnect();
    Q_INVOKABLE void onPushButtonNewServerImport();

public:
    explicit StartPageLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~StartPageLogic() = default;

    bool getPushButtonBackFromStartVisible() const;
    void setPushButtonBackFromStartVisible(bool pushButtonBackFromStartVisible);
    bool getPushButtonNewServerConnectEnabled() const;
    void setPushButtonNewServerConnectEnabled(bool pushButtonNewServerConnectEnabled);

    bool getPushButtonNewServerConnectVisible() const;
    void setPushButtonNewServerConnectVisible(bool pushButtonNewServerConnectVisible);
    bool getPushButtonNewServerConnectKeyChecked() const;
    void setPushButtonNewServerConnectKeyChecked(bool pushButtonNewServerConnectKeyChecked);
    QString getLineEditStartExistingCodeText() const;
    void setLineEditStartExistingCodeText(const QString &lineEditStartExistingCodeText);
    QString getTextEditNewServerSshKeyText() const;
    void setTextEditNewServerSshKeyText(const QString &textEditNewServerSshKeyText);
    QString getLineEditNewServerIpText() const;
    void setLineEditNewServerIpText(const QString &lineEditNewServerIpText);
    QString getLineEditNewServerPasswordText() const;
    void setLineEditNewServerPasswordText(const QString &lineEditNewServerPasswordText);
    QString getLineEditNewServerLoginText() const;
    void setLineEditNewServerLoginText(const QString &lineEditNewServerLoginText);
    bool getLabelNewServerWaitInfoVisible() const;
    void setLabelNewServerWaitInfoVisible(bool labelNewServerWaitInfoVisible);
    QString getLabelNewServerWaitInfoText() const;
    void setLabelNewServerWaitInfoText(const QString &labelNewServerWaitInfoText);

    QString getPushButtonNewServerConnectText() const;
    void setPushButtonNewServerConnectText(const QString &pushButtonNewServerConnectText);

signals:
    void pushButtonNewServerConnectKeyCheckedChanged();
    void lineEditStartExistingCodeTextChanged();
    void textEditNewServerSshKeyTextChanged();
    void lineEditNewServerIpTextChanged();
    void lineEditNewServerPasswordTextChanged();
    void lineEditNewServerLoginTextChanged();
    void labelNewServerWaitInfoVisibleChanged();
    void labelNewServerWaitInfoTextChanged();
    void pushButtonBackFromStartVisibleChanged();
    void pushButtonNewServerConnectVisibleChanged();
    void pushButtonNewServerConnectEnabledChanged();
    void pushButtonNewServerConnectTextChanged();

private:


private slots:



private:
    Settings m_settings;
    UiLogic *m_uiLogic;

    bool m_pushButtonNewServerConnectEnabled;
    QString m_pushButtonNewServerConnectText;
    bool m_pushButtonNewServerConnectKeyChecked;
    QString m_lineEditStartExistingCodeText;
    QString m_textEditNewServerSshKeyText;
    QString m_lineEditNewServerIpText;
    QString m_lineEditNewServerPasswordText;
    QString m_lineEditNewServerLoginText;
    bool m_labelNewServerWaitInfoVisible;
    QString m_labelNewServerWaitInfoText;
    bool m_pushButtonBackFromStartVisible;
    bool m_pushButtonNewServerConnectVisible;
};
#endif // START_PAGE_LOGIC_H

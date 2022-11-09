#ifndef SERVER_LIST_LOGIC_H
#define SERVER_LIST_LOGIC_H

#include "PageLogicBase.h"

class UiLogic;

class ServerListLogic : public PageLogicBase
{
    Q_OBJECT

    READONLY_PROPERTY(QObject *, serverListModel)
	Q_PROPERTY(int currServerIdx READ currServerIdx NOTIFY currServerIdxChanged)

public:
	int currServerIdx() const;

    Q_INVOKABLE void onUpdatePage() override;
    Q_INVOKABLE void onServerListPushbuttonDefaultClicked(int index);
    Q_INVOKABLE void onServerListPushbuttonSettingsClicked(int index);

public:
    explicit ServerListLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~ServerListLogic() = default;

signals:
	void currServerIdxChanged();

};
#endif // SERVER_LIST_LOGIC_H

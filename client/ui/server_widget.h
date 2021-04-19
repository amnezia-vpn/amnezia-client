#ifndef SERVER_WIDGET_H
#define SERVER_WIDGET_H

#include <QJsonObject>
#include <QWidget>

namespace Ui {
class ServerWidget;
}

class ServerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ServerWidget(const QJsonObject &server, bool isDefault, QWidget *parent = nullptr);
    ~ServerWidget();
    Ui::ServerWidget *ui;

};

#endif // SERVER_WIDGET_H

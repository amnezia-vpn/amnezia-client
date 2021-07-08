#ifndef SERVER_WIDGET_H
#define SERVER_WIDGET_H

#include <QJsonObject>
#include <QWidget>

#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>

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
private:
    void enterEvent(QEvent *e) override;
    void leaveEvent(QEvent *e) override;

    QPropertyAnimation animation;
    QGraphicsOpacityEffect eff;
};

#endif // SERVER_WIDGET_H

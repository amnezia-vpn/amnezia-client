#include "server_widget.h"
#include "ui_server_widget.h"

#include "settings.h"
#include "protocols/protocols_defs.h"

ServerWidget::ServerWidget(const QJsonObject &server, bool isDefault, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ServerWidget)
{
    ui->setupUi(this);
    ui->pushButton_connect->hide();
    ui->pushButton_share->hide();

    //ui->pushButton_settings->hide();\\

    ui->pushButton_settings->setGraphicsEffect(&eff);
    animation.setTargetObject(&eff);
    animation.setPropertyName("opacity");
    animation.setDuration(150);
    animation.setEasingCurve(QEasingCurve::InOutQuad);

    eff.setOpacity(0);

    QString desc = server.value(config_key::description).toString();
    QString address = server.value(config_key::hostName).toString();

    ui->label_address->setText(address);

    if (desc.isEmpty()) {
        ui->label_description->setText(address);
    }
    else {
        ui->label_description->setText(desc);
    }

    ui->pushButton_default->setChecked(isDefault);
    ui->pushButton_default->setDisabled(isDefault);
}

ServerWidget::~ServerWidget()
{
    delete ui;
}

void ServerWidget::enterEvent(QEvent *e)
{
    animation.setStartValue(0);
    animation.setEndValue(1);

    animation.start();
}

void ServerWidget::leaveEvent(QEvent *e)
{
    animation.setStartValue(1);
    animation.setEndValue(0);

    animation.start();
}


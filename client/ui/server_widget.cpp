#include "server_widget.h"
#include "ui_server_widget.h"

#include "settings.h"

ServerWidget::ServerWidget(const QJsonObject &server, bool isDefault, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ServerWidget)
{
    ui->setupUi(this);
    QString desc = server.value(Settings::descriptionString).toString();
    QString address = server.value(Settings::hostNameString).toString();

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

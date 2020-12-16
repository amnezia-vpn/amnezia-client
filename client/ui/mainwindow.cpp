#include <QKeyEvent>
#include <QMessageBox>

#include "debug.h"
#include "defines.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Post initialization
    ui->widget_tittlebar->hide();
    ui->stackedWidget_main->setCurrentIndex(2);

    connect(ui->pushButton_blocked_list, SIGNAL(clicked(bool)), this, SLOT(onPushButtonBlockedListClicked(bool)));
    connect(ui->pushButton_connect, SIGNAL(clicked(bool)), this, SLOT(onPushButtonConnectClicked(bool)));
    connect(ui->pushButton_settings, SIGNAL(clicked(bool)), this, SLOT(onPushButtonSettingsClicked(bool)));

    connect(ui->pushButton_back_from_sites, SIGNAL(clicked(bool)), this, SLOT(onPushButtonBackFromSitesClicked(bool)));
    connect(ui->pushButton_back_from_settings, SIGNAL(clicked(bool)), this, SLOT(onPushButtonBackFromSettingsClicked(bool)));

    setFixedSize(width(),height());

    qDebug() << APPLICATION_NAME;
    qDebug() << "Started";
}

MainWindow::~MainWindow()
{
    delete ui;

    qDebug() << "Closed";
}

void MainWindow::goToIndex(int index)
{
    ui->stackedWidget_main->setCurrentIndex(index);
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
        case Qt::Key_L:
            if (!Debug::openLogsFolder()) {
                QMessageBox::warning(this, APPLICATION_NAME, tr("Cannot open logs folder!"));
            }
        break;
        default:
        ;
    }
}

void MainWindow::onPushButtonBackFromSettingsClicked(bool)
{
    goToIndex(2);
}

void MainWindow::onPushButtonBackFromSitesClicked(bool)
{
    goToIndex(2);
}

void MainWindow::onPushButtonBlockedListClicked(bool)
{
    goToIndex(3);
}

void MainWindow::onPushButtonSettingsClicked(bool)
{
    goToIndex(4);
}

void MainWindow::onPushButtonConnectClicked(bool)
{
    qDebug() << "onPushButtonConnectClicked";
}



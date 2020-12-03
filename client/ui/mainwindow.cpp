#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

   // Post initialization
    ui->widget_tittlebar->hide();
    ui->stackedWidget_main->setCurrentIndex(2);
}

MainWindow::~MainWindow()
{
    delete ui;
}


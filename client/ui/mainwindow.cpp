#include <QMetaEnum>
#include <QMovie>
#include <QMessageBox>
#include <QMouseEvent>
#include <QScroller>
#include <QScrollBar>
#include <QDesktopWidget>
#include <QDesktopServices>
#include <QGridLayout>
#include <QTime>

#include "mainwindow.h"


#ifdef Q_OS_WIN
#include "ui_mainwindow.h"
#endif

#ifdef Q_OS_MAC
#include "ui_mainwindow_mac.h"
#include "publib/macos_functions.h"
#endif


MainWindow::MainWindow(bool useForceUseBrightIcons, QWidget *parent) : QMainWindow(parent),
    ui(new Ui::MainWindow),
    forceUseBrightIcons(useForceUseBrightIcons)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}


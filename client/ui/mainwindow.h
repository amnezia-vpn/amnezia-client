#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QAction>
#include <QMenu>
#include <QJsonDocument>
#include <QClipboard>
#include <QStringListModel>
#include <QDataStream>

#include <QGraphicsBlurEffect>
#include "customshadoweffect.h"



namespace Ui {
class MainWindow;
}

/**
 * @brief The MainWindow class - Main application window
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(bool setForceUseBrightIcons = false, QWidget *parent = nullptr);
    ~MainWindow();

public slots:


private slots:

private:
    Ui::MainWindow *ui;
    bool forceUseBrightIcons = false;
};

#endif // MAINWINDOW_H

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

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
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:


private slots:
    void onPushButtonBlockedListClicked(bool clicked);
    void onPushButtonConnectClicked(bool clicked);
    void onPushButtonSettingsClicked(bool clicked);

    void onPushButtonBackFromSettingsClicked(bool clicked);
    void onPushButtonBackFromSitesClicked(bool clicked);

protected:
    void keyPressEvent(QKeyEvent* event);

private:
    void goToIndex(int index);

    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickView>
#include "Ssh.hpp"

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication app(argc, argv);

    qmlRegisterType<Ssh> ("Ssh", 1, 0, "Ssh");

    QQuickView view;
    view.setSource(QUrl("qrc:/Qml/Main.qml"));
    view.show();
    return app.exec();
}

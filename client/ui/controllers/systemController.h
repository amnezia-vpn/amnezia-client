#ifndef SYSTEMCONTROLLER_H
#define SYSTEMCONTROLLER_H

#include <QObject>

#include "settings.h"

class SystemController : public QObject
{
    Q_OBJECT
public:
    explicit SystemController(const std::shared_ptr<Settings> &setting, QObject *parent = nullptr);

public slots:
    void saveFile(QString fileName, const QString &data);
    QString getFileName();

    void setQmlRoot(QObject *qmlRoot);

signals:
    void fileDialogAccepted();
    void fileDialogRejected();

private:
    std::shared_ptr<Settings> m_settings;

    QObject *m_qmlRoot;
};

#endif // SYSTEMCONTROLLER_H

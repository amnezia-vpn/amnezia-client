#ifndef SYSTEMCONTROLLER_H
#define SYSTEMCONTROLLER_H

#include <QObject>

#include "settings.h"

class SystemController : public QObject
{
    Q_OBJECT
public:
    explicit SystemController(const std::shared_ptr<Settings> &setting, QObject *parent = nullptr);

    static void saveFile(QString fileName, const QString &data);

public slots:
    QString getFileName(const QString &acceptLabel, const QString &nameFilter, const QString &selectedFile = "",
                        const bool isSaveMode = false, const QString &defaultSuffix = "");

    void setQmlRoot(QObject *qmlRoot);

    bool isAuthenticated();
signals:
    void fileDialogClosed(const bool isAccepted);

private:
    std::shared_ptr<Settings> m_settings;

    QObject *m_qmlRoot;
};

#endif // SYSTEMCONTROLLER_H

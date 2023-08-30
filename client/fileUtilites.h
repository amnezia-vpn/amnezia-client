#ifndef FILEUTILITES_H
#define FILEUTILITES_H

#include <QObject>
#include <QString>

class FileUtilites : public QObject
{
    Q_OBJECT

public:
    static void saveFile(QString fileName, const QString &data);
    static QString getFileName(QString fileName);
};

#endif // FILEUTILITES_H

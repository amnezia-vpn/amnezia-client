#ifndef FILEUTILS_H
#define FILEUTILS_H

#include <QString>
#include <QByteArray>

namespace FileUtils {

bool readFile(const QString &fileName, QByteArray &data);
bool readFile(const QString &fileName, QString &data);
void saveFile(const QString &fileName, const QString &data);

}

#endif // FILEUTILS_H



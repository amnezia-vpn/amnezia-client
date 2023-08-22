#include "fileUtilites.h"

#include <QDesktopServices>
#include <QStandardPaths>

void FileUtilites::saveFile(const QString &fileExtension, const QString &caption, const QString &fileName,
                            const QString &data)
{
    QString docDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QUrl fileUrl = QFileDialog::getSaveFileUrl(nullptr, caption, QUrl::fromLocalFile(docDir + "/" + fileName),
                                               "*" + fileExtension);
    if (fileUrl.isEmpty())
        return;
    if (!fileUrl.toString().endsWith(fileExtension)) {
        fileUrl = QUrl(fileUrl.toString() + fileExtension);
    }
    if (fileUrl.isEmpty())
        return;

    QFile save(fileUrl.toLocalFile());

    // todo check if save successful
    save.open(QIODevice::WriteOnly);
    save.write(data.toUtf8());
    save.close();

    QFileInfo fi(fileUrl.toLocalFile());
    QDesktopServices::openUrl(fi.absoluteDir().absolutePath());
}

QString FileUtilites::getFileName(QWidget *parent, const QString &caption, const QString &dir, const QString &filter,
                                  QString *selectedFilter, QFileDialog::Options options)
{
    QString fileName = QFileDialog::getOpenFileName(parent, caption, dir, filter, selectedFilter, options);

#ifdef Q_OS_ANDROID
    // patch for files containing spaces etc
    const QString sep { "raw%3A%2F" };
    if (fileName.startsWith("content://") && fileName.contains(sep)) {
        QString contentUrl = fileName.split(sep).at(0);
        QString rawUrl = fileName.split(sep).at(1);
        rawUrl.replace(" ", "%20");
        fileName = contentUrl + sep + rawUrl;
    }
#endif
    return fileName;
}

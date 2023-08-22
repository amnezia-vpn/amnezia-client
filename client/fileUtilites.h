#ifndef FILEUTILITES_H
#define FILEUTILITES_H

#include <QFileDialog>

class FileUtilites : public QObject
{
    Q_OBJECT

public:
    static void saveFile(const QString &fileExtension, const QString &caption, const QString &fileName,
                         const QString &data);

    static QString getFileName(QWidget *parent = nullptr, const QString &caption = QString(),
                               const QString &dir = QString(), const QString &filter = QString(),
                               QString *selectedFilter = nullptr, QFileDialog::Options options = QFileDialog::Options());
};

#endif // FILEUTILITES_H

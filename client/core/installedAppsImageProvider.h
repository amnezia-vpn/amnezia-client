#ifndef INSTALLEDAPPSIMAGEPROVIDER_H
#define INSTALLEDAPPSIMAGEPROVIDER_H

#include <QObject>
#include <QQuickImageProvider>

class InstalledAppsImageProvider : public QQuickImageProvider
{
public:
    InstalledAppsImageProvider();

    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize) override;
};

#endif // INSTALLEDAPPSIMAGEPROVIDER_H

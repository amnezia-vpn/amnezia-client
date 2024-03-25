#include "installedAppsImageProvider.h"

#include "platforms/android/android_controller.h"

InstalledAppsImageProvider::InstalledAppsImageProvider() : QQuickImageProvider(QQuickImageProvider::Pixmap)
{
}

QPixmap InstalledAppsImageProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
    return AndroidController::instance()->getAppIcon(id, size, requestedSize);
}

#include "MobileUtils.h"

MobileUtils::MobileUtils(QObject *parent) : QObject(parent)
{
}

bool MobileUtils::shareText(const QStringList &)
{
    return false;
}

QString MobileUtils::openFile()
{
    return QString();
}

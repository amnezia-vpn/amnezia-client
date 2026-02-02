#ifndef MIMECACHE_H
#define MIMECACHE_H

class QString;
class QUrl;

QString mimeTypeForExtension(const QString &extension);
QString mimeTypeForUrl(const QUrl &url);

#endif

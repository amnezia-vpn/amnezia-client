#include "jsonTranslation.h"

#include <QFile>

#include <QJsonDocument>
#include <QJsonObject>

JsonTranslation::JsonTranslation(QObject *parent) : QObject{parent} {
  loadTranslation(QLocale());
}

void JsonTranslation::loadTranslation(const QLocale &locale) {
  switch (locale.language()) {
  case QLocale::English:
    loadTranslationFile("://ui/jsonTranslations/en.json");
    break;
  case QLocale::Russian:
    loadTranslationFile("://ui/jsonTranslations/ru.json");
    break;
  default:
    loadTranslationFile("://ui/jsonTranslations/en.json");
    break;
  }
}

QString JsonTranslation::translate(QString translationKey) {
  const auto it = m_translationMap.find(translationKey);
  if (it == m_translationMap.end())
    return translationKey;
  return it.value();
}

void JsonTranslation::loadTranslationFile(QString filePath) {
  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly)) {
    qDebug() << "Failed to open translation file: " << filePath;
    return;
  }

  QByteArray translation = file.readAll();

  QJsonDocument document = QJsonDocument::fromJson(translation);
  QJsonObject object = document.object();

  QMap<QString, QString> translationMap{};

  for (QString key : object.keys()) {
    QString value = object[key].toString();
    translationMap.insert(key, value);
  }

  m_translationMap = translationMap;
}

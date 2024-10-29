#pragma once

#include <QMap>
#include <QObject>
#include <QString>
#include <QLocale>

class JsonTranslation : public QObject {
  Q_OBJECT

public:
  explicit JsonTranslation(QObject *parent = nullptr);

public slots:
  void loadTranslation(const QLocale &locale);

  QString translate(QString translationKey);

private:
  void loadTranslationFile(QString filePath);

private:
  QMap<QString, QString> m_translationMap{};
};
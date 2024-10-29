#include "errorParser.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "amnezia_application.h"

Errors ErrorParser::parse(QByteArray body, bool isFormError) {
  Errors err{};
  err.isFormError = isFormError;

  QJsonDocument document = QJsonDocument::fromJson(body);
  if (document.isNull()) {
    err.errorMessage =
        QCoreApplication::translate("ServerLocale", "Server error");
    return err;
  }

  auto *jsonTranslation = amnApp->jsonTranslation();

  QJsonObject obj = document.object();
  err.errorMessage = jsonTranslation->translate(obj["error"].toString());
  if (obj.contains("errors")) {
    QJsonObject errors = obj["errors"].toObject();

    if (errors.contains("fieldErrors")) {
      QJsonObject fieldErrors = errors["fieldErrors"].toObject();

      for (QString key : fieldErrors.keys()) {
        QJsonArray keyErrors = fieldErrors[key].toArray();
        QString keyErrorsJoined{};

        for (qsizetype i = 0; i < keyErrors.size(); i++) {
          keyErrorsJoined +=
              jsonTranslation->translate(keyErrors[i].toString());
          if (i != 0)
            keyErrorsJoined += "\n";
        }

        err.fieldErrors.insert(key, keyErrorsJoined);
      }
    }
  }

  return err;
}

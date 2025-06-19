#ifndef APINEWSCONTROLLER_H
#define APINEWSCONTROLLER_H

#include <QObject>
#include <QSharedPointer>
#include <memory>
#include <QJsonArray>

#include "settings.h"
#include "ui/models/newsmodel.h"
#include "core/controllers/gatewayController.h"
#include "core/api/apiDefs.h"

class ApiNewsController : public QObject
{
    Q_OBJECT
public:
    explicit ApiNewsController(const QSharedPointer<NewsModel> &newsModel,
                               const std::shared_ptr<Settings> &settings,
                               QObject *parent = nullptr);

    Q_INVOKABLE void fetchNews();

signals:
    void errorOccurred(ErrorCode errorCode);

private:
    QSharedPointer<NewsModel> m_newsModel;
    std::shared_ptr<Settings> m_settings;
};

#endif // APINEWSCONTROLLER_H 
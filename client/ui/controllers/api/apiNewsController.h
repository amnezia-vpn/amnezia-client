#ifndef APINEWSCONTROLLER_H
#define APINEWSCONTROLLER_H

#include <QJsonArray>
#include <QObject>
#include <QSharedPointer>
#include <memory>

#include "core/api/apiDefs.h"
#include "core/controllers/gatewayController.h"
#include "core/controllers/serversController.h"
#include "settings.h"
#include "ui/models/newsModel.h"

class ApiNewsController : public QObject
{
    Q_OBJECT
public:
    explicit ApiNewsController(const QSharedPointer<NewsModel> &newsModel, const std::shared_ptr<Settings> &settings,
                               const QSharedPointer<ServersController> &serversController, QObject *parent = nullptr);

    Q_INVOKABLE void fetchNews(bool showError);

signals:
    void errorOccurred(ErrorCode errorCode, bool showError);
    void fetchNewsFinished();

private:
    QSharedPointer<NewsModel> m_newsModel;
    std::shared_ptr<Settings> m_settings;
    QSharedPointer<ServersController> m_serversController;
};

#endif // APINEWSCONTROLLER_H

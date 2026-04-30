#ifndef APINEWSUICONTROLLER_H
#define APINEWSUICONTROLLER_H

#include <QJsonArray>
#include <QObject>

#include "core/utils/errorCodes.h"
#include "core/utils/routeModes.h"
#include "core/utils/commonStructs.h"
#include "core/controllers/api/newsController.h"
#include "ui/models/newsModel.h"

class ApiNewsUiController : public QObject
{
    Q_OBJECT
public:
    explicit ApiNewsUiController(NewsModel* newsModel,
                                 NewsController* newsController,
                                 QObject *parent = nullptr);

    Q_INVOKABLE void fetchNews(bool showError);

signals:
    void errorOccurred(ErrorCode errorCode, bool showError);
    void fetchNewsFinished();

private:
    NewsModel* m_newsModel;
    NewsController* m_newsController;
};

#endif // APINEWSUICONTROLLER_H

#include "apiNewsUiController.h"

#include <QtConcurrent/QtConcurrent>

ApiNewsUiController::ApiNewsUiController(NewsModel* newsModel,
                                         NewsController* newsController,
                                         QObject *parent)
    : QObject(parent), m_newsModel(newsModel), m_newsController(newsController)
{
}

void ApiNewsUiController::fetchNews(bool showError)
{
    QtConcurrent::run([this, showError]() {
        QJsonArray newsArray;
        ErrorCode errorCode = m_newsController->fetchNews(newsArray);
        
        if (errorCode != ErrorCode::NoError) {
            emit errorOccurred(errorCode, showError);
            return;
        }

        m_newsModel->updateModel(newsArray);
        emit fetchNewsFinished();
    });
}

#pragma once

#include <QKeySequence>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTest>

#include "secureQSettings.h"
#include "ui/controllers/qml/pageController.h"
#include "ui/utils/pageEnum.h"
#include "utils/testCoreController.h"

class PagesUiTestFixture
{
public:
    bool initialize(QObject *parent);
    void cleanup();

    bool loadPage(const QString &pageName);
    bool loadPageFromStartPage(PageLoader::PageEnum page);
    void closePage();

    TestCoreController *core() const
    {
        return m_core;
    }

    QQmlApplicationEngine *engine() const
    {
        return m_engine;
    }

    PageController *pageController() const
    {
        return m_pageController;
    }

    QQuickWindow *window() const
    {
        return m_window;
    }

    QQuickItem *page() const
    {
        return m_page;
    }

    QList<QQuickItem *> items(QQuickItem *root = nullptr) const;
    QList<QQuickItem *> itemsOfType(const QString &qmlType, QQuickItem *root = nullptr) const;

    QQuickItem *item(const QString &property, const QVariant &value, QQuickItem *root = nullptr) const;
    QQuickItem *visibleItem(const QString &property, const QVariant &value, QQuickItem *root = nullptr) const;
    QQuickItem *clickable(const QString &text) const;

    void click(QQuickItem *target);
    void typeText(QQuickItem *target, const QString &text);
    void clearAndType(QQuickItem *target, const QString &text);

private:
    static void collectItems(QQuickItem *root, QList<QQuickItem *> &result);
    static bool isQmlType(const QObject *object, const QString &qmlType);

    SecureQSettings *m_settings = nullptr;
    QQmlApplicationEngine *m_engine = nullptr;
    TestCoreController *m_core = nullptr;
    PageController *m_pageController = nullptr;

    QQuickWindow *m_window = nullptr;
    QQuickItem *m_page = nullptr;
};

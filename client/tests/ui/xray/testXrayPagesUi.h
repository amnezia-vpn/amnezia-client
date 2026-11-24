#ifndef TEST_XRAY_PAGES_UI_H
#define TEST_XRAY_PAGES_UI_H

#include <QClipboard>
#include <QGuiApplication>
#include <QJSValue>
#include <QJsonObject>
#include <QKeySequence>
#include <QQmlApplicationEngine>
#include <QQmlProperty>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSignalSpy>
#include <QTest>

#include "core/models/containerConfig.h"
#include "core/models/protocols/xrayInbound.h"
#include "core/models/protocols/xrayPresets.h"
#include "core/models/protocols/xrayProtocolConfig.h"
#include "core/models/selfhosted/selfHostedAdminServerConfig.h"
#include "core/utils/serverConfigUtils.h"
#include "secureQSettings.h"
#include "ui/controllers/qml/pageController.h"
#include "ui/controllers/selfhosted/installUiController.h"
#include "ui/controllers/serversUiController.h"
#include "ui/models/protocols/xrayPresetsModel.h"
#include "ui/models/protocols/xraySettingsModel.h"
#include "utils/testCoreController.h"
#include "utils/xrayMatrix.h"

#define XRAY_LOAD(page)                                                                                                \
    do {                                                                                                               \
        loadPage(QStringLiteral(page));                                                                                \
        if (QTest::currentTestFailed())                                                                                \
            return;                                                                                                    \
    } while (0)

#define XRAY_QUESTIONS(name)                                                                                           \
    QSignalSpy name(dialogs(), SIGNAL(questionRequested(QString, QString, QString, QString, QVariant, QVariant)))

class TestXrayPagesUi : public QObject
{
    Q_OBJECT

private:
    SecureQSettings *m_settings = nullptr;
    QQmlApplicationEngine *m_engine = nullptr;
    TestCoreController *m_core = nullptr;
    XraySettingsModel *m_model = nullptr;
    XrayPresetsModel *m_presets = nullptr;
    PageController *m_pages = nullptr;
    QString m_serverId;
    QQuickWindow *m_window = nullptr;
    QQuickItem *m_page = nullptr;

    static amnezia::ContainerConfig xrayContainer(const amnezia::XrayServerConfig &srv)
    {
        amnezia::ContainerConfig container;
        container.container = amnezia::DockerContainer::Xray;
        amnezia::XrayProtocolConfig xray;
        xray.serverConfig = srv;
        amnezia::XrayClientConfig client;
        client.nativeConfig = xraytest::compact(xraytest::clientJson(srv));
        client.localPort = QStringLiteral("10808");
        client.id = xraytest::clientId();
        xray.setClientConfig(client);
        container.protocolConfig = xray;
        return container;
    }

    static QJsonObject serverJson(const QString &host, const QString &password, const amnezia::XrayServerConfig &srv)
    {
        amnezia::SelfHostedAdminServerConfig server;
        server.hostName = host;
        server.userName = QStringLiteral("root");
        server.password = password;
        server.port = 22;
        server.description = QStringLiteral("xray ui test ") + (host.isEmpty() ? QStringLiteral("hostless") : host);
        server.displayName = server.description;
        server.defaultContainer = amnezia::DockerContainer::Xray;
        server.containers.insert(amnezia::DockerContainer::Xray, xrayContainer(srv));
        return server.toJson();
    }

    static amnezia::XrayServerConfig realityVision()
    {
        return xraytest::baseConfig(QStringLiteral("raw"), QStringLiteral("reality"), QStringLiteral("xtls-rprx-vision"));
    }

    static QString builtinName(const QString &id)
    {
        for (const amnezia::XrayPreset &preset :
             amnezia::xrayPresets::builtins(QStringLiteral("www.example.com"), QStringLiteral("443"))) {
            if (preset.id == id)
                return preset.name;
        }
        return {};
    }

    static QString withReplaced(QString text, const QString &from, const QString &to)
    {
        const int at = text.indexOf(from);
        return at < 0 ? text : text.replace(at, from.size(), to);
    }

    QString addServer(const QString &host, const QString &password, serverConfigUtils::ConfigType kind)
    {
        const int before = m_core->m_serversRepository->serversCount();
        const QString id =
                m_core->m_serversRepository->addServer(QString(), serverJson(host, password, realityVision()), kind);
        return m_core->m_serversRepository->serversCount() == before + 1 ? id : QString();
    }

    void storeAndOpen(const amnezia::XrayServerConfig &srv)
    {
        m_core->m_serversRepository->editServer(
                m_serverId, serverJson(QStringLiteral("203.0.113.10"), QStringLiteral("not-used"), srv),
                m_core->m_serversRepository->serverKind(m_serverId));
        m_core->m_installUiController->openServerSettings(m_serverId, static_cast<int>(amnezia::DockerContainer::Xray),
                                                          static_cast<int>(amnezia::Proto::Xray));
    }

    amnezia::XrayServerConfig stored() const
    {
        const amnezia::ContainerConfig container =
                m_core->m_serversController->getContainerConfig(m_serverId, amnezia::DockerContainer::Xray);
        const auto *xray = container.getXrayProtocolConfig();
        return xray ? xray->serverConfig : amnezia::XrayServerConfig {};
    }

    void selectServer(const QString &serverId)
    {
        m_core->m_serversUiController->updateModel();
        m_core->m_serversUiController->setProcessedServerId(serverId);
        m_core->m_serversUiController->setProcessedContainerIndex(static_cast<int>(amnezia::DockerContainer::Xray));
    }

    QStringList customIds() const
    {
        QStringList ids;
        for (int row = 0; row < m_presets->rowCount(); ++row) {
            const QModelIndex index = m_presets->index(row);
            if (!index.data(XrayPresetsModel::BuiltinRole).toBool())
                ids << index.data(XrayPresetsModel::IdRole).toString();
        }
        return ids;
    }

    void clearCustomPresets()
    {
        for (const QString &id : customIds())
            m_presets->removePreset(id);
    }

    static void step(const QString &what)
    {
        const char *tag = QTest::currentDataTag();
        qInfo().noquote() << "[XrayUiTest]" << (QTest::currentTestFunction() ? QTest::currentTestFunction() : "")
                          << (tag ? QStringLiteral("[%1]").arg(QString::fromUtf8(tag)) : QString()) << what;
    }

    static QString describe(const QObject *object)
    {
        if (!object)
            return QStringLiteral("<null>");
        QString name = QString::fromLatin1(object->metaObject()->className());
        name = name.left(name.indexOf(QLatin1String("_QML")));
        for (const char *property : { "text", "headerText", "title" }) {
            const QString value = object->property(property).toString();
            if (!value.isEmpty())
                return QStringLiteral("%1 \"%2\"").arg(name, value);
        }
        return name;
    }

    QString editedInbound(const QString &from, const QString &to) const
    {
        return withReplaced(m_model->inboundText(), from, to);
    }

    void loadPage(const QString &page)
    {
        closePage();
        step(QStringLiteral("load page %1").arg(page));
        const QByteArray qml = QStringLiteral("import QtQuick\n"
                                              "import QtQuick.Controls\n"
                                              "import \"../Config\"\n"
                                              "ApplicationWindow {\n"
                                              "    width: 420; height: 800; visible: true\n"
                                              "    property var dialogs: Dialogs\n"
                                              "    Loader { objectName: \"pageLoader\"; anchors.fill: parent; source: "
                                              "\"qrc:/ui/qml/Pages2/%1.qml\" }\n"
                                              "}\n")
                                       .arg(page)
                                       .toUtf8();
        m_engine->loadData(qml, QUrl(QStringLiteral("qrc:/ui/qml/Pages2/xrayUiTestHost.qml")));
        QVERIFY2(!m_engine->rootObjects().isEmpty(), "test host window did not load");
        m_window = qobject_cast<QQuickWindow *>(m_engine->rootObjects().last());
        QVERIFY(m_window);
        step(QStringLiteral("wait for window exposed"));
        QVERIFY(QTest::qWaitForWindowExposed(m_window));
        m_window->requestActivate();
        step(QStringLiteral("wait for window active"));
        QVERIFY(QTest::qWaitForWindowActive(m_window));
        auto *loader = m_window->findChild<QObject *>(QStringLiteral("pageLoader"));
        QVERIFY(loader);
        QTRY_VERIFY2(loader->property("status").toInt() != 2, qPrintable(page + QStringLiteral(" is still loading")));
        const int status = loader->property("status").toInt();
        step(QStringLiteral("loader status %1").arg(status));
        QVERIFY2(status == 1, qPrintable(page + QStringLiteral(" failed to load, see the QWARN lines above")));
        m_page = qvariant_cast<QQuickItem *>(loader->property("item"));
        QVERIFY(m_page);
        step(QStringLiteral("page %1 loaded").arg(page));
    }

    void closePage()
    {
        m_page = nullptr;
        if (m_window) {
            step(QStringLiteral("close page window"));
            m_window->close();
            m_window->deleteLater();
            m_window = nullptr;
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        }
    }

    QObject *dialogs() const
    {
        return m_window ? qvariant_cast<QObject *>(m_window->property("dialogs")) : nullptr;
    }

    static void answer(const QSignalSpy &questions, bool yes)
    {
        step(QStringLiteral("answer question \"%1\" with %2")
                     .arg(questions.last().at(0).toString(), yes ? QStringLiteral("yes") : QStringLiteral("no")));
        QJSValue fn = questions.last().at(yes ? 4 : 5).value<QJSValue>();
        fn.call();
        QTest::qWait(50);
    }

    static void collect(QQuickItem *item, QList<QQuickItem *> &out)
    {
        out.append(item);
        for (QQuickItem *child : item->childItems())
            collect(child, out);
    }

    QList<QQuickItem *> items(QQuickItem *root = nullptr) const
    {
        QList<QQuickItem *> out;
        if (QQuickItem *start = root ? root : m_page)
            collect(start, out);
        return out;
    }

    static bool isType(const QObject *object, const char *qmlType)
    {
        return QString::fromLatin1(object->metaObject()->className()).startsWith(QLatin1String(qmlType));
    }

    QQuickItem *typed(const char *qmlType, const char *property, const QString &value, QQuickItem *root = nullptr)
    {
        for (QQuickItem *candidate : items(root)) {
            if (!isType(candidate, qmlType))
                continue;
            const QVariant v = candidate->property(property);
            if (v.isValid() && v.toString() == value)
                return candidate;
        }
        qWarning().noquote() << "no" << qmlType << "with" << property << "=" << value;
        return nullptr;
    }

    QList<QQuickItem *> allOfType(const char *qmlType, QQuickItem *root = nullptr)
    {
        QList<QQuickItem *> out;
        for (QQuickItem *candidate : items(root)) {
            if (isType(candidate, qmlType))
                out.append(candidate);
        }
        return out;
    }

    QList<QQuickItem *> visibleOfType(const char *qmlType, QQuickItem *root = nullptr)
    {
        QList<QQuickItem *> out;
        for (QQuickItem *candidate : allOfType(qmlType, root)) {
            if (candidate->isVisible())
                out.append(candidate);
        }
        return out;
    }

    static QQuickItem *input(QQuickItem *field)
    {
        return field ? qvariant_cast<QQuickItem *>(field->property("textField")) : nullptr;
    }

    QQuickItem *field(const QString &headerText, QQuickItem *root = nullptr)
    {
        return input(typed("TextFieldType", "headerText", headerText, root));
    }

    QQuickItem *row(const QString &text)
    {
        return typed("ListItemType", "text", text);
    }

    QQuickItem *radio(const QString &text)
    {
        for (QQuickItem *candidate : visibleOfType("RadioItemType")) {
            if (candidate->property("text").toString() == text)
                return candidate;
        }
        qWarning().noquote() << "no visible RadioItemType" << text;
        return nullptr;
    }

    QQuickItem *button(const QString &text, QQuickItem *root = nullptr)
    {
        return typed("ButtonType", "text", text, root);
    }

    QQuickItem *iconButton(const QString &iconSuffix, QQuickItem *root = nullptr)
    {
        for (QQuickItem *candidate : items(root)) {
            QQmlProperty source(candidate, QStringLiteral("icon.source"));
            if (source.isValid() && source.read().toUrl().toString().endsWith(iconSuffix))
                return candidate;
        }
        qWarning().noquote() << "no icon button" << iconSuffix;
        return nullptr;
    }

    QQuickItem *codeEditor()
    {
        return allOfType("XrayCodeEditor").value(0);
    }

    static QQuickItem *codeArea(QQuickItem *editor)
    {
        return editor ? qvariant_cast<QQuickItem *>(editor->property("textArea")) : nullptr;
    }

    QObject *popup(const char *qmlType, const char *property = nullptr, const QString &value = QString()) const
    {
        if (!m_page)
            return nullptr;
        for (QObject *child : m_page->findChildren<QObject *>()) {
            if (!child->inherits("QQuickPopup") || !isType(child, qmlType))
                continue;
            if (!property || child->property(property).toString() == value)
                return child;
        }
        qWarning().noquote() << "no popup" << qmlType << (property ? property : "") << value;
        return nullptr;
    }

    QObject *alert(const QString &headerText) const
    {
        return popup("AlertDialogType", "headerText", headerText);
    }

    static QQuickItem *popupContent(QObject *popup)
    {
        return popup ? qvariant_cast<QQuickItem *>(popup->property("contentItem")) : nullptr;
    }

    static bool waitOpened(QObject *popup)
    {
        step(QStringLiteral("wait for %1 to open").arg(describe(popup)));
        const bool opened = popup && QTest::qWaitFor([popup] { return popup->property("opened").toBool(); }, 3000);
        step(QStringLiteral("%1 %2").arg(describe(popup),
                                         opened ? QStringLiteral("opened") : QStringLiteral("did not open")));
        return opened;
    }

    static bool waitClosed(QObject *popup)
    {
        step(QStringLiteral("wait for %1 to close").arg(describe(popup)));
        const bool closed = popup && QTest::qWaitFor([popup] { return !popup->property("visible").toBool(); }, 3000);
        step(QStringLiteral("%1 %2").arg(describe(popup),
                                         closed ? QStringLiteral("closed") : QStringLiteral("did not close")));
        return closed;
    }

    bool clickDialogButton(QObject *dialog, const QString &text)
    {
        QQuickItem *target = typed("DialogButtonType", "text", text, popupContent(dialog));
        if (!target)
            return false;
        click(target);
        return waitClosed(dialog);
    }

    static void polishAncestors(QQuickItem *item)
    {
        QList<QQuickItem *> chain;
        for (QQuickItem *p = item; p; p = p->parentItem())
            chain.prepend(p);
        for (QQuickItem *p : chain)
            p->ensurePolished();
    }

    void scrollIntoView(QQuickItem *target)
    {
        for (QQuickItem *p = target->parentItem(); p; p = p->parentItem()) {
            if (!p->property("contentY").isValid() || !p->property("contentHeight").isValid())
                continue;
            auto *content = qvariant_cast<QQuickItem *>(p->property("contentItem"));
            if (!content)
                continue;
            const qreal inView = target->mapToItem(p, QPointF(0, 0)).y();
            if (inView >= 0 && inView + target->height() <= p->height())
                return;
            const qreal originY = p->property("originY").toReal();
            const qreal y = target->mapToItem(content, QPointF(0, 0)).y();
            const qreal maxY = qMax<qreal>(originY, originY + p->property("contentHeight").toReal() - p->height());
            p->setProperty("contentY", qBound<qreal>(originY, y - p->height() / 3, maxY));
            QTest::qWait(50);
            return;
        }
    }

    void click(QQuickItem *target)
    {
        step(QStringLiteral("click %1").arg(describe(target)));
        if (!target)
            return;
        polishAncestors(target);
        scrollIntoView(target);
        const QPointF center = target->mapToScene(QPointF(target->width() / 2, target->height() / 2));
        QTest::mouseClick(m_window, Qt::LeftButton, {}, center.toPoint());
        QTest::qWait(50);
    }

    void type(const QString &text)
    {
        step(QStringLiteral("type \"%1\"").arg(text));
        for (const QChar ch : text)
            QTest::keyClick(m_window, ch.toLatin1());
    }

    void clearAndType(QQuickItem *textInput, const QString &text)
    {
        click(textInput);
        QTest::keySequence(m_window, QKeySequence::SelectAll);
        QTest::keyClick(m_window, Qt::Key_Backspace);
        type(text);
        QTest::qWait(30);
    }

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void cleanupTestCase();

    void settingsShowsMatchedPreset();
    void settingsUnmatchedConfigIsCustom();
    void settingsPresetRowNavigates();
    void settingsEditRowOpensEditor();
    void settingsPresetChangeTogglesSave();
    void settingsEditedPresetSubtitle();
    void settingsSaveDialogCancelled();
    void settingsResetDialogTexts();
    void settingsResetAccepted();
    void settingsResetCancelled();
    void settingsKeyMigrationDialog();
    void settingsBackButtonClosesPage();

    void presetsBuiltinList();
    void presetsBuiltinClickSelects();
    void presetsCustomGroupHiddenWhenEmpty();
    void presetsCustomListed();
    void presetsCustomClickSelects();
    void presetsMenuButtonDoesNotSelect();
    void presetsMenuShareNavigates();
    void presetsDeleteCancelled();
    void presetsDeleteConfirmed();
    void presetsDeleteSelectedFallsBackToBuiltin();
    void presetsImportRowNavigates();
    void presetsShareCurrentNavigates();
    void presetsSaveAsSheet();
    void presetsSaveAsSheetNeedsName();

    void editorShowsDraft();
    void editorTypingBreaksJson();
    void editorRuleErrors_data();
    void editorRuleErrors();
    void editorFormat();
    void editorFormatRefusesBrokenJson();
    void editorDoneCommits();
    void editorBackWithoutChanges();
    void editorBackDiscardCancelled();
    void editorBackDiscardConfirmed();
    void editorCopy();

    void importAddDisabledInitially();
    void importInvalidJsonShowsError_data();
    void importInvalidJsonShowsError();
    void importValidJsonNeedsName();
    void importAddsPreset();
    void shareCurrentPreset();
    void shareCustomPreset();
    void shareCopyJson();
};

#endif

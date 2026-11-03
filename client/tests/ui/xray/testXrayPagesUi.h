#ifndef TEST_XRAY_PAGES_UI_H
#define TEST_XRAY_PAGES_UI_H

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
#include "core/models/protocols/xrayProtocolConfig.h"
#include "core/models/selfhosted/selfHostedAdminServerConfig.h"
#include "core/utils/serverConfigUtils.h"
#include "ui/controllers/selfhosted/installUiController.h"
#include "ui/controllers/serversUiController.h"
#include "secureQSettings.h"
#include "ui/controllers/qml/pageController.h"
#include "ui/models/protocols/xrayConfigModel.h"
#include "ui/models/protocols/xrayConfigSnapshotsModel.h"
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
    XrayConfigModel *m_model = nullptr;
    XrayConfigSnapshotsModel *m_snapshots = nullptr;
    PageController *m_pages = nullptr;
    QString m_serverId;
    QString m_readOnlyServerId;
    QString m_hostlessServerId;
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

    QString addServer(const QString &host, const QString &password, serverConfigUtils::ConfigType kind)
    {
        const int before = m_core->m_serversRepository->serversCount();
        const QString id = m_core->m_serversRepository->addServer(
                QString(), serverJson(host, password, xraytest::baseConfig("xhttp", "reality", "")), kind);
        return m_core->m_serversRepository->serversCount() == before + 1 ? id : QString();
    }

    void storeAndOpen(const amnezia::XrayServerConfig &srv, const QString &serverId = QString())
    {
        const QString id = serverId.isEmpty() ? m_serverId : serverId;
        const auto kind = m_core->m_serversRepository->serverKind(id);
        const QString host = id == m_serverId ? QStringLiteral("203.0.113.10")
                : id == m_readOnlyServerId    ? QStringLiteral("203.0.113.11")
                                              : QString();
        const QString password = id == m_readOnlyServerId ? QString() : QStringLiteral("not-used");
        m_core->m_serversRepository->editServer(id, serverJson(host, password, srv), kind);
        m_core->m_installUiController->openServerSettings(id, static_cast<int>(amnezia::DockerContainer::Xray),
                                                          static_cast<int>(amnezia::Proto::Xray));
    }

    void openStored(const amnezia::XrayServerConfig &srv)
    {
        amnezia::XrayProtocolConfig cfg = *xrayContainer(srv).getXrayProtocolConfig();
        m_model->updateModel(amnezia::DockerContainer::Xray, cfg);
    }

    amnezia::XrayServerConfig stored(const QString &serverId = QString()) const
    {
        const amnezia::ContainerConfig container = m_core->m_serversController->getContainerConfig(
                serverId.isEmpty() ? m_serverId : serverId, amnezia::DockerContainer::Xray);
        const auto *xray = container.getXrayProtocolConfig();
        return xray ? xray->serverConfig : amnezia::XrayServerConfig {};
    }

    void selectServer(const QString &serverId)
    {
        m_core->m_serversUiController->updateModel();
        m_core->m_serversUiController->setProcessedServerId(serverId);
        m_core->m_serversUiController->setProcessedContainerIndex(static_cast<int>(amnezia::DockerContainer::Xray));
    }

    void loadPage(const QString &page)
    {
        closePage();
        const QByteArray qml = QStringLiteral(
                "import QtQuick\n"
                "import QtQuick.Controls\n"
                "import \"../Config\"\n"
                "ApplicationWindow {\n"
                "    width: 420; height: 800; visible: true\n"
                "    property var dialogs: Dialogs\n"
                "    Loader { objectName: \"pageLoader\"; anchors.fill: parent; source: \"qrc:/ui/qml/Pages2/%1.qml\" }\n"
                "}\n").arg(page).toUtf8();
        m_engine->loadData(qml, QUrl(QStringLiteral("qrc:/ui/qml/Pages2/xrayUiTestHost.qml")));
        QVERIFY2(!m_engine->rootObjects().isEmpty(), "test host window did not load");
        m_window = qobject_cast<QQuickWindow *>(m_engine->rootObjects().last());
        QVERIFY(m_window);
        QVERIFY(QTest::qWaitForWindowExposed(m_window));
        m_window->requestActivate();
        QVERIFY(QTest::qWaitForWindowActive(m_window));
        auto *loader = m_window->findChild<QObject *>(QStringLiteral("pageLoader"));
        QVERIFY(loader);
        QTRY_VERIFY2(loader->property("status").toInt() == 1, qPrintable(page + QStringLiteral(" failed to load")));
        m_page = qvariant_cast<QQuickItem *>(loader->property("item"));
        QVERIFY(m_page);
    }

    void closePage()
    {
        m_page = nullptr;
        if (m_window) {
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

    QQuickItem *item(const char *property, const QString &value, QQuickItem *root = nullptr)
    {
        for (QQuickItem *candidate : items(root)) {
            const QVariant v = candidate->property(property);
            if (v.isValid() && v.toString() == value)
                return candidate;
        }
        qWarning().noquote() << "no item with" << property << "=" << value;
        return nullptr;
    }

    QQuickItem *visibleItem(const char *property, const QString &value, QQuickItem *root = nullptr)
    {
        for (QQuickItem *candidate : items(root)) {
            const QVariant v = candidate->property(property);
            if (v.isValid() && v.toString() == value && candidate->isVisible())
                return candidate;
        }
        qWarning().noquote() << "no visible item with" << property << "=" << value;
        return nullptr;
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

    static QQuickItem *input(QQuickItem *field)
    {
        return field ? qvariant_cast<QQuickItem *>(field->property("textField")) : nullptr;
    }

    static QQuickItem *ownerField(QQuickItem *textInput)
    {
        QQuickItem *p = textInput ? textInput->parentItem() : nullptr;
        while (p && !isType(p, "TextFieldType"))
            p = p->parentItem();
        return p;
    }

    QQuickItem *field(const QString &headerText)
    {
        return input(typed("TextFieldType", "headerText", headerText));
    }

    QQuickItem *visibleField(const QString &headerText)
    {
        for (QQuickItem *candidate : allOfType("TextFieldType")) {
            if (candidate->property("headerText").toString() == headerText && candidate->isVisible())
                return input(candidate);
        }
        qWarning().noquote() << "no visible TextFieldType" << headerText;
        return nullptr;
    }

    QQuickItem *dropDown(const QString &descriptionText)
    {
        for (QQuickItem *candidate : allOfType("DropDownType")) {
            if (candidate->property("descriptionText").toString() == descriptionText && candidate->isVisible())
                return candidate;
        }
        return typed("DropDownType", "descriptionText", descriptionText);
    }

    QQuickItem *minMaxRow(int index) { return allOfType("MinMaxRowType").value(index); }

    QQuickItem *minMaxInput(QQuickItem *row, const QString &which)
    {
        return row ? input(typed("TextFieldType", "headerText", which, row)) : nullptr;
    }

    QQuickItem *iconButton(const QString &iconSuffix)
    {
        for (QQuickItem *candidate : items()) {
            QQmlProperty source(candidate, QStringLiteral("icon.source"));
            if (source.isValid() && source.read().toUrl().toString().endsWith(iconSuffix))
                return candidate;
        }
        qWarning().noquote() << "no icon button" << iconSuffix;
        return nullptr;
    }

    QQuickItem *saveButton() { return typed("ButtonType", "text", QStringLiteral("Save")); }

    QQuickItem *switcherOf(QQuickItem *switchRow)
    {
        return switchRow ? qvariant_cast<QQuickItem *>(switchRow->property("switcher")) : nullptr;
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

    static void polishAncestors(QQuickItem *item)
    {
        QList<QQuickItem *> chain;
        for (QQuickItem *p = item; p; p = p->parentItem())
            chain.prepend(p);
        for (QQuickItem *p : chain)
            p->ensurePolished();
    }

    void click(QQuickItem *target)
    {
        polishAncestors(target);
        scrollIntoView(target);
        const QPointF center = target->mapToScene(QPointF(target->width() / 2, target->height() / 2));
        QTest::mouseClick(m_window, Qt::LeftButton, {}, center.toPoint());
        QTest::qWait(50);
    }

    void type(const QString &text)
    {
        for (const QChar ch : text)
            QTest::keyClick(m_window, ch.toLatin1());
    }

    void key(Qt::Key k)
    {
        QTest::keyClick(m_window, k);
        QTest::qWait(30);
    }

    void clearAndType(QQuickItem *textInput, const QString &text, bool commit = true)
    {
        click(textInput);
        QTest::keySequence(m_window, QKeySequence::SelectAll);
        QTest::keyClick(m_window, Qt::Key_Backspace);
        type(text);
        if (commit)
            key(Qt::Key_Return);
        QTest::qWait(30);
    }

    void blur(QQuickItem *otherInput) { click(otherInput); }

    static QObject *popupOf(QObject *owner)
    {
        if (!owner)
            return nullptr;
        for (QObject *child : owner->findChildren<QObject *>()) {
            if (child->inherits("QQuickPopup"))
                return child;
        }
        return nullptr;
    }

    static QQuickItem *popupContent(QObject *popup)
    {
        return popup ? qvariant_cast<QQuickItem *>(popup->property("contentItem")) : nullptr;
    }

    QObject *openDropDown(QQuickItem *dd)
    {
        QObject *popup = popupOf(dd);
        if (!popup)
            return nullptr;
        for (int attempt = 0; attempt < 2; ++attempt) {
            QTest::qWait(100);
            click(dd);
            if (QTest::qWaitFor([popup] { return popup->property("opened").toBool(); }, 1500)) {
                QTest::qWait(250);
                return popup;
            }
        }
        return nullptr;
    }

    bool closeWithEscape(QObject *popup)
    {
        key(Qt::Key_Escape);
        return QTest::qWaitFor([popup] { return !popup->property("visible").toBool(); }, 3000);
    }

    QList<QQuickItem *> options(QObject *popup)
    {
        QList<QQuickItem *> out;
        for (QQuickItem *candidate : allOfType("RadioItemType", popupContent(popup))) {
            if (candidate->isVisible())
                out.append(candidate);
        }
        return out;
    }

    bool chooseOption(QQuickItem *dd, const QString &option)
    {
        QObject *popup = openDropDown(dd);
        if (!popup) {
            qWarning().noquote() << "drop-down did not open";
            return false;
        }
        for (QQuickItem *candidate : options(popup)) {
            if (candidate->property("text").toString() == option) {
                click(candidate);
                return QTest::qWaitFor([popup] { return !popup->property("visible").toBool(); }, 3000);
            }
        }
        qWarning().noquote() << "no option" << option;
        closeWithEscape(popup);
        return false;
    }

    QStringList optionNames(QQuickItem *dd, bool checkedOnly)
    {
        QStringList names;
        QObject *popup = openDropDown(dd);
        if (!popup)
            return { QStringLiteral("<did not open>") };
        for (QQuickItem *candidate : options(popup)) {
            if (!checkedOnly || candidate->property("checked").toBool())
                names << candidate->property("text").toString();
        }
        closeWithEscape(popup);
        return names;
    }

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void cleanupTestCase();
    void transportRadioClickChangesModel();
    void packetUpFieldsFollowMode_data();
    void packetUpFieldsFollowMode();
    void pathGetsLeadingSlash();
    void visionNeedsTlsOrReality_data();
    void visionNeedsTlsOrReality();
    void fingerprintHiddenOnMkcp_data();
    void fingerprintHiddenOnMkcp();
    void sniRejectsWildcardInput();
    void xPaddingFieldsFollowObfsMode_data();
    void xPaddingFieldsFollowObfsMode();
    void subpageSaveRefusesInvalidSni_data();
    void subpageSaveRefusesInvalidSni();

    void mainPortPlaceholderAndDefault();
    void mainPortInput_data();
    void mainPortInput();
    void mainPortEditTogglesSave();
    void mainEmptyPortDisablesSave();
    void mainEmptyPortErrorOnFocusOut();
    void mainPortFollowsModelOnlyWhenUnfocused();
    void mainSaveRefusesInvalidPort();
    void mainSaveJoinsErrors();
    void mainPortChangeDialog();
    void mainTransportSubtitle_data();
    void mainTransportSubtitle();
    void mainSecuritySubtitle_data();
    void mainSecuritySubtitle();
    void mainFlowSubtitle_data();
    void mainFlowSubtitle();
    void mainRowsNavigate_data();
    void mainRowsNavigate();
    void mainSnapshotsIconNavigates();
    void mainBackButtonClosesPage();
    void mainResetDialogTexts();
    void mainResetAccepted();
    void mainResetCancelled();
    void mainKeyMigrationDialog();
    void mainReadOnlyServer();
    void mainWritableServerBaseline();

    void transportXhttpRadioSubtitle();
    void transportRadioCheckedFollowsModel_data();
    void transportRadioCheckedFollowsModel();
    void transportRadioReturnKey();
    void transportSectionVisibility_data();
    void transportSectionVisibility();
    void transportRawToXhttpFillsDefaults();
    void transportSwitchKeepsXhttpValues();
    void transportPacketUpFieldsMoreRows_data();
    void transportPacketUpFieldsMoreRows();
    void transportPacketUpFieldsFollowModeLive();
    void transportAlwaysVisibleXhttpControls_data();
    void transportAlwaysVisibleXhttpControls();
    void transportDropDownOptions_data();
    void transportDropDownOptions();
    void transportDropDownRoundTrip_data();
    void transportDropDownRoundTrip();
    void transportDropDownEscape();
    void transportChoosingCurrentModeIsNoChange();
    void transportPacketUpRealityAllowed();
    void transportSwitches_data();
    void transportSwitches();
    void transportSwitchLabelToggles();
    void transportHostFilter();
    void transportHostValidity_data();
    void transportHostValidity();
    void transportEmptyHost();
    void transportSaveRefusesInvalidHost();
    void transportPathFilter();
    void transportPathEdgeCases_data();
    void transportPathEdgeCases();
    void transportKeyFieldsFilter_data();
    void transportKeyFieldsFilter();
    void transportNumberFields_data();
    void transportNumberFields();
    void transportMinMaxRows_data();
    void transportMinMaxRows();
    void transportMinMaxOverflow();
    void transportMkcpPlaceholders_data();
    void transportMkcpPlaceholders();
    void transportMkcpClamps_data();
    void transportMkcpClamps();
    void transportMtuRaisesSendingWindow_data();
    void transportMtuRaisesSendingWindow();
    void transportSaveDialogDescription_data();
    void transportSaveDialogDescription();
    void transportSubpageRows();
    void transportReadOnly();

    void securityMatrix_data();
    void securityMatrix();
    void securityRadioRoundTrip();
    void securityRealityNotClickableOnMkcp();
    void securityMkcpStoredReality();
    void securityDropDownOptions_data();
    void securityDropDownOptions();
    void securityDropDownRoundTrip_data();
    void securityDropDownRoundTrip();
    void securityFingerprintSharedBetweenBlocks();
    void securityValidSni();
    void securitySniSiteCoupling();
    void securityInvalidSni_data();
    void securityInvalidSni();
    void securityIpv4Sni();
    void securitySniFilter();
    void securityEmptySni();
    void securitySniErrorOnFocusOut();
    void securityNoneIgnoresSni();
    void securitySaveDialogDescription_data();
    void securitySaveDialogDescription();

    void flowRadioRoundTrip();
    void flowUdp443Subtitle();
    void flowVisionNotAllowed_data();
    void flowVisionNotAllowed();
    void flowEmptyWhenVisionNotAllowed();
    void flowDeviceOrServerChange_data();
    void flowDeviceOrServerChange();
    void flowEmptyTransportCountsAsRaw();
    void flowUnknownStoredFlow();
    void flowKeyboard();
    void paddingInitialState();
    void paddingSwitchTogglesFields();
    void paddingSwitchLabelToggles();
    void paddingBytesSubtitle_data();
    void paddingBytesSubtitle();
    void paddingBytesRowNavigates_data();
    void paddingBytesRowNavigates();
    void paddingKeyHeaderFilter_data();
    void paddingKeyHeaderFilter();
    void paddingDropDownRoundTrip_data();
    void paddingDropDownRoundTrip();
    void paddingDisabledDropDownDoesNotOpen();
    void paddingBytesInitialState();
    void paddingBytesRange_data();
    void paddingBytesRange();
    void paddingBytesOverflowAndFilter();

    void xmuxInitialState();
    void xmuxSwitchDisablesFields();
    void xmuxMinRaisesMax();
    void xmuxMaxBelowMin();
    void xmuxBothLimitsRefusedOnSave();
    void xmuxZeroCountsAsUnset();
    void xmuxDisabledIsNotValidated();
    void xmuxTwoErrorsAtOnce();
    void xmuxKeepAlive_data();
    void xmuxKeepAlive();
    void xmuxEditIsDeviceOnly();

    void saveInitiallyDisabled_data();
    void saveInitiallyDisabled();
    void saveEditThenRevert();
    void saveReloadDiscardsEdits();
    void saveCancelDoesNothing();
    void saveDeviceOnlyContinue_data();
    void saveDeviceOnlyContinue();
    void saveServerChangeOnHostlessServer();
    void saveCppGuardRefusesInvalid();
    void saveReadOnlySubpages_data();
    void saveReadOnlySubpages();
    void snapshotsEmptyState();
    void snapshotsCreate();
    void snapshotsApply();
    void snapshotsDelete();
    void snapshotsDeleteCancelled();
    void snapshotsImportExport();
    void snapshotsExportWithoutSnapshots();

    void issueTransportRoundTripLeavesSaveEnabled();
    void issueSubpageSavesEmptyPort();
    void issueHiddenRealitySniBlocksMkcpSave();
    void issueClearingXmuxZeroEnablesSave();
    void issueSnapshotWithoutPortApplyIsSilent();
    void issueSaveWithoutReturnSkipsClamp();
    void issueSaveWithoutReturnKeepsRawKeepAlive();
};

#endif

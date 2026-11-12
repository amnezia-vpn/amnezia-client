#include <QTest>

#include "core/models/protocols/awgProtocolConfig.h"

using namespace amnezia;

class TestAwgHeaders : public QObject
{
    Q_OBJECT

private slots:
    void headers_data()
    {
        QTest::addColumn<QString>("h1");
        QTest::addColumn<QString>("h2");
        QTest::addColumn<QString>("h3");
        QTest::addColumn<QString>("h4");
        QTest::addColumn<bool>("equal");

        QTest::newRow("defaults") << "1" << "2" << "3" << "4" << false;
        QTest::newRow("same value") << "5" << "5" << "3" << "4" << true;
        QTest::newRow("same range") << "100-200" << "100-200" << "3" << "4" << true;
        QTest::newRow("overlapping ranges") << "100-200" << "150-250" << "3" << "4" << true;
        QTest::newRow("touching ranges") << "100-200" << "200-300" << "3" << "4" << true;
        QTest::newRow("value inside range") << "100-200" << "150" << "3" << "4" << true;
        QTest::newRow("disjoint ranges") << "100-200" << "201-300" << "301-400" << "401-500" << false;
        QTest::newRow("reversed range") << "200-100" << "150" << "3" << "4" << true;
        QTest::newRow("empty fields") << "100-200" << "" << "" << "300" << false;
    }

    void headers()
    {
        QFETCH(QString, h1);
        QFETCH(QString, h2);
        QFETCH(QString, h3);
        QFETCH(QString, h4);
        QFETCH(bool, equal);

        QCOMPARE(AwgProtocolConfig::isHeadersEqual(h1, h2, h3, h4), equal);
    }
};

QTEST_MAIN(TestAwgHeaders)
#include "testAwgHeaders.moc"

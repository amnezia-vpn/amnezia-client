#include <QFile>
#include <QTemporaryDir>
#include <QTest>

#ifdef AMNEZIA_HAS_QZIPWRITER
    #include <QBuffer>
    #include <QtCore/private/qzipreader_p.h>
#endif

#include "core/controllers/logsController.h"

class TestLogsController : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_dir;

    QString writeFile(const QString &name, const QByteArray &data)
    {
        const QString path = m_dir.filePath(name);
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            qFatal("Cannot create test file");
        }
        file.write(data);
        return path;
    }

private slots:
    void initTestCase()
    {
        QVERIFY(m_dir.isValid());
    }

    void noCut()
    {
        const QString path = writeFile("current.log", "line1\nline2\nline3\n");
        QCOMPARE(LogsController::tailOfFiles({ path }, 1000), QByteArray("line1\nline2\nline3\n"));
    }

    void cutMidLine()
    {
        const QString path = writeFile("current.log", "line1\nline2\nline3\n");
        QCOMPARE(LogsController::tailOfFiles({ path }, 8), QByteArray("line3\n"));
    }

    void cutAtLineBoundary()
    {
        const QString path = writeFile("current.log", "line1\nline2\nline3\n");
        QCOMPARE(LogsController::tailOfFiles({ path }, 6), QByteArray("line3\n"));
        QCOMPARE(LogsController::tailOfFiles({ path }, 12), QByteArray("line2\nline3\n"));
    }

    void spanningTwoFiles()
    {
        const QString rotated = writeFile("rotated.log", "old1\nold2\n");
        const QString current = writeFile("current.log", "new1\nnew2\n");

        QCOMPARE(LogsController::tailOfFiles({ rotated, current }, 1000),
                 QByteArray("----- rotated.log -----\nold1\nold2\n"
                            "----- current.log -----\nnew1\nnew2\n"));

        QCOMPARE(LogsController::tailOfFiles({ rotated, current }, 15),
                 QByteArray("----- rotated.log -----\nold2\n"
                            "----- current.log -----\nnew1\nnew2\n"));

        QCOMPARE(LogsController::tailOfFiles({ rotated, current }, 7), QByteArray("new2\n"));
    }

    void cutWithoutNewline()
    {
        const QString path = writeFile("current.log", "\xE2\x82\xAC\xE2\x82\xAC\xE2\x82\xAC");
        QCOMPARE(LogsController::tailOfFiles({ path }, 3), QByteArray("\xE2\x82\xAC"));
        QCOMPARE(LogsController::tailOfFiles({ path }, 4), QByteArray("\xE2\x82\xAC"));
        QCOMPARE(LogsController::tailOfFiles({ path }, 5), QByteArray("\xE2\x82\xAC"));
        QCOMPARE(LogsController::tailOfFiles({ path }, 7), QByteArray("\xE2\x82\xAC\xE2\x82\xAC"));

        const QString ascii = writeFile("ascii.log", "abcdef");
        QCOMPARE(LogsController::tailOfFiles({ ascii }, 3), QByteArray("def"));
    }

    void newestFileEnd()
    {
        const QString rotated = writeFile("rotated.log", "old1\nold2\n");
        const QString current = writeFile("current.log", "new1\nnew2");
        qint64 end = -1;

        QCOMPARE(LogsController::tailOfFiles({ rotated, current }, 1000, &end),
                 QByteArray("----- rotated.log -----\nold1\nold2\n"
                            "----- current.log -----\nnew1\nnew2\n"));
        QCOMPARE(end, 9);

        QCOMPARE(LogsController::tailOfFiles({ rotated, current }, 6, &end), QByteArray("new2"));
        QCOMPARE(end, 9);

        end = -1;
        QCOMPARE(LogsController::tailOfFiles({ current, m_dir.filePath("missing.log") }, 1000, &end),
                 QByteArray("new1\nnew2"));
        QCOMPARE(end, 0);
    }

    void emptyFile()
    {
        const QString empty = writeFile("empty.log", "");
        const QString current = writeFile("current.log", "new1\n");

        QCOMPARE(LogsController::tailOfFiles({ empty }, 1000), QByteArray());
        QCOMPARE(LogsController::tailOfFiles({ empty, current }, 1000), QByteArray("new1\n"));
    }

    void missingFile()
    {
        const QString missing = m_dir.filePath("missing.log");
        const QString current = writeFile("current.log", "new1\n");

        QCOMPARE(LogsController::tailOfFiles({ missing }, 1000), QByteArray());
        QCOMPARE(LogsController::tailOfFiles({ missing, current }, 1000), QByteArray("new1\n"));
    }

    void zipFiles()
    {
#ifdef AMNEZIA_HAS_QZIPWRITER
        const QByteArray app = "app line\n";
        const QByteArray tunnel(100000, 'x');

        QByteArray zip =
            LogsController::zipFiles({ { QStringLiteral("app.log"), app }, { QStringLiteral("tunnel.log"), tunnel } });
        QBuffer buffer(&zip);
        QZipReader reader(&buffer);
        QCOMPARE(reader.status(), QZipReader::NoError);

        const QList<QZipReader::FileInfo> entries = reader.fileInfoList();
        QCOMPARE(entries.size(), 2);
        QCOMPARE(entries.at(0).filePath, QStringLiteral("app.log"));
        QCOMPARE(entries.at(1).filePath, QStringLiteral("tunnel.log"));
        QCOMPARE(reader.fileData(QStringLiteral("app.log")), app);
        QCOMPARE(reader.fileData(QStringLiteral("tunnel.log")), tunnel);
#else
        QSKIP("QZipReader needs Qt private headers");
#endif
    }
};

QTEST_MAIN(TestLogsController)
#include "testLogsController.moc"

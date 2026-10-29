#include <QtTest>

#include "ui/models/languageModel.h"

class TestableLanguageModel : public LanguageModel
{
public:
    using LanguageModel::roleNames;
};

class LanguageModelTest : public QObject
{
    Q_OBJECT

private slots:
    void rowCount_returnsNumberOfAvailableLanguages();
    void rowCount_returnsCurrentCountForValidParent();
    void data_returnsLanguageName();
    void data_returnsLanguageIndex();
    void data_returnsInvalidVariantForInvalidIndex();
    void data_returnsInvalidVariantForUnsupportedRole();
    void roleNames_returnsExpectedNames();
};

void LanguageModelTest::rowCount_returnsNumberOfAvailableLanguages()
{
    LanguageModel model;
    QCOMPARE(model.rowCount(), 14);
}

void LanguageModelTest::rowCount_returnsCurrentCountForValidParent()
{
    LanguageModel model;

    const QModelIndex parent = model.index(0, 0);
    QVERIFY(parent.isValid());
    QCOMPARE(model.rowCount(parent), model.rowCount());
}

void LanguageModelTest::data_returnsLanguageName()
{
    LanguageModel model;

    const QStringList expectedNames = {
        QStringLiteral("English"),
        QStringLiteral("Русский"),
        QStringLiteral("简体中文"),
        QStringLiteral("Українська"),
        QStringLiteral("فارسی"),
        QStringLiteral("العربية"),
        QStringLiteral("မြန်မာဘာသာ"),
        QStringLiteral("اُرْدُوْ"),
        QStringLiteral("हिन्दी"),
        QStringLiteral("Español"),
        QStringLiteral("한국어"),
        QStringLiteral("Français"),
        QStringLiteral("ไทย"),
        QStringLiteral("Bahasa Indonesia")
    };

    QCOMPARE(model.rowCount(), expectedNames.size());

    for (int row = 0; row < expectedNames.size(); ++row) {
        const QModelIndex index = model.index(row, 0);

        QVERIFY(index.isValid());
        QCOMPARE(model.data(index, LanguageModel::NameRole).toString(), expectedNames.at(row));
    }
}

void LanguageModelTest::data_returnsLanguageIndex()
{
    LanguageModel model;

    for (int row = 0; row < model.rowCount(); ++row) {
        const QModelIndex index = model.index(row, 0);

        QVERIFY(index.isValid());
        QCOMPARE(model.data(index, LanguageModel::IndexRole).toInt(), row);
    }
}

void LanguageModelTest::data_returnsInvalidVariantForInvalidIndex()
{
    LanguageModel model;

    QVERIFY(!model.data(QModelIndex(), LanguageModel::NameRole).isValid());
    QVERIFY(!model.data(QModelIndex(), LanguageModel::IndexRole).isValid());

    QVERIFY(!model.data(model.index(-1, 0), LanguageModel::NameRole).isValid());
    QVERIFY(!model.data(model.index(model.rowCount(), 0), LanguageModel::NameRole).isValid());
}

void LanguageModelTest::data_returnsInvalidVariantForUnsupportedRole()
{
    LanguageModel model;

    const QModelIndex index = model.index(0, 0);
    QVERIFY(index.isValid());

    QVERIFY(!model.data(index, Qt::DisplayRole).isValid());
    QVERIFY(!model.data(index, Qt::UserRole + 100).isValid());
}

void LanguageModelTest::roleNames_returnsExpectedNames()
{
    TestableLanguageModel model;

    const auto roles = model.roleNames();

    QCOMPARE(roles.value(TestableLanguageModel::NameRole), QByteArray("languageName"));
    QCOMPARE(roles.value(TestableLanguageModel::IndexRole), QByteArray("languageIndex"));
}

QTEST_GUILESS_MAIN(LanguageModelTest)
#include "testLanguageModel.moc"

#include <QString>
#include <QtTest>

class SmokeQtTest : public QObject
{
    Q_OBJECT

private slots:
    void testHarnessRuns();
};

void SmokeQtTest::testHarnessRuns()
{
    QCOMPARE(QStringLiteral("Scribe").toUpper(), QStringLiteral("SCRIBE"));
}

QTEST_APPLESS_MAIN(SmokeQtTest)

#include "SmokeQtTest.moc"

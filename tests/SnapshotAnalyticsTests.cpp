#include "core/SnapshotAnalytics.h"

#include <QtTest/QTest>

using namespace llm;

class SnapshotAnalyticsTests : public QObject {
    Q_OBJECT

private slots:
    void computesSpendAndTopUps();
};

void SnapshotAnalyticsTests::computesSpendAndTopUps() {
    QList<BalanceSnapshot> snapshots = {
        {QStringLiteral("provider-1"), QDateTime(QDate(2026, 3, 1), QTime(0, 0), Qt::UTC), 100.0, QStringLiteral("USD")},
        {QStringLiteral("provider-1"), QDateTime(QDate(2026, 3, 2), QTime(0, 0), Qt::UTC), 95.0, QStringLiteral("USD")},
        {QStringLiteral("provider-1"), QDateTime(QDate(2026, 3, 3), QTime(0, 0), Qt::UTC), 97.0, QStringLiteral("USD")},
        {QStringLiteral("provider-1"), QDateTime(QDate(2026, 3, 4), QTime(0, 0), Qt::UTC), 90.0, QStringLiteral("USD")}
    };

    const SnapshotComputation computation = computeUsageFromSnapshots(
        snapshots,
        UsageRange::CurrentMonth,
        QDateTime(QDate(2026, 3, 27), QTime(12, 0), Qt::UTC));

    QCOMPARE(computation.currency, QStringLiteral("USD"));
    QCOMPARE(computation.totalSpent, 12.0);
    QCOMPARE(computation.alerts.size(), 1);
    QVERIFY(computation.alerts.first().contains(QStringLiteral("Top-up detected")));

    bool sawSpendDay = false;
    bool sawTopUpDay = false;
    for (const UsagePoint& point : computation.usagePoints) {
        if (point.bucketStart.date() == QDate(2026, 3, 2)) {
            QCOMPARE(point.cost, 5.0);
            sawSpendDay = true;
        }
        if (point.bucketStart.date() == QDate(2026, 3, 3)) {
            QVERIFY(point.topUpEvent);
            QCOMPARE(point.topUpAmount, 2.0);
            sawTopUpDay = true;
        }
    }

    QVERIFY(sawSpendDay);
    QVERIFY(sawTopUpDay);
}

QTEST_APPLESS_MAIN(SnapshotAnalyticsTests)

#include "SnapshotAnalyticsTests.moc"

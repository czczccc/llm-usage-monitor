#include "SnapshotAnalytics.h"

#include <QMap>
#include <QTime>

#include <algorithm>

namespace llm {

SnapshotComputation computeUsageFromSnapshots(const QList<BalanceSnapshot>& snapshots,
                                             UsageRange range,
                                             const QDateTime& now) {
    SnapshotComputation computation;
    if (snapshots.isEmpty()) {
        return computation;
    }

    QList<BalanceSnapshot> ordered = snapshots;
    std::sort(ordered.begin(), ordered.end(), [](const BalanceSnapshot& left, const BalanceSnapshot& right) {
        return left.capturedAt < right.capturedAt;
    });

    computation.currency = ordered.first().currency;

    const DateRange targetRange = dateRangeFor(range, now);
    const QDate startDate = targetRange.start.date();
    const QDate endDateExclusive = targetRange.end.date();

    QMap<QDate, UsagePoint> byDate;
    for (QDate date = startDate; date < endDateExclusive; date = date.addDays(1)) {
        UsagePoint point;
        point.bucketStart = QDateTime(date, QTime(0, 0), Qt::UTC);
        point.bucketEnd = point.bucketStart.addDays(1);
        point.currency = computation.currency;
        point.dataSource = DataSource::Snapshot;
        byDate.insert(date, point);
    }

    constexpr double epsilon = 0.0001;

    for (int index = 1; index < ordered.size(); ++index) {
        const BalanceSnapshot& previous = ordered.at(index - 1);
        const BalanceSnapshot& current = ordered.at(index);

        const QDate bucketDate = current.capturedAt.date();
        if (bucketDate < startDate || bucketDate >= endDateExclusive) {
            continue;
        }

        UsagePoint point = byDate.value(bucketDate);
        const double delta = current.balance - previous.balance;

        if (delta < -epsilon) {
            point.cost += -delta;
            computation.totalSpent += -delta;
        } else if (delta > epsilon) {
            point.topUpEvent = true;
            point.topUpAmount += delta;
            point.note = QStringLiteral("Top-up +%1 %2").arg(delta, 0, 'f', 2).arg(current.currency);
            computation.alerts << QStringLiteral("Top-up detected on %1: +%2 %3")
                                      .arg(bucketDate.toString(Qt::ISODate))
                                      .arg(delta, 0, 'f', 2)
                                      .arg(current.currency);
        }

        byDate.insert(bucketDate, point);
    }

    computation.usagePoints = byDate.values();
    return computation;
}

}  // namespace llm

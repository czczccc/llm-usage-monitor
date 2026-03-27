#pragma once

#include "Models.h"

namespace llm {

struct SnapshotComputation {
    QList<UsagePoint> usagePoints;
    double totalSpent = 0.0;
    QString currency = QStringLiteral("USD");
    QStringList alerts;
};

SnapshotComputation computeUsageFromSnapshots(const QList<BalanceSnapshot>& snapshots,
                                             UsageRange range,
                                             const QDateTime& now = QDateTime::currentDateTimeUtc());

}  // namespace llm

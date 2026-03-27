#pragma once

#include "core/Models.h"

namespace llm {

class SnapshotRepository {
public:
    SnapshotRepository();

    bool initialize(QString* error = nullptr) const;

    bool insertBalanceSnapshot(const BalanceSnapshot& snapshot, QString* error = nullptr) const;
    QList<BalanceSnapshot> loadBalanceSnapshots(const QString& providerConfigId,
                                               const QDateTime& start,
                                               const QDateTime& end) const;

    bool addSyncLog(const SyncLogEntry& logEntry, QString* error = nullptr) const;
    QList<SyncLogEntry> loadRecentSyncLogs(const QString& providerConfigId, int limit = 20) const;

    QString databasePath() const;

private:
    bool ensureParentDirectory() const;
    QString connectionName() const;
    QString isoString(const QDateTime& dateTime) const;
};

}  // namespace llm

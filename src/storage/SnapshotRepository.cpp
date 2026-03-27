#include "SnapshotRepository.h"

#include <QDir>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QUuid>

namespace llm {

SnapshotRepository::SnapshotRepository() = default;

QString SnapshotRepository::databasePath() const {
    const QString basePath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    return QDir(basePath).filePath(QStringLiteral("monitor.db"));
}

bool SnapshotRepository::ensureParentDirectory() const {
    const QFileInfo info(databasePath());
    return info.dir().mkpath(QStringLiteral("."));
}

QString SnapshotRepository::connectionName() const {
    return QStringLiteral("repo-%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
}

QString SnapshotRepository::isoString(const QDateTime& dateTime) const {
    return dateTime.toUTC().toString(Qt::ISODateWithMs);
}

bool SnapshotRepository::initialize(QString* error) const {
    if (!ensureParentDirectory()) {
        if (error) {
            *error = QStringLiteral("Failed to create application data directory.");
        }
        return false;
    }

    const QString name = connectionName();
    {
        QSqlDatabase database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), name);
        database.setDatabaseName(databasePath());
        if (!database.open()) {
            if (error) {
                *error = database.lastError().text();
            }
            database = QSqlDatabase();
            QSqlDatabase::removeDatabase(name);
            return false;
        }

        QSqlQuery query(database);
        if (!query.exec(QStringLiteral(
                "CREATE TABLE IF NOT EXISTS balance_snapshots ("
                "provider_config_id TEXT NOT NULL,"
                "captured_at TEXT NOT NULL,"
                "balance REAL NOT NULL,"
                "currency TEXT NOT NULL,"
                "PRIMARY KEY(provider_config_id, captured_at))"))) {
            if (error) {
                *error = query.lastError().text();
            }
            database.close();
            database = QSqlDatabase();
            QSqlDatabase::removeDatabase(name);
            return false;
        }

        if (!query.exec(QStringLiteral(
                "CREATE TABLE IF NOT EXISTS sync_logs ("
                "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                "provider_config_id TEXT NOT NULL,"
                "created_at TEXT NOT NULL,"
                "level TEXT NOT NULL,"
                "message TEXT NOT NULL,"
                "raw_response TEXT)"))) {
            if (error) {
                *error = query.lastError().text();
            }
            database.close();
            database = QSqlDatabase();
            QSqlDatabase::removeDatabase(name);
            return false;
        }

        database.close();
        database = QSqlDatabase();
    }
    QSqlDatabase::removeDatabase(name);
    return true;
}

bool SnapshotRepository::insertBalanceSnapshot(const BalanceSnapshot& snapshot, QString* error) const {
    const QString name = connectionName();
    bool ok = false;
    {
        QSqlDatabase database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), name);
        database.setDatabaseName(databasePath());
        if (!database.open()) {
            if (error) {
                *error = database.lastError().text();
            }
            database = QSqlDatabase();
            QSqlDatabase::removeDatabase(name);
            return false;
        }

        QSqlQuery query(database);
        query.prepare(QStringLiteral(
            "INSERT OR REPLACE INTO balance_snapshots(provider_config_id, captured_at, balance, currency) "
            "VALUES(:provider_config_id, :captured_at, :balance, :currency)"));
        query.bindValue(QStringLiteral(":provider_config_id"), snapshot.providerConfigId);
        query.bindValue(QStringLiteral(":captured_at"), isoString(snapshot.capturedAt));
        query.bindValue(QStringLiteral(":balance"), snapshot.balance);
        query.bindValue(QStringLiteral(":currency"), snapshot.currency);
        ok = query.exec();
        if (!ok && error) {
            *error = query.lastError().text();
        }
        database.close();
        database = QSqlDatabase();
    }
    QSqlDatabase::removeDatabase(name);
    return ok;
}

QList<BalanceSnapshot> SnapshotRepository::loadBalanceSnapshots(const QString& providerConfigId,
                                                               const QDateTime& start,
                                                               const QDateTime& end) const {
    QList<BalanceSnapshot> snapshots;
    const QString name = connectionName();
    {
        QSqlDatabase database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), name);
        database.setDatabaseName(databasePath());
        if (!database.open()) {
            database = QSqlDatabase();
            QSqlDatabase::removeDatabase(name);
            return snapshots;
        }

        QSqlQuery query(database);
        query.prepare(QStringLiteral(
            "SELECT provider_config_id, captured_at, balance, currency "
            "FROM balance_snapshots "
            "WHERE provider_config_id = :provider_config_id "
            "AND captured_at >= :start "
            "AND captured_at <= :end "
            "ORDER BY captured_at ASC"));
        query.bindValue(QStringLiteral(":provider_config_id"), providerConfigId);
        query.bindValue(QStringLiteral(":start"), isoString(start));
        query.bindValue(QStringLiteral(":end"), isoString(end));
        if (query.exec()) {
            while (query.next()) {
                BalanceSnapshot snapshot;
                snapshot.providerConfigId = query.value(0).toString();
                snapshot.capturedAt = QDateTime::fromString(query.value(1).toString(), Qt::ISODateWithMs);
                snapshot.balance = query.value(2).toDouble();
                snapshot.currency = query.value(3).toString();
                snapshots << snapshot;
            }
        }
        database.close();
        database = QSqlDatabase();
    }
    QSqlDatabase::removeDatabase(name);
    return snapshots;
}

bool SnapshotRepository::addSyncLog(const SyncLogEntry& logEntry, QString* error) const {
    const QString name = connectionName();
    bool ok = false;
    {
        QSqlDatabase database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), name);
        database.setDatabaseName(databasePath());
        if (!database.open()) {
            if (error) {
                *error = database.lastError().text();
            }
            database = QSqlDatabase();
            QSqlDatabase::removeDatabase(name);
            return false;
        }

        QSqlQuery query(database);
        query.prepare(QStringLiteral(
            "INSERT INTO sync_logs(provider_config_id, created_at, level, message, raw_response) "
            "VALUES(:provider_config_id, :created_at, :level, :message, :raw_response)"));
        query.bindValue(QStringLiteral(":provider_config_id"), logEntry.providerConfigId);
        query.bindValue(QStringLiteral(":created_at"), isoString(logEntry.createdAt));
        query.bindValue(QStringLiteral(":level"), logEntry.level);
        query.bindValue(QStringLiteral(":message"), logEntry.message);
        query.bindValue(QStringLiteral(":raw_response"), logEntry.rawResponse);
        ok = query.exec();
        if (!ok && error) {
            *error = query.lastError().text();
        }
        database.close();
        database = QSqlDatabase();
    }
    QSqlDatabase::removeDatabase(name);
    return ok;
}

QList<SyncLogEntry> SnapshotRepository::loadRecentSyncLogs(const QString& providerConfigId, int limit) const {
    QList<SyncLogEntry> logs;
    const QString name = connectionName();
    {
        QSqlDatabase database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), name);
        database.setDatabaseName(databasePath());
        if (!database.open()) {
            database = QSqlDatabase();
            QSqlDatabase::removeDatabase(name);
            return logs;
        }

        QSqlQuery query(database);
        query.prepare(QStringLiteral(
            "SELECT id, provider_config_id, created_at, level, message, raw_response "
            "FROM sync_logs "
            "WHERE provider_config_id = :provider_config_id "
            "ORDER BY created_at DESC "
            "LIMIT :limit"));
        query.bindValue(QStringLiteral(":provider_config_id"), providerConfigId);
        query.bindValue(QStringLiteral(":limit"), limit);
        if (query.exec()) {
            while (query.next()) {
                SyncLogEntry entry;
                entry.id = query.value(0).toLongLong();
                entry.providerConfigId = query.value(1).toString();
                entry.createdAt = QDateTime::fromString(query.value(2).toString(), Qt::ISODateWithMs);
                entry.level = query.value(3).toString();
                entry.message = query.value(4).toString();
                entry.rawResponse = query.value(5).toString();
                logs << entry;
            }
        }
        database.close();
        database = QSqlDatabase();
    }
    QSqlDatabase::removeDatabase(name);
    return logs;
}

}  // namespace llm

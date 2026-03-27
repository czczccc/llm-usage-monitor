#include "MonitorService.h"

#include <QDate>
#include <QUuid>

namespace {

llm::AuthStatus statusFromHttp(int statusCode) {
    if (statusCode == 401 || statusCode == 403) {
        return llm::AuthStatus::Unauthorized;
    }
    if (statusCode >= 400) {
        return llm::AuthStatus::Invalid;
    }
    return llm::AuthStatus::Valid;
}

}  // namespace

namespace llm {

MonitorService::MonitorService()
    : m_settings(),
      m_credentialStore(std::make_unique<WindowsCredentialStore>()),
      m_httpClient(30000) {
    m_configs = m_settings.loadProviderConfigs();
    QString error;
    m_repository.initialize(&error);
}

QList<ProviderConfig> MonitorService::providerConfigs() const {
    return m_configs;
}

ProviderConfig MonitorService::addProvider(ProviderId providerId, const QString& displayName) {
    ProviderConfig config;
    config.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    config.providerId = providerId;
    config.displayName = displayName.trimmed().isEmpty() ? providerDisplayName(providerId) : displayName.trimmed();
    config.refreshIntervalMinutes = 30;
    config.credentialTarget = defaultCredentialTarget(config);
    m_configs << config;
    persistConfigs();
    return config;
}

bool MonitorService::saveProviderConfig(const ProviderConfig& config, const QString& apiKey, QString* error) {
    ProviderConfig stored = config;
    if (stored.id.trimmed().isEmpty()) {
        stored.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    if (stored.credentialTarget.trimmed().isEmpty()) {
        stored.credentialTarget = defaultCredentialTarget(stored);
    }

    bool found = false;
    for (ProviderConfig& existing : m_configs) {
        if (existing.id == stored.id) {
            existing = stored;
            found = true;
            break;
        }
    }
    if (!found) {
        m_configs << stored;
    }

    if (!apiKey.trimmed().isEmpty() && !m_credentialStore->writeSecret(stored.credentialTarget, apiKey.trimmed(), error)) {
        return false;
    }

    persistConfigs();
    return true;
}

bool MonitorService::removeProvider(const QString& providerConfigId, QString* error) {
    for (int index = 0; index < m_configs.size(); ++index) {
        if (m_configs.at(index).id != providerConfigId) {
            continue;
        }
        const ProviderConfig config = m_configs.takeAt(index);
        if (!config.credentialTarget.isEmpty()) {
            QString deleteError;
            m_credentialStore->deleteSecret(config.credentialTarget, &deleteError);
            if (!deleteError.isEmpty() && error && error->isEmpty()) {
                *error = deleteError;
            }
        }
        m_cachedOverviews.remove(providerConfigId);
        persistConfigs();
        return true;
    }

    if (error) {
        *error = QStringLiteral("Provider not found.");
    }
    return false;
}

ProviderConfig* MonitorService::findConfigMutable(const QString& providerConfigId) {
    for (ProviderConfig& config : m_configs) {
        if (config.id == providerConfigId) {
            return &config;
        }
    }
    return nullptr;
}

const ProviderConfig* MonitorService::findConfig(const QString& providerConfigId) const {
    for (const ProviderConfig& config : m_configs) {
        if (config.id == providerConfigId) {
            return &config;
        }
    }
    return nullptr;
}

QString MonitorService::apiKeyFor(const ProviderConfig& config, QString* error) const {
    if (config.credentialTarget.trimmed().isEmpty()) {
        if (error) {
            *error = QStringLiteral("Credential target is not configured.");
        }
        return {};
    }
    return m_credentialStore->readSecret(config.credentialTarget, error);
}

void MonitorService::persistConfigs() {
    m_settings.saveProviderConfigs(m_configs);
}

void MonitorService::writeLog(const QString& providerConfigId, const SyncResult& result, const QString& level) const {
    SyncLogEntry entry;
    entry.providerConfigId = providerConfigId;
    entry.createdAt = result.completedAt.isValid() ? result.completedAt : QDateTime::currentDateTimeUtc();
    entry.level = level.isEmpty() ? (result.success ? QStringLiteral("info") : QStringLiteral("error")) : level;
    entry.message = result.message;
    entry.rawResponse = result.rawResponse.left(8000);
    m_repository.addSyncLog(entry);
}

void MonitorService::cacheOverview(const ProviderConfig& config, const AccountOverview& overview) {
    m_cachedOverviews.insert(config.id, overview);
}

AccountOverview MonitorService::enrichDeepSeekOverview(const ProviderConfig& config,
                                                      const AccountOverview& overview,
                                                      const QDateTime& now) {
    AccountOverview enriched = overview;
    const DateRange dateRange = dateRangeFor(UsageRange::CurrentMonth, now);
    const QList<BalanceSnapshot> snapshots = m_repository.loadBalanceSnapshots(
        config.id,
        dateRange.start.addDays(-2),
        dateRange.end);

    const SnapshotComputation computation = computeUsageFromSnapshots(snapshots, UsageRange::CurrentMonth, now);
    enriched.monthlyUsed = computation.totalSpent;
    enriched.estimatedRemaining = enriched.currentBalance;
    if (computation.totalSpent > 0.0 || !computation.alerts.isEmpty()) {
        enriched.dataSource = DataSource::Snapshot;
    }
    enriched.alerts << computation.alerts;
    enriched.statusSummary = QStringLiteral("DeepSeek balance is official; usage is derived from local snapshots.");
    return enriched;
}

SyncResult MonitorService::testConnection(const QString& providerConfigId) {
    SyncResult result;
    const ProviderConfig* config = findConfig(providerConfigId);
    if (!config) {
        result.message = QStringLiteral("Provider not found.");
        result.completedAt = QDateTime::currentDateTimeUtc();
        return result;
    }

    QString credentialError;
    const QString apiKey = apiKeyFor(*config, &credentialError);
    if (apiKey.isEmpty()) {
        result.message = credentialError.isEmpty() ? QStringLiteral("API key is not stored yet.") : credentialError;
        result.completedAt = QDateTime::currentDateTimeUtc();
        writeLog(config->id, result);
        return result;
    }

    auto adapter = createProviderAdapter(config->providerId, m_httpClient);
    result = adapter->validateCredentials(*config, apiKey);
    if (result.success && result.overview.has_value()) {
        cacheOverview(*config, result.overview.value());
    } else if (!result.success) {
        AccountOverview overview;
        overview.authStatus = statusFromHttp(result.statusCode);
        overview.lastSyncAt = result.completedAt;
        overview.dataSource = DataSource::Unavailable;
        overview.statusSummary = result.message;
        m_cachedOverviews.insert(config->id, overview);
    }
    writeLog(config->id, result);
    return result;
}

SyncResult MonitorService::refreshProvider(const QString& providerConfigId) {
    SyncResult result;
    ProviderConfig* config = findConfigMutable(providerConfigId);
    if (!config) {
        result.message = QStringLiteral("Provider not found.");
        result.completedAt = QDateTime::currentDateTimeUtc();
        return result;
    }

    QString credentialError;
    const QString apiKey = apiKeyFor(*config, &credentialError);
    if (apiKey.isEmpty()) {
        result.message = credentialError.isEmpty() ? QStringLiteral("API key is not stored yet.") : credentialError;
        result.completedAt = QDateTime::currentDateTimeUtc();
        writeLog(config->id, result);
        return result;
    }

    auto adapter = createProviderAdapter(config->providerId, m_httpClient);
    result = adapter->fetchOverview(*config, apiKey);

    if (result.success && result.overview.has_value()) {
        AccountOverview overview = result.overview.value();
        if (config->providerId == ProviderId::DeepSeek && overview.currentBalance.has_value()) {
            BalanceSnapshot snapshot;
            snapshot.providerConfigId = config->id;
            snapshot.capturedAt = result.completedAt;
            snapshot.balance = overview.currentBalance.value();
            snapshot.currency = overview.currency;
            m_repository.insertBalanceSnapshot(snapshot);
            overview = enrichDeepSeekOverview(*config, overview, result.completedAt);
            result.overview = overview;
        }
        cacheOverview(*config, overview);
    } else {
        AccountOverview overview;
        overview.authStatus = statusFromHttp(result.statusCode);
        overview.lastSyncAt = result.completedAt;
        overview.dataSource = DataSource::Unavailable;
        overview.statusSummary = result.message;
        cacheOverview(*config, overview);
    }

    config->lastAutoRefreshAt = result.completedAt;
    persistConfigs();
    writeLog(config->id, result);
    return result;
}

SyncResult MonitorService::loadDeepSeekUsage(const ProviderConfig& config, UsageRange range, const QDateTime& now) const {
    SyncResult result;
    result.completedAt = now;

    const DateRange dateRange = dateRangeFor(range, now);
    const QList<BalanceSnapshot> snapshots = m_repository.loadBalanceSnapshots(
        config.id,
        dateRange.start.addDays(-2),
        dateRange.end);

    const SnapshotComputation computation = computeUsageFromSnapshots(snapshots, range, now);
    result.success = true;
    result.usagePoints = computation.usagePoints;
    result.message = snapshots.size() >= 2
        ? QStringLiteral("DeepSeek usage derived from local balance snapshots.")
        : QStringLiteral("Need at least two DeepSeek balance snapshots to compute usage.");
    result.infoMessages = computation.alerts;
    return result;
}

SyncResult MonitorService::loadUsage(const QString& providerConfigId, UsageRange range) {
    SyncResult result;
    const ProviderConfig* config = findConfig(providerConfigId);
    if (!config) {
        result.message = QStringLiteral("Provider not found.");
        result.completedAt = QDateTime::currentDateTimeUtc();
        return result;
    }

    if (config->providerId == ProviderId::DeepSeek) {
        result = loadDeepSeekUsage(*config, range, QDateTime::currentDateTimeUtc());
        writeLog(config->id, result, QStringLiteral("info"));
        return result;
    }

    QString credentialError;
    const QString apiKey = apiKeyFor(*config, &credentialError);
    if (apiKey.isEmpty()) {
        result.message = credentialError.isEmpty() ? QStringLiteral("API key is not stored yet.") : credentialError;
        result.completedAt = QDateTime::currentDateTimeUtc();
        writeLog(config->id, result);
        return result;
    }

    auto adapter = createProviderAdapter(config->providerId, m_httpClient);
    result = adapter->fetchUsage(*config, apiKey, range);
    writeLog(config->id, result, result.success ? QStringLiteral("info") : QStringLiteral("warn"));
    return result;
}

QList<SyncLogEntry> MonitorService::recentLogs(const QString& providerConfigId, int limit) const {
    return m_repository.loadRecentSyncLogs(providerConfigId, limit);
}

std::optional<AccountOverview> MonitorService::cachedOverview(const QString& providerConfigId) const {
    if (!m_cachedOverviews.contains(providerConfigId)) {
        return std::nullopt;
    }
    return m_cachedOverviews.value(providerConfigId);
}

void MonitorService::refreshDueProviders() {
    const QDateTime now = QDateTime::currentDateTimeUtc();
    for (const ProviderConfig& config : m_configs) {
        if (config.refreshIntervalMinutes <= 0) {
            continue;
        }
        if (config.lastAutoRefreshAt.isValid() &&
            config.lastAutoRefreshAt.secsTo(now) < config.refreshIntervalMinutes * 60) {
            continue;
        }
        refreshProvider(config.id);
    }
}

QString MonitorService::databasePath() const {
    return m_repository.databasePath();
}

QByteArray MonitorService::loadWindowGeometry() const {
    return m_settings.loadWindowGeometry();
}

void MonitorService::saveWindowGeometry(const QByteArray& geometry) {
    m_settings.saveWindowGeometry(geometry);
}

QByteArray MonitorService::loadSplitterState() const {
    return m_settings.loadSplitterState();
}

void MonitorService::saveSplitterState(const QByteArray& state) {
    m_settings.saveSplitterState(state);
}

}  // namespace llm

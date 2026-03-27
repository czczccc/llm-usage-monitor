#pragma once

#include "ProviderFactory.h"
#include "SnapshotAnalytics.h"
#include "storage/CredentialStore.h"
#include "storage/SettingsStore.h"
#include "storage/SnapshotRepository.h"

#include <QHash>

namespace llm {

class MonitorService {
public:
    MonitorService();

    QList<ProviderConfig> providerConfigs() const;
    ProviderConfig addProvider(ProviderId providerId, const QString& displayName);
    bool saveProviderConfig(const ProviderConfig& config, const QString& apiKey, QString* error = nullptr);
    bool removeProvider(const QString& providerConfigId, QString* error = nullptr);

    SyncResult testConnection(const QString& providerConfigId);
    SyncResult refreshProvider(const QString& providerConfigId);
    SyncResult loadUsage(const QString& providerConfigId, UsageRange range);
    QList<SyncLogEntry> recentLogs(const QString& providerConfigId, int limit = 20) const;

    std::optional<AccountOverview> cachedOverview(const QString& providerConfigId) const;
    void refreshDueProviders();

    QString databasePath() const;
    QByteArray loadWindowGeometry() const;
    void saveWindowGeometry(const QByteArray& geometry);
    QByteArray loadSplitterState() const;
    void saveSplitterState(const QByteArray& state);

private:
    ProviderConfig* findConfigMutable(const QString& providerConfigId);
    const ProviderConfig* findConfig(const QString& providerConfigId) const;
    QString apiKeyFor(const ProviderConfig& config, QString* error) const;
    void persistConfigs();
    void writeLog(const QString& providerConfigId, const SyncResult& result, const QString& level = QString()) const;
    void cacheOverview(const ProviderConfig& config, const AccountOverview& overview);
    AccountOverview enrichDeepSeekOverview(const ProviderConfig& config, const AccountOverview& overview, const QDateTime& now);
    SyncResult loadDeepSeekUsage(const ProviderConfig& config, UsageRange range, const QDateTime& now) const;

    QList<ProviderConfig> m_configs;
    SettingsStore m_settings;
    std::unique_ptr<ICredentialStore> m_credentialStore;
    SnapshotRepository m_repository;
    HttpJsonClient m_httpClient;
    QHash<QString, AccountOverview> m_cachedOverviews;
};

}  // namespace llm

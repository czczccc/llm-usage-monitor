#pragma once

#include <QDateTime>
#include <QFlags>
#include <QList>
#include <QString>
#include <QStringList>

#include <optional>

namespace llm {

enum class ProviderId {
    DeepSeek,
    OpenAI,
    Anthropic,
    Gemini
};

QString providerIdToString(ProviderId providerId);
QString providerDisplayName(ProviderId providerId);
ProviderId providerIdFromString(const QString& value);
QList<ProviderId> allProviderIds();

enum CapabilityFlag {
    CanValidateCredentials = 0x1,
    CanFetchOverview = 0x2,
    CanFetchUsage = 0x4,
    HasRealBalance = 0x8,
    UsesBudgetRemaining = 0x10,
    HasModelMetadata = 0x20
};
Q_DECLARE_FLAGS(CapabilityFlags, CapabilityFlag)
Q_DECLARE_OPERATORS_FOR_FLAGS(CapabilityFlags)

enum class DataSource {
    Official,
    Snapshot,
    Estimated,
    Unavailable
};

QString dataSourceToString(DataSource dataSource);

enum class AuthStatus {
    Unknown,
    Valid,
    Invalid,
    Unauthorized,
    Unsupported
};

QString authStatusToString(AuthStatus status);

enum class UsageRange {
    Last7Days,
    Last30Days,
    CurrentMonth
};

QString usageRangeToString(UsageRange range);

struct DateRange {
    QDateTime start;
    QDateTime end;
};

DateRange dateRangeFor(UsageRange range, const QDateTime& now = QDateTime::currentDateTimeUtc());

struct ProviderConfig {
    QString id;
    ProviderId providerId = ProviderId::DeepSeek;
    QString displayName;
    double monthlyBudget = 0.0;
    int refreshIntervalMinutes = 30;
    QString organizationId;
    QString projectId;
    QStringList modelFilters;
    QString credentialTarget;
    QDateTime lastAutoRefreshAt;
};

struct AccountOverview {
    AuthStatus authStatus = AuthStatus::Unknown;
    QDateTime lastSyncAt;
    std::optional<double> currentBalance;
    std::optional<double> monthlyUsed;
    std::optional<double> estimatedRemaining;
    QString currency = QStringLiteral("USD");
    DataSource dataSource = DataSource::Unavailable;
    QStringList alerts;
    QString statusSummary;
};

struct UsagePoint {
    QDateTime bucketStart;
    QDateTime bucketEnd;
    double cost = 0.0;
    QString currency = QStringLiteral("USD");
    DataSource dataSource = DataSource::Unavailable;
    std::optional<qint64> inputTokens;
    std::optional<qint64> outputTokens;
    std::optional<qint64> requestCount;
    bool topUpEvent = false;
    double topUpAmount = 0.0;
    QString note;
};

struct SyncResult {
    bool success = false;
    QString message;
    QString rawResponse;
    int statusCode = 0;
    QDateTime completedAt;
    std::optional<AccountOverview> overview;
    QList<UsagePoint> usagePoints;
    QStringList infoMessages;
};

struct BalanceSnapshot {
    QString providerConfigId;
    QDateTime capturedAt;
    double balance = 0.0;
    QString currency = QStringLiteral("USD");
};

struct SyncLogEntry {
    qint64 id = -1;
    QString providerConfigId;
    QDateTime createdAt;
    QString level = QStringLiteral("info");
    QString message;
    QString rawResponse;
};

}  // namespace llm

Q_DECLARE_METATYPE(llm::UsageRange)

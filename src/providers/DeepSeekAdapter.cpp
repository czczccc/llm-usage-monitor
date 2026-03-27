#include "DeepSeekAdapter.h"

#include "core/ProviderParsers.h"

namespace llm {

DeepSeekAdapter::DeepSeekAdapter(const HttpJsonClient& httpClient)
    : m_httpClient(httpClient) {}

ProviderId DeepSeekAdapter::providerId() const {
    return ProviderId::DeepSeek;
}

CapabilityFlags DeepSeekAdapter::capabilities() const {
    return CanValidateCredentials | CanFetchOverview | HasRealBalance;
}

SyncResult DeepSeekAdapter::validateCredentials(const ProviderConfig&, const QString& apiKey) {
    SyncResult result = requestBalance(apiKey);
    result.message = result.success
        ? QStringLiteral("DeepSeek API key is valid.")
        : QStringLiteral("Failed to validate DeepSeek API key.");
    return result;
}

SyncResult DeepSeekAdapter::fetchOverview(const ProviderConfig&, const QString& apiKey) {
    return requestBalance(apiKey);
}

SyncResult DeepSeekAdapter::fetchUsage(const ProviderConfig&, const QString&, UsageRange) {
    SyncResult result;
    result.success = false;
    result.completedAt = QDateTime::currentDateTimeUtc();
    result.message = QStringLiteral("DeepSeek usage history is derived from local balance snapshots.");
    return result;
}

SyncResult DeepSeekAdapter::requestBalance(const QString& apiKey) const {
    SyncResult result;
    result.completedAt = QDateTime::currentDateTimeUtc();

    if (apiKey.trimmed().isEmpty()) {
        result.message = QStringLiteral("DeepSeek API key is empty.");
        return result;
    }

    const QMap<QString, QString> headers = {
        {QStringLiteral("Authorization"), QStringLiteral("Bearer %1").arg(apiKey)}
    };
    const HttpResponse response = m_httpClient.get(QUrl(QStringLiteral("https://api.deepseek.com/user/balance")), headers);
    result.statusCode = response.statusCode;
    result.rawResponse = QString::fromUtf8(response.rawBody);

    if (!response.ok) {
        result.message = response.errorMessage.isEmpty() ? QStringLiteral("DeepSeek balance request failed.") : response.errorMessage;
        return result;
    }

    QString parseError;
    const auto parsed = ProviderParsers::parseDeepSeekBalance(response.json, &parseError);
    if (!parsed.has_value()) {
        result.message = parseError;
        return result;
    }

    AccountOverview overview;
    overview.authStatus = AuthStatus::Valid;
    overview.lastSyncAt = result.completedAt;
    overview.currentBalance = parsed->balance;
    overview.estimatedRemaining = parsed->balance;
    overview.currency = parsed->currency;
    overview.dataSource = DataSource::Official;
    overview.alerts = parsed->alerts;
    overview.statusSummary = QStringLiteral("Official DeepSeek balance fetched.");

    result.success = true;
    result.message = QStringLiteral("DeepSeek balance fetched successfully.");
    result.overview = overview;
    return result;
}

}  // namespace llm

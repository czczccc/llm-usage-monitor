#include "AnthropicAdapter.h"

#include "core/ProviderParsers.h"

namespace {

double sumCost(const QList<llm::UsagePoint>& points) {
    double total = 0.0;
    for (const llm::UsagePoint& point : points) {
        total += point.cost;
    }
    return total;
}

QMap<QString, QString> anthropicHeaders(const QString& apiKey) {
    return {
        {QStringLiteral("x-api-key"), apiKey},
        {QStringLiteral("anthropic-version"), QStringLiteral("2023-06-01")}
    };
}

}  // namespace

namespace llm {

AnthropicAdapter::AnthropicAdapter(const HttpJsonClient& httpClient)
    : m_httpClient(httpClient) {}

ProviderId AnthropicAdapter::providerId() const {
    return ProviderId::Anthropic;
}

CapabilityFlags AnthropicAdapter::capabilities() const {
    return CanValidateCredentials | CanFetchOverview | CanFetchUsage | UsesBudgetRemaining;
}

SyncResult AnthropicAdapter::validateCredentials(const ProviderConfig& config, const QString& apiKey) {
    SyncResult result = requestCosts(config, apiKey, UsageRange::Last7Days);
    result.message = result.success
        ? QStringLiteral("Anthropic admin key is valid for organization cost APIs.")
        : QStringLiteral("Failed to validate Anthropic admin key.");
    return result;
}

SyncResult AnthropicAdapter::fetchOverview(const ProviderConfig& config, const QString& apiKey) {
    SyncResult result = requestCosts(config, apiKey, UsageRange::CurrentMonth);
    if (!result.success) {
        return result;
    }

    AccountOverview overview;
    overview.authStatus = AuthStatus::Valid;
    overview.lastSyncAt = result.completedAt;
    overview.monthlyUsed = sumCost(result.usagePoints);
    overview.currency = QStringLiteral("USD");
    overview.dataSource = config.monthlyBudget > 0.0 ? DataSource::Estimated : DataSource::Official;
    overview.statusSummary = QStringLiteral("Official Anthropic cost data fetched.");
    if (config.monthlyBudget > 0.0 && overview.monthlyUsed.has_value()) {
        overview.estimatedRemaining = config.monthlyBudget - overview.monthlyUsed.value();
        overview.alerts << QStringLiteral("Remaining is derived from the configured monthly budget.");
    } else {
        overview.alerts << QStringLiteral("Add a monthly budget to estimate remaining spend.");
    }

    result.overview = overview;
    result.message = QStringLiteral("Anthropic overview fetched successfully.");
    return result;
}

SyncResult AnthropicAdapter::fetchUsage(const ProviderConfig& config, const QString& apiKey, UsageRange range) {
    return requestCosts(config, apiKey, range);
}

SyncResult AnthropicAdapter::requestCosts(const ProviderConfig& config, const QString& apiKey, UsageRange range) const {
    Q_UNUSED(config)
    SyncResult result;
    result.completedAt = QDateTime::currentDateTimeUtc();

    if (apiKey.trimmed().isEmpty()) {
        result.message = QStringLiteral("Anthropic key is empty.");
        return result;
    }

    const DateRange dateRange = dateRangeFor(range, result.completedAt);
    const QList<QPair<QString, QString>> queryItems = {
        {QStringLiteral("starting_at"), dateRange.start.toUTC().toString(Qt::ISODate)},
        {QStringLiteral("ending_at"), dateRange.end.toUTC().toString(Qt::ISODate)},
        {QStringLiteral("limit"), QStringLiteral("31")}
    };

    const HttpResponse response = m_httpClient.get(
        QUrl(QStringLiteral("https://api.anthropic.com/v1/organizations/cost_report")),
        anthropicHeaders(apiKey),
        queryItems);

    result.statusCode = response.statusCode;
    result.rawResponse = QString::fromUtf8(response.rawBody);
    if (!response.ok) {
        result.message = response.errorMessage.isEmpty() ? QStringLiteral("Anthropic cost request failed.") : response.errorMessage;
        return result;
    }

    QString parseError;
    result.usagePoints = ProviderParsers::parseAnthropicCosts(response.json, &parseError);
    if (result.usagePoints.isEmpty() && !parseError.isEmpty()) {
        result.message = parseError;
        return result;
    }

    result.success = true;
    result.message = QStringLiteral("Anthropic cost data fetched successfully.");
    return result;
}

}  // namespace llm

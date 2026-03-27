#include "OpenAiAdapter.h"

#include "core/ProviderParsers.h"

namespace {

double sumCost(const QList<llm::UsagePoint>& points) {
    double total = 0.0;
    for (const llm::UsagePoint& point : points) {
        total += point.cost;
    }
    return total;
}

}  // namespace

namespace llm {

OpenAiAdapter::OpenAiAdapter(const HttpJsonClient& httpClient)
    : m_httpClient(httpClient) {}

ProviderId OpenAiAdapter::providerId() const {
    return ProviderId::OpenAI;
}

CapabilityFlags OpenAiAdapter::capabilities() const {
    return CanValidateCredentials | CanFetchOverview | CanFetchUsage | UsesBudgetRemaining;
}

QMap<QString, QString> OpenAiAdapter::headersFor(const ProviderConfig& config, const QString& apiKey) const {
    QMap<QString, QString> headers = {
        {QStringLiteral("Authorization"), QStringLiteral("Bearer %1").arg(apiKey)}
    };
    if (!config.organizationId.trimmed().isEmpty()) {
        headers.insert(QStringLiteral("OpenAI-Organization"), config.organizationId.trimmed());
    }
    return headers;
}

SyncResult OpenAiAdapter::validateCredentials(const ProviderConfig& config, const QString& apiKey) {
    SyncResult result = requestCosts(config, apiKey, UsageRange::Last7Days);
    result.message = result.success
        ? QStringLiteral("OpenAI admin key is valid for organization cost APIs.")
        : QStringLiteral("Failed to validate OpenAI admin key.");
    return result;
}

SyncResult OpenAiAdapter::fetchOverview(const ProviderConfig& config, const QString& apiKey) {
    SyncResult result = requestCosts(config, apiKey, UsageRange::CurrentMonth);
    if (!result.success) {
        return result;
    }

    AccountOverview overview;
    overview.authStatus = AuthStatus::Valid;
    overview.lastSyncAt = result.completedAt;
    overview.monthlyUsed = sumCost(result.usagePoints);
    overview.currency = result.usagePoints.isEmpty() ? QStringLiteral("USD") : result.usagePoints.first().currency;
    overview.dataSource = config.monthlyBudget > 0.0 ? DataSource::Estimated : DataSource::Official;
    overview.statusSummary = QStringLiteral("Official OpenAI organization cost data fetched.");
    if (config.monthlyBudget > 0.0 && overview.monthlyUsed.has_value()) {
        overview.estimatedRemaining = config.monthlyBudget - overview.monthlyUsed.value();
        overview.alerts << QStringLiteral("Remaining is derived from the configured monthly budget.");
    } else {
        overview.alerts << QStringLiteral("Add a monthly budget to estimate remaining spend.");
    }

    result.overview = overview;
    result.message = QStringLiteral("OpenAI overview fetched successfully.");
    return result;
}

SyncResult OpenAiAdapter::fetchUsage(const ProviderConfig& config, const QString& apiKey, UsageRange range) {
    return requestCosts(config, apiKey, range);
}

SyncResult OpenAiAdapter::requestCosts(const ProviderConfig& config, const QString& apiKey, UsageRange range) const {
    SyncResult result;
    result.completedAt = QDateTime::currentDateTimeUtc();

    if (apiKey.trimmed().isEmpty()) {
        result.message = QStringLiteral("OpenAI key is empty.");
        return result;
    }

    const DateRange dateRange = dateRangeFor(range, result.completedAt);
    const QList<QPair<QString, QString>> queryItems = {
        {QStringLiteral("start_time"), QString::number(dateRange.start.toSecsSinceEpoch())},
        {QStringLiteral("end_time"), QString::number(dateRange.end.toSecsSinceEpoch())},
        {QStringLiteral("bucket_width"), QStringLiteral("1d")},
        {QStringLiteral("limit"), QStringLiteral("31")}
    };

    const HttpResponse response = m_httpClient.get(
        QUrl(QStringLiteral("https://api.openai.com/v1/organization/costs")),
        headersFor(config, apiKey),
        queryItems);

    result.statusCode = response.statusCode;
    result.rawResponse = QString::fromUtf8(response.rawBody);
    if (!response.ok) {
        result.message = response.errorMessage.isEmpty() ? QStringLiteral("OpenAI cost request failed.") : response.errorMessage;
        return result;
    }

    QString parseError;
    result.usagePoints = ProviderParsers::parseOpenAiCosts(response.json, &parseError);
    if (result.usagePoints.isEmpty() && !parseError.isEmpty()) {
        result.message = parseError;
        return result;
    }

    result.success = true;
    result.message = QStringLiteral("OpenAI cost data fetched successfully.");
    return result;
}

}  // namespace llm

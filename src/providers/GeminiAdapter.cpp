#include "GeminiAdapter.h"

#include "core/ProviderParsers.h"

namespace llm {

GeminiAdapter::GeminiAdapter(const HttpJsonClient& httpClient)
    : m_httpClient(httpClient) {}

ProviderId GeminiAdapter::providerId() const {
    return ProviderId::Gemini;
}

CapabilityFlags GeminiAdapter::capabilities() const {
    return CanValidateCredentials | CanFetchOverview | HasModelMetadata;
}

SyncResult GeminiAdapter::validateCredentials(const ProviderConfig&, const QString& apiKey) {
    SyncResult result = requestModels(apiKey);
    result.message = result.success
        ? QStringLiteral("Gemini API key is valid.")
        : QStringLiteral("Failed to validate Gemini API key.");
    return result;
}

SyncResult GeminiAdapter::fetchOverview(const ProviderConfig&, const QString& apiKey) {
    return requestModels(apiKey);
}

SyncResult GeminiAdapter::fetchUsage(const ProviderConfig&, const QString&, UsageRange) {
    SyncResult result;
    result.completedAt = QDateTime::currentDateTimeUtc();
    result.message = QStringLiteral("Gemini usage and remaining spend are not available in v1.");
    return result;
}

SyncResult GeminiAdapter::requestModels(const QString& apiKey) const {
    SyncResult result;
    result.completedAt = QDateTime::currentDateTimeUtc();

    if (apiKey.trimmed().isEmpty()) {
        result.message = QStringLiteral("Gemini key is empty.");
        return result;
    }

    const QList<QPair<QString, QString>> queryItems = {
        {QStringLiteral("key"), apiKey},
        {QStringLiteral("pageSize"), QStringLiteral("5")}
    };
    const HttpResponse response = m_httpClient.get(
        QUrl(QStringLiteral("https://generativelanguage.googleapis.com/v1beta/models")),
        {},
        queryItems);

    result.statusCode = response.statusCode;
    result.rawResponse = QString::fromUtf8(response.rawBody);
    if (!response.ok) {
        result.message = response.errorMessage.isEmpty() ? QStringLiteral("Gemini models request failed.") : response.errorMessage;
        return result;
    }

    QString parseError;
    const QStringList models = ProviderParsers::parseGeminiModels(response.json, &parseError);
    if (models.isEmpty() && !parseError.isEmpty()) {
        result.message = parseError;
        return result;
    }

    AccountOverview overview;
    overview.authStatus = AuthStatus::Valid;
    overview.lastSyncAt = result.completedAt;
    overview.dataSource = DataSource::Unavailable;
    overview.statusSummary = QStringLiteral("Gemini key is valid. Usage and remaining spend are not available in v1.");
    overview.alerts << QStringLiteral("Gemini v1 shows API key status and available models only.");
    if (!models.isEmpty()) {
        overview.alerts << QStringLiteral("Models available: %1").arg(models.join(QStringLiteral(", ")));
    }

    result.success = true;
    result.message = QStringLiteral("Gemini model metadata fetched successfully.");
    result.overview = overview;
    result.infoMessages = models;
    return result;
}

}  // namespace llm

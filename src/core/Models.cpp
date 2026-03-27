#include "Models.h"

#include <QTime>

namespace llm {

QString providerIdToString(ProviderId providerId) {
    switch (providerId) {
    case ProviderId::DeepSeek:
        return QStringLiteral("deepseek");
    case ProviderId::OpenAI:
        return QStringLiteral("openai");
    case ProviderId::Anthropic:
        return QStringLiteral("anthropic");
    case ProviderId::Gemini:
        return QStringLiteral("gemini");
    }
    return QStringLiteral("deepseek");
}

QString providerDisplayName(ProviderId providerId) {
    switch (providerId) {
    case ProviderId::DeepSeek:
        return QStringLiteral("DeepSeek");
    case ProviderId::OpenAI:
        return QStringLiteral("OpenAI");
    case ProviderId::Anthropic:
        return QStringLiteral("Anthropic");
    case ProviderId::Gemini:
        return QStringLiteral("Gemini (AI Studio)");
    }
    return QStringLiteral("DeepSeek");
}

ProviderId providerIdFromString(const QString& value) {
    const QString normalized = value.trimmed().toLower();
    if (normalized == QStringLiteral("openai")) {
        return ProviderId::OpenAI;
    }
    if (normalized == QStringLiteral("anthropic")) {
        return ProviderId::Anthropic;
    }
    if (normalized == QStringLiteral("gemini")) {
        return ProviderId::Gemini;
    }
    return ProviderId::DeepSeek;
}

QList<ProviderId> allProviderIds() {
    return {
        ProviderId::DeepSeek,
        ProviderId::OpenAI,
        ProviderId::Anthropic,
        ProviderId::Gemini
    };
}

QString dataSourceToString(DataSource dataSource) {
    switch (dataSource) {
    case DataSource::Official:
        return QStringLiteral("official");
    case DataSource::Snapshot:
        return QStringLiteral("snapshot");
    case DataSource::Estimated:
        return QStringLiteral("estimated");
    case DataSource::Unavailable:
        return QStringLiteral("unavailable");
    }
    return QStringLiteral("unavailable");
}

QString authStatusToString(AuthStatus status) {
    switch (status) {
    case AuthStatus::Unknown:
        return QStringLiteral("unknown");
    case AuthStatus::Valid:
        return QStringLiteral("valid");
    case AuthStatus::Invalid:
        return QStringLiteral("invalid");
    case AuthStatus::Unauthorized:
        return QStringLiteral("unauthorized");
    case AuthStatus::Unsupported:
        return QStringLiteral("unsupported");
    }
    return QStringLiteral("unknown");
}

QString usageRangeToString(UsageRange range) {
    switch (range) {
    case UsageRange::Last7Days:
        return QStringLiteral("7 days");
    case UsageRange::Last30Days:
        return QStringLiteral("30 days");
    case UsageRange::CurrentMonth:
        return QStringLiteral("current month");
    }
    return QStringLiteral("7 days");
}

DateRange dateRangeFor(UsageRange range, const QDateTime& now) {
    const QDate currentDate = now.date();
    switch (range) {
    case UsageRange::Last7Days:
        return {
            QDateTime(currentDate.addDays(-6), QTime(0, 0), Qt::UTC),
            QDateTime(currentDate.addDays(1), QTime(0, 0), Qt::UTC)
        };
    case UsageRange::Last30Days:
        return {
            QDateTime(currentDate.addDays(-29), QTime(0, 0), Qt::UTC),
            QDateTime(currentDate.addDays(1), QTime(0, 0), Qt::UTC)
        };
    case UsageRange::CurrentMonth:
        return {
            QDateTime(QDate(currentDate.year(), currentDate.month(), 1), QTime(0, 0), Qt::UTC),
            QDateTime(currentDate.addDays(1), QTime(0, 0), Qt::UTC)
        };
    }
    return {
        QDateTime(currentDate.addDays(-6), QTime(0, 0), Qt::UTC),
        QDateTime(currentDate.addDays(1), QTime(0, 0), Qt::UTC)
    };
}

}  // namespace llm

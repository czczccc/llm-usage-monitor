#pragma once

#include "Models.h"

#include <QJsonDocument>

#include <optional>

namespace llm::ProviderParsers {

struct ParsedBalance {
    double balance = 0.0;
    QString currency = QStringLiteral("USD");
    QStringList alerts;
};

std::optional<ParsedBalance> parseDeepSeekBalance(const QJsonDocument& document, QString* error);
QList<UsagePoint> parseOpenAiCosts(const QJsonDocument& document, QString* error);
QList<UsagePoint> parseAnthropicCosts(const QJsonDocument& document, QString* error);
QStringList parseGeminiModels(const QJsonDocument& document, QString* error);

}  // namespace llm::ProviderParsers

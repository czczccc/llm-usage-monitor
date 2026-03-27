#include "ProviderParsers.h"

#include <QJsonArray>
#include <QJsonObject>

#include <algorithm>

namespace {

double valueToDouble(const QJsonValue& value, bool* ok = nullptr) {
    if (value.isDouble()) {
        if (ok) {
            *ok = true;
        }
        return value.toDouble();
    }

    if (value.isString()) {
        bool localOk = false;
        const double parsed = value.toString().toDouble(&localOk);
        if (ok) {
            *ok = localOk;
        }
        return parsed;
    }

    if (ok) {
        *ok = false;
    }
    return 0.0;
}

double parseAmount(const QJsonObject& object, QString* currency = nullptr) {
    const QStringList directKeys = {
        QStringLiteral("cost_usd"),
        QStringLiteral("total_cost_usd"),
        QStringLiteral("amount_value")
    };

    for (const QString& key : directKeys) {
        if (!object.contains(key)) {
            continue;
        }

        bool ok = false;
        const double amount = valueToDouble(object.value(key), &ok);
        if (ok) {
            if (currency && key.contains(QStringLiteral("usd"))) {
                *currency = QStringLiteral("USD");
            }
            return amount;
        }
    }

    if (object.contains(QStringLiteral("amount")) && object.value(QStringLiteral("amount")).isObject()) {
        const QJsonObject amountObject = object.value(QStringLiteral("amount")).toObject();
        if (currency && amountObject.contains(QStringLiteral("currency"))) {
            *currency = amountObject.value(QStringLiteral("currency")).toString().toUpper();
        }
        return amountObject.value(QStringLiteral("value")).toDouble();
    }

    if (object.contains(QStringLiteral("cost")) && object.value(QStringLiteral("cost")).isObject()) {
        const QJsonObject amountObject = object.value(QStringLiteral("cost")).toObject();
        if (currency && amountObject.contains(QStringLiteral("currency"))) {
            *currency = amountObject.value(QStringLiteral("currency")).toString().toUpper();
        }
        if (amountObject.contains(QStringLiteral("value"))) {
            return amountObject.value(QStringLiteral("value")).toDouble();
        }
    }

    return 0.0;
}

QDateTime parseDateTime(const QJsonValue& value) {
    if (value.isDouble()) {
        return QDateTime::fromSecsSinceEpoch(static_cast<qint64>(value.toDouble()), Qt::UTC);
    }

    QDateTime dateTime = QDateTime::fromString(value.toString(), Qt::ISODate);
    if (!dateTime.isValid()) {
        dateTime = QDateTime::fromString(value.toString(), Qt::ISODateWithMs);
    }
    if (dateTime.isValid() && dateTime.timeSpec() != Qt::UTC) {
        dateTime = dateTime.toUTC();
    }
    return dateTime;
}

QList<llm::UsagePoint> sortPoints(QList<llm::UsagePoint> points) {
    std::sort(points.begin(), points.end(), [](const llm::UsagePoint& left, const llm::UsagePoint& right) {
        return left.bucketStart < right.bucketStart;
    });
    return points;
}

}  // namespace

namespace llm::ProviderParsers {

std::optional<ParsedBalance> parseDeepSeekBalance(const QJsonDocument& document, QString* error) {
    if (!document.isObject()) {
        if (error) {
            *error = QStringLiteral("DeepSeek response is not a JSON object.");
        }
        return std::nullopt;
    }

    const QJsonObject root = document.object();
    if (root.contains(QStringLiteral("is_available")) && !root.value(QStringLiteral("is_available")).toBool(true)) {
        if (error) {
            *error = QStringLiteral("DeepSeek account is not available.");
        }
        return std::nullopt;
    }

    const QJsonArray balanceInfos = root.value(QStringLiteral("balance_infos")).toArray();
    if (balanceInfos.isEmpty()) {
        if (error) {
            *error = QStringLiteral("DeepSeek response did not include balance_infos.");
        }
        return std::nullopt;
    }

    ParsedBalance parsed;
    bool hasValue = false;
    QString chosenCurrency;

    for (const QJsonValue& value : balanceInfos) {
        const QJsonObject object = value.toObject();
        const QString currency = object.value(QStringLiteral("currency")).toString().toUpper();

        bool ok = false;
        double amount = 0.0;
        const QStringList candidateKeys = {
            QStringLiteral("total_balance"),
            QStringLiteral("available_balance"),
            QStringLiteral("balance")
        };

        for (const QString& key : candidateKeys) {
            if (!object.contains(key)) {
                continue;
            }
            amount = valueToDouble(object.value(key), &ok);
            if (ok) {
                break;
            }
        }

        if (!ok) {
            continue;
        }

        if (!hasValue) {
            parsed.currency = currency.isEmpty() ? QStringLiteral("USD") : currency;
            chosenCurrency = parsed.currency;
            parsed.balance = amount;
            hasValue = true;
            continue;
        }

        if (currency == chosenCurrency || currency.isEmpty()) {
            parsed.balance += amount;
        } else {
            parsed.alerts << QStringLiteral("Multiple DeepSeek balance currencies detected; showing %1 only.").arg(chosenCurrency);
        }
    }

    if (!hasValue) {
        if (error) {
            *error = QStringLiteral("DeepSeek balance_infos did not contain a usable balance field.");
        }
        return std::nullopt;
    }

    return parsed;
}

QList<UsagePoint> parseOpenAiCosts(const QJsonDocument& document, QString* error) {
    QList<UsagePoint> points;
    if (!document.isObject()) {
        if (error) {
            *error = QStringLiteral("OpenAI cost response is not a JSON object.");
        }
        return points;
    }

    const QJsonArray data = document.object().value(QStringLiteral("data")).toArray();
    for (const QJsonValue& bucketValue : data) {
        const QJsonObject bucketObject = bucketValue.toObject();
        UsagePoint point;
        point.bucketStart = parseDateTime(bucketObject.value(QStringLiteral("start_time")));
        point.bucketEnd = parseDateTime(bucketObject.value(QStringLiteral("end_time")));
        point.dataSource = DataSource::Official;

        QString currency = QStringLiteral("USD");
        double amount = 0.0;
        if (bucketObject.contains(QStringLiteral("results"))) {
            const QJsonArray results = bucketObject.value(QStringLiteral("results")).toArray();
            for (const QJsonValue& resultValue : results) {
                amount += parseAmount(resultValue.toObject(), &currency);
            }
        } else {
            amount = parseAmount(bucketObject, &currency);
        }
        point.cost = amount;
        point.currency = currency;

        if (!point.bucketStart.isValid()) {
            point.bucketStart = QDateTime::currentDateTimeUtc();
        }
        if (!point.bucketEnd.isValid()) {
            point.bucketEnd = point.bucketStart.addDays(1);
        }

        points << point;
    }

    if (points.isEmpty() && error) {
        *error = QStringLiteral("OpenAI cost response did not include any cost buckets.");
    }
    return sortPoints(points);
}

QList<UsagePoint> parseAnthropicCosts(const QJsonDocument& document, QString* error) {
    QList<UsagePoint> points;
    if (!document.isObject()) {
        if (error) {
            *error = QStringLiteral("Anthropic cost response is not a JSON object.");
        }
        return points;
    }

    const QJsonArray data = document.object().value(QStringLiteral("data")).toArray();
    for (const QJsonValue& itemValue : data) {
        const QJsonObject itemObject = itemValue.toObject();
        UsagePoint point;
        point.bucketStart = parseDateTime(itemObject.value(QStringLiteral("starting_at")));
        point.bucketEnd = parseDateTime(itemObject.value(QStringLiteral("ending_at")));
        point.dataSource = DataSource::Official;
        point.currency = QStringLiteral("USD");

        double amount = 0.0;
        if (itemObject.contains(QStringLiteral("results"))) {
            const QJsonArray results = itemObject.value(QStringLiteral("results")).toArray();
            for (const QJsonValue& resultValue : results) {
                amount += parseAmount(resultValue.toObject(), &point.currency);
            }
        } else {
            amount = parseAmount(itemObject, &point.currency);
        }

        point.cost = amount;
        if (!point.bucketStart.isValid()) {
            point.bucketStart = QDateTime::currentDateTimeUtc();
        }
        if (!point.bucketEnd.isValid()) {
            point.bucketEnd = point.bucketStart.addDays(1);
        }
        points << point;
    }

    if (points.isEmpty() && error) {
        *error = QStringLiteral("Anthropic cost response did not include any cost buckets.");
    }
    return sortPoints(points);
}

QStringList parseGeminiModels(const QJsonDocument& document, QString* error) {
    QStringList models;
    if (!document.isObject()) {
        if (error) {
            *error = QStringLiteral("Gemini response is not a JSON object.");
        }
        return models;
    }

    const QJsonArray modelArray = document.object().value(QStringLiteral("models")).toArray();
    for (const QJsonValue& modelValue : modelArray) {
        const QString name = modelValue.toObject().value(QStringLiteral("name")).toString();
        if (!name.isEmpty()) {
            models << name;
        }
    }

    if (models.isEmpty() && error) {
        *error = QStringLiteral("Gemini response did not include any models.");
    }
    return models;
}

}  // namespace llm::ProviderParsers

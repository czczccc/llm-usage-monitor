#include "core/ProviderParsers.h"

#include <QFile>
#include <QJsonDocument>
#include <QtTest/QTest>

using namespace llm;

namespace {

QJsonDocument loadFixture(const QString& relativePath) {
    QFile file(QFINDTESTDATA(relativePath));
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    return QJsonDocument::fromJson(file.readAll());
}

}  // namespace

class ProviderParserTests : public QObject {
    Q_OBJECT

private slots:
    void parsesDeepSeekBalance();
    void parsesOpenAiCosts();
    void parsesAnthropicCosts();
    void parsesGeminiModels();
};

void ProviderParserTests::parsesDeepSeekBalance() {
    QString error;
    const auto parsed = ProviderParsers::parseDeepSeekBalance(loadFixture(QStringLiteral("fixtures/deepseek_balance.json")), &error);
    QVERIFY2(parsed.has_value(), qPrintable(error));
    QCOMPARE(parsed->currency, QStringLiteral("USD"));
    QCOMPARE(parsed->balance, 12.34);
}

void ProviderParserTests::parsesOpenAiCosts() {
    QString error;
    const QList<UsagePoint> points = ProviderParsers::parseOpenAiCosts(loadFixture(QStringLiteral("fixtures/openai_costs.json")), &error);
    QVERIFY2(!points.isEmpty(), qPrintable(error));
    QCOMPARE(points.size(), 2);
    QCOMPARE(points.at(0).cost, 1.25);
    QCOMPARE(points.at(1).cost, 2.75);
}

void ProviderParserTests::parsesAnthropicCosts() {
    QString error;
    const QList<UsagePoint> points = ProviderParsers::parseAnthropicCosts(loadFixture(QStringLiteral("fixtures/anthropic_costs.json")), &error);
    QVERIFY2(!points.isEmpty(), qPrintable(error));
    QCOMPARE(points.size(), 2);
    QCOMPARE(points.at(0).cost, 1.10);
    QCOMPARE(points.at(1).cost, 2.90);
}

void ProviderParserTests::parsesGeminiModels() {
    QString error;
    const QStringList models = ProviderParsers::parseGeminiModels(loadFixture(QStringLiteral("fixtures/gemini_models.json")), &error);
    QVERIFY2(!models.isEmpty(), qPrintable(error));
    QCOMPARE(models.size(), 2);
    QCOMPARE(models.at(0), QStringLiteral("models/gemini-2.0-flash"));
}

QTEST_APPLESS_MAIN(ProviderParserTests)

#include "ProviderParserTests.moc"

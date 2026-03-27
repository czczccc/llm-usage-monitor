#include "storage/CredentialStore.h"

#include <QtTest/QTest>

using namespace llm;

class CredentialStoreTests : public QObject {
    Q_OBJECT

private slots:
    void inMemoryStoreRoundTrip();
};

void CredentialStoreTests::inMemoryStoreRoundTrip() {
    InMemoryCredentialStore store;
    QString error;

    QVERIFY(store.writeSecret(QStringLiteral("test-target"), QStringLiteral("secret-value"), &error));
    QCOMPARE(store.readSecret(QStringLiteral("test-target"), &error), QStringLiteral("secret-value"));
    QVERIFY(store.deleteSecret(QStringLiteral("test-target"), &error));
    QVERIFY(store.readSecret(QStringLiteral("test-target"), &error).isEmpty());
}

QTEST_APPLESS_MAIN(CredentialStoreTests)

#include "CredentialStoreTests.moc"

#pragma once

#include "core/Models.h"

#include <QHash>

namespace llm {

class ICredentialStore {
public:
    virtual ~ICredentialStore() = default;

    virtual bool writeSecret(const QString& target, const QString& secret, QString* error) = 0;
    virtual QString readSecret(const QString& target, QString* error) const = 0;
    virtual bool deleteSecret(const QString& target, QString* error) = 0;
};

class WindowsCredentialStore : public ICredentialStore {
public:
    bool writeSecret(const QString& target, const QString& secret, QString* error) override;
    QString readSecret(const QString& target, QString* error) const override;
    bool deleteSecret(const QString& target, QString* error) override;
};

class InMemoryCredentialStore : public ICredentialStore {
public:
    bool writeSecret(const QString& target, const QString& secret, QString* error) override;
    QString readSecret(const QString& target, QString* error) const override;
    bool deleteSecret(const QString& target, QString* error) override;

private:
    QHash<QString, QString> m_secrets;
};

QString defaultCredentialTarget(const ProviderConfig& config);

}  // namespace llm

#include "CredentialStore.h"

#ifdef Q_OS_WIN
#include <Windows.h>
#include <wincred.h>
#endif

namespace llm {

bool WindowsCredentialStore::writeSecret(const QString& target, const QString& secret, QString* error) {
#ifdef Q_OS_WIN
    QByteArray blob = secret.toUtf8();

    CREDENTIALW credential{};
    credential.Type = CRED_TYPE_GENERIC;
    credential.TargetName = const_cast<LPWSTR>(reinterpret_cast<LPCWSTR>(target.utf16()));
    credential.CredentialBlobSize = static_cast<DWORD>(blob.size());
    credential.CredentialBlob = reinterpret_cast<LPBYTE>(blob.data());
    credential.Persist = CRED_PERSIST_LOCAL_MACHINE;
    credential.UserName = const_cast<LPWSTR>(L"LlmUsageMonitor");

    if (!CredWriteW(&credential, 0)) {
        if (error) {
            *error = QStringLiteral("CredWriteW failed with error %1.").arg(GetLastError());
        }
        return false;
    }
    return true;
#else
    if (error) {
        *error = QStringLiteral("Windows Credential Manager is only supported on Windows.");
    }
    Q_UNUSED(target)
    Q_UNUSED(secret)
    return false;
#endif
}

QString WindowsCredentialStore::readSecret(const QString& target, QString* error) const {
#ifdef Q_OS_WIN
    PCREDENTIALW credential = nullptr;
    if (!CredReadW(reinterpret_cast<LPCWSTR>(target.utf16()), CRED_TYPE_GENERIC, 0, &credential)) {
        if (error) {
            *error = QStringLiteral("CredReadW failed with error %1.").arg(GetLastError());
        }
        return {};
    }

    const QByteArray blob(reinterpret_cast<const char*>(credential->CredentialBlob),
                          static_cast<int>(credential->CredentialBlobSize));
    const QString secret = QString::fromUtf8(blob);
    CredFree(credential);
    return secret;
#else
    if (error) {
        *error = QStringLiteral("Windows Credential Manager is only supported on Windows.");
    }
    Q_UNUSED(target)
    return {};
#endif
}

bool WindowsCredentialStore::deleteSecret(const QString& target, QString* error) {
#ifdef Q_OS_WIN
    if (!CredDeleteW(reinterpret_cast<LPCWSTR>(target.utf16()), CRED_TYPE_GENERIC, 0)) {
        if (error) {
            *error = QStringLiteral("CredDeleteW failed with error %1.").arg(GetLastError());
        }
        return false;
    }
    return true;
#else
    if (error) {
        *error = QStringLiteral("Windows Credential Manager is only supported on Windows.");
    }
    Q_UNUSED(target)
    return false;
#endif
}

bool InMemoryCredentialStore::writeSecret(const QString& target, const QString& secret, QString* error) {
    Q_UNUSED(error)
    m_secrets.insert(target, secret);
    return true;
}

QString InMemoryCredentialStore::readSecret(const QString& target, QString* error) const {
    if (!m_secrets.contains(target)) {
        if (error) {
            *error = QStringLiteral("Secret not found.");
        }
        return {};
    }
    return m_secrets.value(target);
}

bool InMemoryCredentialStore::deleteSecret(const QString& target, QString* error) {
    if (!m_secrets.remove(target)) {
        if (error) {
            *error = QStringLiteral("Secret not found.");
        }
        return false;
    }
    return true;
}

QString defaultCredentialTarget(const ProviderConfig& config) {
    return QStringLiteral("LlmUsageMonitor/%1/%2")
        .arg(providerIdToString(config.providerId), config.id);
}

}  // namespace llm

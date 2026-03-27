#pragma once

#include "core/HttpJsonClient.h"
#include "core/IProviderAdapter.h"

namespace llm {

class AnthropicAdapter : public IProviderAdapter {
public:
    explicit AnthropicAdapter(const HttpJsonClient& httpClient);

    ProviderId providerId() const override;
    CapabilityFlags capabilities() const override;
    SyncResult validateCredentials(const ProviderConfig& config, const QString& apiKey) override;
    SyncResult fetchOverview(const ProviderConfig& config, const QString& apiKey) override;
    SyncResult fetchUsage(const ProviderConfig& config, const QString& apiKey, UsageRange range) override;

private:
    SyncResult requestCosts(const ProviderConfig& config, const QString& apiKey, UsageRange range) const;

    const HttpJsonClient& m_httpClient;
};

}  // namespace llm

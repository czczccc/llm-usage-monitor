#pragma once

#include "Models.h"

namespace llm {

class IProviderAdapter {
public:
    virtual ~IProviderAdapter() = default;

    virtual ProviderId providerId() const = 0;
    virtual CapabilityFlags capabilities() const = 0;
    virtual SyncResult validateCredentials(const ProviderConfig& config, const QString& apiKey) = 0;
    virtual SyncResult fetchOverview(const ProviderConfig& config, const QString& apiKey) = 0;
    virtual SyncResult fetchUsage(const ProviderConfig& config, const QString& apiKey, UsageRange range) = 0;
};

}  // namespace llm

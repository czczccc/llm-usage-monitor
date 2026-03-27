#pragma once

#include "HttpJsonClient.h"
#include "IProviderAdapter.h"

#include <memory>

namespace llm {

std::unique_ptr<IProviderAdapter> createProviderAdapter(ProviderId providerId, const HttpJsonClient& httpClient);

}  // namespace llm

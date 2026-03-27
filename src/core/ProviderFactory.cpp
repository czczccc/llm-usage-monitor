#include "ProviderFactory.h"

#include "providers/AnthropicAdapter.h"
#include "providers/DeepSeekAdapter.h"
#include "providers/GeminiAdapter.h"
#include "providers/OpenAiAdapter.h"

namespace llm {

std::unique_ptr<IProviderAdapter> createProviderAdapter(ProviderId providerId, const HttpJsonClient& httpClient) {
    switch (providerId) {
    case ProviderId::DeepSeek:
        return std::make_unique<DeepSeekAdapter>(httpClient);
    case ProviderId::OpenAI:
        return std::make_unique<OpenAiAdapter>(httpClient);
    case ProviderId::Anthropic:
        return std::make_unique<AnthropicAdapter>(httpClient);
    case ProviderId::Gemini:
        return std::make_unique<GeminiAdapter>(httpClient);
    }
    return std::make_unique<DeepSeekAdapter>(httpClient);
}

}  // namespace llm

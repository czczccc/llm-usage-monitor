#include "SettingsStore.h"

#include <QCoreApplication>
#include <QSettings>

namespace llm {

SettingsStore::SettingsStore() {
    QCoreApplication::setOrganizationName(QStringLiteral("Codex"));
    QCoreApplication::setApplicationName(QStringLiteral("LlmUsageMonitor"));
}

QList<ProviderConfig> SettingsStore::loadProviderConfigs() const {
    QSettings settings;
    QList<ProviderConfig> configs;

    const int size = settings.beginReadArray(QStringLiteral("providers"));
    for (int index = 0; index < size; ++index) {
        settings.setArrayIndex(index);

        ProviderConfig config;
        config.id = settings.value(QStringLiteral("id")).toString();
        config.providerId = providerIdFromString(settings.value(QStringLiteral("providerId")).toString());
        config.displayName = settings.value(QStringLiteral("displayName")).toString();
        config.monthlyBudget = settings.value(QStringLiteral("monthlyBudget"), 0.0).toDouble();
        config.refreshIntervalMinutes = settings.value(QStringLiteral("refreshIntervalMinutes"), 30).toInt();
        config.organizationId = settings.value(QStringLiteral("organizationId")).toString();
        config.projectId = settings.value(QStringLiteral("projectId")).toString();
        config.modelFilters = settings.value(QStringLiteral("modelFilters")).toStringList();
        config.credentialTarget = settings.value(QStringLiteral("credentialTarget")).toString();
        config.lastAutoRefreshAt = settings.value(QStringLiteral("lastAutoRefreshAt")).toDateTime();
        configs << config;
    }
    settings.endArray();
    return configs;
}

void SettingsStore::saveProviderConfigs(const QList<ProviderConfig>& configs) const {
    QSettings settings;
    settings.beginWriteArray(QStringLiteral("providers"));
    for (int index = 0; index < configs.size(); ++index) {
        settings.setArrayIndex(index);
        const ProviderConfig& config = configs.at(index);
        settings.setValue(QStringLiteral("id"), config.id);
        settings.setValue(QStringLiteral("providerId"), providerIdToString(config.providerId));
        settings.setValue(QStringLiteral("displayName"), config.displayName);
        settings.setValue(QStringLiteral("monthlyBudget"), config.monthlyBudget);
        settings.setValue(QStringLiteral("refreshIntervalMinutes"), config.refreshIntervalMinutes);
        settings.setValue(QStringLiteral("organizationId"), config.organizationId);
        settings.setValue(QStringLiteral("projectId"), config.projectId);
        settings.setValue(QStringLiteral("modelFilters"), config.modelFilters);
        settings.setValue(QStringLiteral("credentialTarget"), config.credentialTarget);
        settings.setValue(QStringLiteral("lastAutoRefreshAt"), config.lastAutoRefreshAt);
    }
    settings.endArray();
}

QByteArray SettingsStore::loadWindowGeometry() const {
    QSettings settings;
    return settings.value(QStringLiteral("ui/windowGeometry")).toByteArray();
}

void SettingsStore::saveWindowGeometry(const QByteArray& geometry) const {
    QSettings settings;
    settings.setValue(QStringLiteral("ui/windowGeometry"), geometry);
}

QByteArray SettingsStore::loadSplitterState() const {
    QSettings settings;
    return settings.value(QStringLiteral("ui/splitterState")).toByteArray();
}

void SettingsStore::saveSplitterState(const QByteArray& splitterState) const {
    QSettings settings;
    settings.setValue(QStringLiteral("ui/splitterState"), splitterState);
}

}  // namespace llm

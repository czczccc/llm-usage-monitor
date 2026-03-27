#pragma once

#include "core/Models.h"

#include <QByteArray>
#include <QList>

namespace llm {

class SettingsStore {
public:
    SettingsStore();

    QList<ProviderConfig> loadProviderConfigs() const;
    void saveProviderConfigs(const QList<ProviderConfig>& configs) const;

    QByteArray loadWindowGeometry() const;
    void saveWindowGeometry(const QByteArray& geometry) const;

    QByteArray loadSplitterState() const;
    void saveSplitterState(const QByteArray& splitterState) const;
};

}  // namespace llm

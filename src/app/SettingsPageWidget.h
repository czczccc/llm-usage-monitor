#pragma once

#include "core/Models.h"

#include <QWidget>

QT_BEGIN_NAMESPACE
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
QT_END_NAMESPACE

namespace llm {

class SettingsPageWidget : public QWidget {
    Q_OBJECT

public:
    explicit SettingsPageWidget(QWidget* parent = nullptr);

    void setProviderConfig(const std::optional<ProviderConfig>& config);
    ProviderConfig providerConfig() const;
    QString apiKeyInput() const;
    void clearApiKeyInput();
    void setStatusText(const QString& text);

signals:
    void saveRequested(const llm::ProviderConfig& config, const QString& apiKey);
    void testRequested();
    void refreshRequested();

private:
    void updateHelpText(ProviderId providerId);

    QString m_providerId;
    QString m_credentialTarget;

    QComboBox* m_providerCombo = nullptr;
    QLineEdit* m_nameEdit = nullptr;
    QDoubleSpinBox* m_budgetSpin = nullptr;
    QSpinBox* m_refreshSpin = nullptr;
    QLineEdit* m_orgEdit = nullptr;
    QLineEdit* m_projectEdit = nullptr;
    QLineEdit* m_modelFiltersEdit = nullptr;
    QLineEdit* m_apiKeyEdit = nullptr;
    QLabel* m_helpLabel = nullptr;
    QLabel* m_statusLabel = nullptr;
};

}  // namespace llm

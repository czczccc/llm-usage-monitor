#include "SettingsPageWidget.h"

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

namespace llm {

SettingsPageWidget::SettingsPageWidget(QWidget* parent)
    : QWidget(parent) {
    auto* rootLayout = new QVBoxLayout(this);
    auto* formLayout = new QFormLayout();

    m_providerCombo = new QComboBox(this);
    for (ProviderId providerId : allProviderIds()) {
        m_providerCombo->addItem(providerDisplayName(providerId), providerIdToString(providerId));
    }
    m_providerCombo->setEnabled(false);

    m_nameEdit = new QLineEdit(this);
    m_budgetSpin = new QDoubleSpinBox(this);
    m_budgetSpin->setMaximum(1000000.0);
    m_budgetSpin->setDecimals(2);
    m_budgetSpin->setPrefix(QStringLiteral("$"));
    m_refreshSpin = new QSpinBox(this);
    m_refreshSpin->setRange(0, 1440);
    m_refreshSpin->setSuffix(QStringLiteral(" min"));
    m_orgEdit = new QLineEdit(this);
    m_projectEdit = new QLineEdit(this);
    m_modelFiltersEdit = new QLineEdit(this);
    m_apiKeyEdit = new QLineEdit(this);
    m_apiKeyEdit->setEchoMode(QLineEdit::Password);

    formLayout->addRow(QStringLiteral("Provider"), m_providerCombo);
    formLayout->addRow(QStringLiteral("Display name"), m_nameEdit);
    formLayout->addRow(QStringLiteral("Monthly budget"), m_budgetSpin);
    formLayout->addRow(QStringLiteral("Refresh interval"), m_refreshSpin);
    formLayout->addRow(QStringLiteral("Organization / workspace"), m_orgEdit);
    formLayout->addRow(QStringLiteral("Project / workspace ID"), m_projectEdit);
    formLayout->addRow(QStringLiteral("Model filters"), m_modelFiltersEdit);
    formLayout->addRow(QStringLiteral("API key (optional to update)"), m_apiKeyEdit);

    auto* buttonsLayout = new QHBoxLayout();
    auto* saveButton = new QPushButton(QStringLiteral("Save"), this);
    auto* testButton = new QPushButton(QStringLiteral("Test connection"), this);
    auto* refreshButton = new QPushButton(QStringLiteral("Refresh now"), this);
    buttonsLayout->addWidget(saveButton);
    buttonsLayout->addWidget(testButton);
    buttonsLayout->addWidget(refreshButton);
    buttonsLayout->addStretch(1);

    m_helpLabel = new QLabel(this);
    m_helpLabel->setWordWrap(true);
    m_statusLabel = new QLabel(QStringLiteral("Create or select a provider to edit settings."), this);
    m_statusLabel->setWordWrap(true);

    connect(saveButton, &QPushButton::clicked, this, [this]() {
        emit saveRequested(providerConfig(), apiKeyInput());
    });
    connect(testButton, &QPushButton::clicked, this, &SettingsPageWidget::testRequested);
    connect(refreshButton, &QPushButton::clicked, this, &SettingsPageWidget::refreshRequested);

    rootLayout->addLayout(formLayout);
    rootLayout->addLayout(buttonsLayout);
    rootLayout->addWidget(m_helpLabel);
    rootLayout->addWidget(m_statusLabel);
    rootLayout->addStretch(1);
}

void SettingsPageWidget::setProviderConfig(const std::optional<ProviderConfig>& config) {
    if (!config.has_value()) {
        m_providerId.clear();
        m_credentialTarget.clear();
        m_nameEdit->clear();
        m_budgetSpin->setValue(0.0);
        m_refreshSpin->setValue(30);
        m_orgEdit->clear();
        m_projectEdit->clear();
        m_modelFiltersEdit->clear();
        m_apiKeyEdit->clear();
        updateHelpText(ProviderId::DeepSeek);
        return;
    }

    const ProviderConfig& value = config.value();
    m_providerId = value.id;
    m_credentialTarget = value.credentialTarget;
    m_providerCombo->setCurrentIndex(m_providerCombo->findData(providerIdToString(value.providerId)));
    m_nameEdit->setText(value.displayName);
    m_budgetSpin->setValue(value.monthlyBudget);
    m_refreshSpin->setValue(value.refreshIntervalMinutes);
    m_orgEdit->setText(value.organizationId);
    m_projectEdit->setText(value.projectId);
    m_modelFiltersEdit->setText(value.modelFilters.join(QStringLiteral(", ")));
    m_apiKeyEdit->clear();
    updateHelpText(value.providerId);
}

ProviderConfig SettingsPageWidget::providerConfig() const {
    ProviderConfig config;
    config.id = m_providerId;
    config.providerId = providerIdFromString(m_providerCombo->currentData().toString());
    config.displayName = m_nameEdit->text().trimmed();
    config.monthlyBudget = m_budgetSpin->value();
    config.refreshIntervalMinutes = m_refreshSpin->value();
    config.organizationId = m_orgEdit->text().trimmed();
    config.projectId = m_projectEdit->text().trimmed();
    config.modelFilters = m_modelFiltersEdit->text().split(',', Qt::SkipEmptyParts);
    for (QString& item : config.modelFilters) {
        item = item.trimmed();
    }
    config.credentialTarget = m_credentialTarget;
    return config;
}

QString SettingsPageWidget::apiKeyInput() const {
    return m_apiKeyEdit->text().trimmed();
}

void SettingsPageWidget::clearApiKeyInput() {
    m_apiKeyEdit->clear();
}

void SettingsPageWidget::setStatusText(const QString& text) {
    m_statusLabel->setText(text);
}

void SettingsPageWidget::updateHelpText(ProviderId providerId) {
    QString helpText;
    switch (providerId) {
    case ProviderId::DeepSeek:
        helpText = QStringLiteral("DeepSeek shows official balance. Monthly usage is derived from local balance snapshots.");
        break;
    case ProviderId::OpenAI:
        helpText = QStringLiteral("OpenAI requires an admin key for organization cost APIs. Remaining spend is budget-based.");
        break;
    case ProviderId::Anthropic:
        helpText = QStringLiteral("Anthropic requires an admin key for organization cost APIs. Remaining spend is budget-based.");
        break;
    case ProviderId::Gemini:
        helpText = QStringLiteral("Gemini v1 validates API keys and lists models. Real usage and remaining spend are not available.");
        break;
    }
    m_helpLabel->setText(helpText);
}

}  // namespace llm

#include "OverviewPageWidget.h"

#include <QFormLayout>
#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>

namespace {

QString formatOptionalMoney(const std::optional<double>& value, const QString& currency) {
    if (!value.has_value()) {
        return QStringLiteral("--");
    }
    return QStringLiteral("%1 %2").arg(value.value(), 0, 'f', 2).arg(currency);
}

}  // namespace

namespace llm {

OverviewPageWidget::OverviewPageWidget(QWidget* parent)
    : QWidget(parent) {
    auto* rootLayout = new QVBoxLayout(this);
    auto* formLayout = new QFormLayout();

    m_authStatusLabel = new QLabel(QStringLiteral("--"), this);
    m_lastSyncLabel = new QLabel(QStringLiteral("--"), this);
    m_balanceLabel = new QLabel(QStringLiteral("--"), this);
    m_monthlyUsedLabel = new QLabel(QStringLiteral("--"), this);
    m_remainingLabel = new QLabel(QStringLiteral("--"), this);
    m_currencyLabel = new QLabel(QStringLiteral("--"), this);
    m_dataSourceLabel = new QLabel(QStringLiteral("--"), this);
    m_summaryLabel = new QLabel(QStringLiteral("Select a provider to view usage details."), this);
    m_summaryLabel->setWordWrap(true);

    formLayout->addRow(QStringLiteral("Auth status"), m_authStatusLabel);
    formLayout->addRow(QStringLiteral("Last sync"), m_lastSyncLabel);
    formLayout->addRow(QStringLiteral("Current balance"), m_balanceLabel);
    formLayout->addRow(QStringLiteral("Monthly used"), m_monthlyUsedLabel);
    formLayout->addRow(QStringLiteral("Estimated remaining"), m_remainingLabel);
    formLayout->addRow(QStringLiteral("Currency"), m_currencyLabel);
    formLayout->addRow(QStringLiteral("Data source"), m_dataSourceLabel);

    m_alertList = new QListWidget(this);

    rootLayout->addLayout(formLayout);
    rootLayout->addWidget(m_summaryLabel);
    rootLayout->addWidget(new QLabel(QStringLiteral("Alerts / notes"), this));
    rootLayout->addWidget(m_alertList, 1);
}

void OverviewPageWidget::setOverview(const std::optional<AccountOverview>& overview) {
    m_alertList->clear();
    if (!overview.has_value()) {
        m_authStatusLabel->setText(QStringLiteral("--"));
        m_lastSyncLabel->setText(QStringLiteral("--"));
        m_balanceLabel->setText(QStringLiteral("--"));
        m_monthlyUsedLabel->setText(QStringLiteral("--"));
        m_remainingLabel->setText(QStringLiteral("--"));
        m_currencyLabel->setText(QStringLiteral("--"));
        m_dataSourceLabel->setText(QStringLiteral("--"));
        m_summaryLabel->setText(QStringLiteral("No overview loaded yet. Save credentials and click Refresh."));
        return;
    }

    const AccountOverview& value = overview.value();
    m_authStatusLabel->setText(authStatusToString(value.authStatus));
    m_lastSyncLabel->setText(value.lastSyncAt.isValid()
                                 ? value.lastSyncAt.toLocalTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"))
                                 : QStringLiteral("--"));
    m_balanceLabel->setText(formatOptionalMoney(value.currentBalance, value.currency));
    m_monthlyUsedLabel->setText(formatOptionalMoney(value.monthlyUsed, value.currency));
    m_remainingLabel->setText(formatOptionalMoney(value.estimatedRemaining, value.currency));
    m_currencyLabel->setText(value.currency);
    m_dataSourceLabel->setText(dataSourceToString(value.dataSource));
    m_summaryLabel->setText(value.statusSummary.isEmpty() ? QStringLiteral("No summary available.") : value.statusSummary);
    for (const QString& alert : value.alerts) {
        m_alertList->addItem(alert);
    }
}

}  // namespace llm

#pragma once

#include "core/Models.h"

#include <QWidget>

class QLabel;
class QListWidget;

namespace llm {

class OverviewPageWidget : public QWidget {
    Q_OBJECT

public:
    explicit OverviewPageWidget(QWidget* parent = nullptr);

    void setOverview(const std::optional<AccountOverview>& overview);

private:
    QLabel* m_authStatusLabel = nullptr;
    QLabel* m_lastSyncLabel = nullptr;
    QLabel* m_balanceLabel = nullptr;
    QLabel* m_monthlyUsedLabel = nullptr;
    QLabel* m_remainingLabel = nullptr;
    QLabel* m_currencyLabel = nullptr;
    QLabel* m_dataSourceLabel = nullptr;
    QLabel* m_summaryLabel = nullptr;
    QListWidget* m_alertList = nullptr;
};

}  // namespace llm

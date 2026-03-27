#pragma once

#include "core/Models.h"

#include <QWidget>

QT_BEGIN_NAMESPACE
class QLabel;
class QComboBox;
class QTableWidget;
QT_END_NAMESPACE

namespace QtCharts {
class QChartView;
}

namespace llm {

class UsagePageWidget : public QWidget {
    Q_OBJECT

public:
    explicit UsagePageWidget(QWidget* parent = nullptr);

    UsageRange currentRange() const;
    void setUsagePoints(const QList<UsagePoint>& usagePoints, const QString& summary);

signals:
    void rangeChanged(llm::UsageRange range);

private:
    QComboBox* m_rangeCombo = nullptr;
    QtCharts::QChartView* m_chartView = nullptr;
    QTableWidget* m_table = nullptr;
    QLabel* m_summaryLabel = nullptr;
};

}  // namespace llm

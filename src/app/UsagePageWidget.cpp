#include "UsagePageWidget.h"

#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

#include <QComboBox>
#include <QHeaderView>
#include <QLabel>
#include <QTableWidget>
#include <QVBoxLayout>

#include <algorithm>

namespace llm {

UsagePageWidget::UsagePageWidget(QWidget* parent)
    : QWidget(parent) {
    auto* rootLayout = new QVBoxLayout(this);

    m_rangeCombo = new QComboBox(this);
    m_rangeCombo->addItem(QStringLiteral("7 days"), static_cast<int>(UsageRange::Last7Days));
    m_rangeCombo->addItem(QStringLiteral("30 days"), static_cast<int>(UsageRange::Last30Days));
    m_rangeCombo->addItem(QStringLiteral("Current month"), static_cast<int>(UsageRange::CurrentMonth));
    connect(m_rangeCombo,
            qOverload<int>(&QComboBox::currentIndexChanged),
            this,
            [this](int) {
        emit rangeChanged(currentRange());
    });

    m_summaryLabel = new QLabel(QStringLiteral("Usage data will appear here after a refresh."), this);
    m_summaryLabel->setWordWrap(true);

    m_chartView = new QtCharts::QChartView(new QtCharts::QChart(), this);
    m_chartView->setMinimumHeight(280);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels({
        QStringLiteral("Date"),
        QStringLiteral("Cost"),
        QStringLiteral("Currency"),
        QStringLiteral("Source"),
        QStringLiteral("Note")
    });
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->verticalHeader()->setVisible(false);

    rootLayout->addWidget(new QLabel(QStringLiteral("Range"), this));
    rootLayout->addWidget(m_rangeCombo);
    rootLayout->addWidget(m_summaryLabel);
    rootLayout->addWidget(m_chartView);
    rootLayout->addWidget(m_table, 1);
}

UsageRange UsagePageWidget::currentRange() const {
    return static_cast<UsageRange>(m_rangeCombo->currentData().toInt());
}

void UsagePageWidget::setUsagePoints(const QList<UsagePoint>& usagePoints, const QString& summary) {
    m_summaryLabel->setText(summary);

    auto* chart = new QtCharts::QChart();
    auto* barSeries = new QtCharts::QBarSeries(chart);
    auto* barSet = new QtCharts::QBarSet(QStringLiteral("Daily cost"), barSeries);
    auto* cumulativeSeries = new QtCharts::QLineSeries(chart);
    cumulativeSeries->setName(QStringLiteral("Cumulative spend"));

    QStringList categories;
    double cumulative = 0.0;
    double maxValue = 0.0;

    m_table->setRowCount(usagePoints.size());
    for (int row = 0; row < usagePoints.size(); ++row) {
        const UsagePoint& point = usagePoints.at(row);
        categories << point.bucketStart.date().toString(QStringLiteral("MM-dd"));
        *barSet << point.cost;
        cumulative += point.cost;
        cumulativeSeries->append(row, cumulative);
        maxValue = std::max(maxValue, std::max(point.cost, cumulative));

        m_table->setItem(row, 0, new QTableWidgetItem(point.bucketStart.date().toString(Qt::ISODate)));
        m_table->setItem(row, 1, new QTableWidgetItem(QString::number(point.cost, 'f', 2)));
        m_table->setItem(row, 2, new QTableWidgetItem(point.currency));
        m_table->setItem(row, 3, new QTableWidgetItem(dataSourceToString(point.dataSource)));
        m_table->setItem(row, 4, new QTableWidgetItem(point.note));
    }

    barSeries->append(barSet);
    chart->addSeries(barSeries);
    chart->addSeries(cumulativeSeries);
    chart->legend()->setVisible(true);
    chart->setTitle(QStringLiteral("Usage trend"));

    auto* axisX = new QtCharts::QBarCategoryAxis(chart);
    axisX->append(categories);
    chart->addAxis(axisX, Qt::AlignBottom);
    barSeries->attachAxis(axisX);
    cumulativeSeries->attachAxis(axisX);

    auto* axisY = new QtCharts::QValueAxis(chart);
    axisY->setRange(0.0, maxValue <= 0.0 ? 1.0 : maxValue * 1.15);
    axisY->setLabelFormat(QStringLiteral("%.2f"));
    chart->addAxis(axisY, Qt::AlignLeft);
    barSeries->attachAxis(axisY);
    cumulativeSeries->attachAxis(axisY);

    m_chartView->setChart(chart);
}

}  // namespace llm

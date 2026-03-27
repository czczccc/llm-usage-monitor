#include "LogPageWidget.h"

#include <QHeaderView>
#include <QTableWidget>
#include <QVBoxLayout>

namespace llm {

LogPageWidget::LogPageWidget(QWidget* parent)
    : QWidget(parent) {
    auto* rootLayout = new QVBoxLayout(this);
    m_table = new QTableWidget(this);
    m_table->setColumnCount(4);
    m_table->setHorizontalHeaderLabels({
        QStringLiteral("Time"),
        QStringLiteral("Level"),
        QStringLiteral("Message"),
        QStringLiteral("Response excerpt")
    });
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->verticalHeader()->setVisible(false);
    rootLayout->addWidget(m_table);
}

void LogPageWidget::setLogs(const QList<SyncLogEntry>& logs) {
    m_table->setRowCount(logs.size());
    for (int row = 0; row < logs.size(); ++row) {
        const SyncLogEntry& entry = logs.at(row);
        m_table->setItem(row, 0, new QTableWidgetItem(entry.createdAt.toLocalTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"))));
        m_table->setItem(row, 1, new QTableWidgetItem(entry.level));
        m_table->setItem(row, 2, new QTableWidgetItem(entry.message));
        m_table->setItem(row, 3, new QTableWidgetItem(entry.rawResponse));
    }
}

}  // namespace llm

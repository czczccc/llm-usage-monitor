#pragma once

#include "core/Models.h"

#include <QWidget>

QT_BEGIN_NAMESPACE
class QTableWidget;
QT_END_NAMESPACE

namespace llm {

class LogPageWidget : public QWidget {
    Q_OBJECT

public:
    explicit LogPageWidget(QWidget* parent = nullptr);

    void setLogs(const QList<SyncLogEntry>& logs);

private:
    QTableWidget* m_table = nullptr;
};

}  // namespace llm

#pragma once

#include "OverviewPageWidget.h"
#include "LogPageWidget.h"
#include "ProviderEditorDialog.h"
#include "SettingsPageWidget.h"
#include "UsagePageWidget.h"
#include "core/MonitorService.h"

#include <QMainWindow>

QT_BEGIN_NAMESPACE
class QListWidget;
class QSplitter;
class QTimer;
QT_END_NAMESPACE

namespace llm {

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override = default;

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void createUi();
    void reloadProviderList();
    void selectProvider(const QString& providerConfigId);
    QString currentProviderId() const;
    std::optional<ProviderConfig> currentConfig() const;
    void refreshViews();
    void refreshOverview();
    void refreshUsage();
    void refreshLogs();
    void saveCurrentProvider();
    void testCurrentProvider();
    void refreshCurrentProvider();
    void addProvider();
    void deleteCurrentProvider();

    MonitorService m_service;
    QListWidget* m_providerList = nullptr;
    OverviewPageWidget* m_overviewPage = nullptr;
    UsagePageWidget* m_usagePage = nullptr;
    SettingsPageWidget* m_settingsPage = nullptr;
    LogPageWidget* m_logPage = nullptr;
    QSplitter* m_splitter = nullptr;
    QTimer* m_autoRefreshTimer = nullptr;
};

}  // namespace llm

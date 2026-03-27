#include "MainWindow.h"

#include <QCloseEvent>
#include <QDialog>
#include <QHBoxLayout>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>
#include <QStatusBar>
#include <QTabWidget>
#include <QTimer>
#include <QVBoxLayout>

namespace llm {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent) {
    setWindowTitle(QStringLiteral("LLM Usage Monitor"));
    resize(1280, 820);
    createUi();
    reloadProviderList();

    const QByteArray geometry = m_service.loadWindowGeometry();
    if (!geometry.isEmpty()) {
        restoreGeometry(geometry);
    }
    const QByteArray splitterState = m_service.loadSplitterState();
    if (!splitterState.isEmpty()) {
        m_splitter->restoreState(splitterState);
    }

    m_autoRefreshTimer = new QTimer(this);
    m_autoRefreshTimer->setInterval(60 * 1000);
    connect(m_autoRefreshTimer, &QTimer::timeout, this, [this]() {
        m_service.refreshDueProviders();
        refreshViews();
    });
    m_autoRefreshTimer->start();

    statusBar()->showMessage(QStringLiteral("SQLite: %1").arg(m_service.databasePath()));
}

void MainWindow::createUi() {
    auto* central = new QWidget(this);
    auto* rootLayout = new QHBoxLayout(central);

    m_splitter = new QSplitter(Qt::Horizontal, central);
    rootLayout->addWidget(m_splitter);

    auto* leftPanel = new QWidget(m_splitter);
    auto* leftLayout = new QVBoxLayout(leftPanel);
    m_providerList = new QListWidget(leftPanel);
    auto* addButton = new QPushButton(QStringLiteral("Add provider"), leftPanel);
    auto* deleteButton = new QPushButton(QStringLiteral("Delete provider"), leftPanel);
    leftLayout->addWidget(m_providerList, 1);
    leftLayout->addWidget(addButton);
    leftLayout->addWidget(deleteButton);

    auto* tabs = new QTabWidget(m_splitter);
    m_overviewPage = new OverviewPageWidget(tabs);
    m_usagePage = new UsagePageWidget(tabs);
    m_settingsPage = new SettingsPageWidget(tabs);
    m_logPage = new LogPageWidget(tabs);

    tabs->addTab(m_overviewPage, QStringLiteral("Overview"));
    tabs->addTab(m_usagePage, QStringLiteral("Usage"));
    tabs->addTab(m_settingsPage, QStringLiteral("Settings"));
    tabs->addTab(m_logPage, QStringLiteral("Logs"));

    setCentralWidget(central);

    connect(addButton, &QPushButton::clicked, this, &MainWindow::addProvider);
    connect(deleteButton, &QPushButton::clicked, this, &MainWindow::deleteCurrentProvider);
    connect(m_providerList, &QListWidget::currentTextChanged, this, [this]() {
        refreshViews();
    });
    connect(m_usagePage, &UsagePageWidget::rangeChanged, this, [this](UsageRange) {
        refreshUsage();
    });
    connect(m_settingsPage, &SettingsPageWidget::saveRequested, this, [this](const ProviderConfig&, const QString&) {
        saveCurrentProvider();
    });
    connect(m_settingsPage, &SettingsPageWidget::testRequested, this, &MainWindow::testCurrentProvider);
    connect(m_settingsPage, &SettingsPageWidget::refreshRequested, this, &MainWindow::refreshCurrentProvider);
}

void MainWindow::reloadProviderList() {
    const QString previouslySelected = currentProviderId();
    m_providerList->clear();
    for (const ProviderConfig& config : m_service.providerConfigs()) {
        auto* item = new QListWidgetItem(config.displayName, m_providerList);
        item->setData(Qt::UserRole, config.id);
        item->setToolTip(providerDisplayName(config.providerId));
    }

    if (!previouslySelected.isEmpty()) {
        selectProvider(previouslySelected);
    } else if (m_providerList->count() > 0) {
        m_providerList->setCurrentRow(0);
    } else {
        m_overviewPage->setOverview(std::nullopt);
        m_settingsPage->setProviderConfig(std::nullopt);
        m_usagePage->setUsagePoints({}, QStringLiteral("Add a provider to begin tracking usage."));
        m_logPage->setLogs({});
    }
}

void MainWindow::selectProvider(const QString& providerConfigId) {
    for (int row = 0; row < m_providerList->count(); ++row) {
        auto* item = m_providerList->item(row);
        if (item->data(Qt::UserRole).toString() == providerConfigId) {
            m_providerList->setCurrentRow(row);
            return;
        }
    }
}

QString MainWindow::currentProviderId() const {
    const QListWidgetItem* item = m_providerList->currentItem();
    return item ? item->data(Qt::UserRole).toString() : QString();
}

std::optional<ProviderConfig> MainWindow::currentConfig() const {
    const QString id = currentProviderId();
    if (id.isEmpty()) {
        return std::nullopt;
    }

    for (const ProviderConfig& config : m_service.providerConfigs()) {
        if (config.id == id) {
            return config;
        }
    }
    return std::nullopt;
}

void MainWindow::refreshViews() {
    const auto config = currentConfig();
    m_settingsPage->setProviderConfig(config);
    refreshOverview();
    refreshUsage();
    refreshLogs();
}

void MainWindow::refreshOverview() {
    const QString id = currentProviderId();
    if (id.isEmpty()) {
        m_overviewPage->setOverview(std::nullopt);
        return;
    }
    m_overviewPage->setOverview(m_service.cachedOverview(id));
}

void MainWindow::refreshUsage() {
    const QString id = currentProviderId();
    if (id.isEmpty()) {
        m_usagePage->setUsagePoints({}, QStringLiteral("No provider selected."));
        return;
    }

    if (!m_service.cachedOverview(id).has_value()) {
        m_usagePage->setUsagePoints({}, QStringLiteral("Refresh this provider to load usage data."));
        return;
    }

    const SyncResult result = m_service.loadUsage(id, m_usagePage->currentRange());
    m_usagePage->setUsagePoints(result.usagePoints, result.message);
}

void MainWindow::refreshLogs() {
    const QString id = currentProviderId();
    if (id.isEmpty()) {
        m_logPage->setLogs({});
        return;
    }
    m_logPage->setLogs(m_service.recentLogs(id, 30));
}

void MainWindow::saveCurrentProvider() {
    ProviderConfig config = m_settingsPage->providerConfig();
    if (config.id.isEmpty()) {
        statusBar()->showMessage(QStringLiteral("Select or create a provider first."), 5000);
        return;
    }

    QString error;
    if (!m_service.saveProviderConfig(config, m_settingsPage->apiKeyInput(), &error)) {
        QMessageBox::warning(this, QStringLiteral("Save failed"), error);
        return;
    }

    m_settingsPage->clearApiKeyInput();
    m_settingsPage->setStatusText(QStringLiteral("Settings saved."));
    reloadProviderList();
    selectProvider(config.id);
    statusBar()->showMessage(QStringLiteral("Settings saved."), 4000);
}

void MainWindow::testCurrentProvider() {
    const QString id = currentProviderId();
    if (id.isEmpty()) {
        statusBar()->showMessage(QStringLiteral("Select a provider first."), 4000);
        return;
    }

    const SyncResult result = m_service.testConnection(id);
    m_settingsPage->setStatusText(result.message);
    refreshOverview();
    refreshLogs();
    statusBar()->showMessage(result.message, 5000);
}

void MainWindow::refreshCurrentProvider() {
    const QString id = currentProviderId();
    if (id.isEmpty()) {
        statusBar()->showMessage(QStringLiteral("Select a provider first."), 4000);
        return;
    }

    const SyncResult result = m_service.refreshProvider(id);
    m_settingsPage->setStatusText(result.message);
    refreshOverview();
    refreshUsage();
    refreshLogs();
    statusBar()->showMessage(result.message, 5000);
}

void MainWindow::addProvider() {
    ProviderEditorDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    const ProviderConfig config = m_service.addProvider(dialog.selectedProvider(), dialog.displayName());
    reloadProviderList();
    selectProvider(config.id);
    m_settingsPage->setStatusText(QStringLiteral("Provider created. Add credentials and save settings."));
}

void MainWindow::deleteCurrentProvider() {
    const auto config = currentConfig();
    if (!config.has_value()) {
        return;
    }

    const QMessageBox::StandardButton answer = QMessageBox::question(
        this,
        QStringLiteral("Delete provider"),
        QStringLiteral("Delete '%1'?").arg(config->displayName));

    if (answer != QMessageBox::Yes) {
        return;
    }

    QString error;
    if (!m_service.removeProvider(config->id, &error)) {
        QMessageBox::warning(this, QStringLiteral("Delete failed"), error);
        return;
    }

    reloadProviderList();
    statusBar()->showMessage(QStringLiteral("Provider removed."), 4000);
}

void MainWindow::closeEvent(QCloseEvent* event) {
    m_service.saveWindowGeometry(saveGeometry());
    m_service.saveSplitterState(m_splitter->saveState());
    QMainWindow::closeEvent(event);
}

}  // namespace llm

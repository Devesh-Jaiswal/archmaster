#include "MainWindow.h"
#include "PackageView.h"
#include "AnalyticsView.h"
#include "ControlPanel.h"
#include "SearchView.h"
#include "UpdateManager.h"
#include "ProfileView.h"
#include "LoadingOverlay.h"
#include "core/PackageManager.h"
#include "core/Database.h"
#include "core/AURClient.h"
#include "core/ProfileManager.h"
#include "utils/Config.h"

#include <QApplication>
#include <QToolButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QCloseEvent>
#include <QStyle>
#include <QFile>
#include <QDebug>
#include <QShortcut>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_packageManager(std::make_unique<PackageManager>(this))
    , m_database(std::make_unique<Database>(this))
    , m_aurClient(std::make_unique<AURClient>(this))
    , m_profileManager(std::make_unique<ProfileManager>())
{
    setWindowTitle("ArchMaster - Package Manager");
    setMinimumSize(1000, 700);
    
    // Initialize core services
    if (!m_packageManager->initialize()) {
        QMessageBox::warning(this, "Initialization Error", 
            "Failed to initialize package manager:\n" + m_packageManager->lastError());
    }
    
    if (!m_database->initialize()) {
        QMessageBox::warning(this, "Initialization Error",
            "Failed to initialize database:\n" + m_database->lastError());
    }
    
    setupUI();
    setupToolbar();
    setupStatusBar();
    setupConnections();
    setupShortcuts();
    loadSettings();
    applyTheme();
    
    // Load initial data
    refreshPackages();
}

MainWindow::~MainWindow() {
    saveSettings();
}

void MainWindow::setupUI() {
    m_stackedWidget = new QStackedWidget(this);
    setCentralWidget(m_stackedWidget);
    
    // Create loading overlay
    m_loadingOverlay = new LoadingOverlay(m_stackedWidget);
    
    // Create views
    m_packageView = new PackageView(m_packageManager.get(), m_database.get(), m_aurClient.get(), this);
    m_analyticsView = new AnalyticsView(m_packageManager.get(), m_database.get(), this);
    m_controlPanel = new ControlPanel(m_packageManager.get(), m_database.get(), this);
    m_searchView = new SearchView(m_packageManager.get(), m_aurClient.get(), this);
    m_updateManager = new UpdateManager(m_packageManager.get(), this);
    m_profileView = new ProfileView(m_profileManager.get(), m_packageManager.get(), this);
    
    m_stackedWidget->addWidget(m_packageView);
    m_stackedWidget->addWidget(m_analyticsView);
    m_stackedWidget->addWidget(m_controlPanel);
    m_stackedWidget->addWidget(m_searchView);
    m_stackedWidget->addWidget(m_updateManager);
    m_stackedWidget->addWidget(m_profileView);
}

void MainWindow::setupToolbar() {
    m_toolbar = addToolBar("Main Toolbar");
    m_toolbar->setMovable(false);
    m_toolbar->setIconSize(QSize(32, 32));
    m_toolbar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    
    // Navigation actions
    m_actionPackages = m_toolbar->addAction("📦 Packages");
    m_actionPackages->setCheckable(true);
    m_actionPackages->setChecked(true);
    connect(m_actionPackages, &QAction::triggered, this, &MainWindow::showPackageView);
    
    m_actionAnalytics = m_toolbar->addAction("📊 Analytics");
    m_actionAnalytics->setCheckable(true);
    connect(m_actionAnalytics, &QAction::triggered, this, &MainWindow::showAnalyticsView);
    
    m_actionControl = m_toolbar->addAction("⚙️ Control");
    m_actionControl->setCheckable(true);
    connect(m_actionControl, &QAction::triggered, this, &MainWindow::showControlPanel);
    
    m_actionSearch = m_toolbar->addAction("🔍 Search");
    m_actionSearch->setCheckable(true);
    connect(m_actionSearch, &QAction::triggered, this, &MainWindow::showSearchView);
    
    m_actionUpdates = m_toolbar->addAction("⬆️ Updates");
    m_actionUpdates->setCheckable(true);
    connect(m_actionUpdates, &QAction::triggered, this, &MainWindow::showUpdateManager);
    
    m_actionProfiles = m_toolbar->addAction("📋 Profiles");
    m_actionProfiles->setCheckable(true);
    connect(m_actionProfiles, &QAction::triggered, this, &MainWindow::showProfileView);
    
    m_toolbar->addSeparator();
    
    // Actions
    m_actionRefresh = m_toolbar->addAction("🔄 Refresh");
    connect(m_actionRefresh, &QAction::triggered, this, &MainWindow::refreshPackages);
    
    m_toolbar->addSeparator();
    
    // Spacer
    QWidget* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_toolbar->addWidget(spacer);
    
    // Right side actions
    QAction* exportAction = m_toolbar->addAction("📤 Export");
    connect(exportAction, &QAction::triggered, this, &MainWindow::exportData);
    
    QAction* importAction = m_toolbar->addAction("📥 Import");
    connect(importAction, &QAction::triggered, this, &MainWindow::importData);
    
    m_toolbar->addSeparator();
    
    m_actionDarkMode = m_toolbar->addAction("🌙 Dark");
    m_actionDarkMode->setCheckable(true);
    m_actionDarkMode->setChecked(Config::instance()->darkMode());
    connect(m_actionDarkMode, &QAction::triggered, this, &MainWindow::toggleDarkMode);
    
    QAction* aboutAction = m_toolbar->addAction("ℹ️ About");
    connect(aboutAction, &QAction::triggered, this, &MainWindow::showAbout);
}

void MainWindow::setupStatusBar() {
    m_statusLabel = new QLabel("Ready");
    m_packageCountLabel = new QLabel();
    m_progressBar = new QProgressBar();
    m_progressBar->setMaximumWidth(200);
    m_progressBar->hide();
    
    statusBar()->addWidget(m_statusLabel, 1);
    statusBar()->addPermanentWidget(m_progressBar);
    statusBar()->addPermanentWidget(m_packageCountLabel);
}

void MainWindow::setupConnections() {
    connect(m_packageView, &PackageView::packageSelected, 
            this, &MainWindow::onPackageSelected);
    
    connect(m_packageManager.get(), &PackageManager::packagesChanged,
            this, &MainWindow::refreshPackages);
    
    connect(m_packageManager.get(), &PackageManager::operationProgress,
            this, [this](const QString& msg, int percent) {
                m_statusLabel->setText(msg);
                m_progressBar->setValue(percent);
                m_progressBar->show();
            });
    
    connect(m_packageManager.get(), &PackageManager::operationCompleted,
            this, [this](bool success, const QString& msg) {
                m_statusLabel->setText(msg);
                m_progressBar->hide();
                if (!success) {
                    QMessageBox::warning(this, "Operation Failed", msg);
                }
            });
    
    connect(Config::instance(), &Config::darkModeChanged,
            this, &MainWindow::applyTheme);
}

void MainWindow::showPackageView() {
    m_stackedWidget->setCurrentWidget(m_packageView);
    m_actionPackages->setChecked(true);
    m_actionAnalytics->setChecked(false);
    m_actionControl->setChecked(false);
    m_actionSearch->setChecked(false);
    m_actionUpdates->setChecked(false);
    m_actionProfiles->setChecked(false);
}

void MainWindow::showAnalyticsView() {
    m_stackedWidget->setCurrentWidget(m_analyticsView);
    m_actionPackages->setChecked(false);
    m_actionAnalytics->setChecked(true);
    m_actionControl->setChecked(false);
    m_actionSearch->setChecked(false);
    m_actionUpdates->setChecked(false);
    m_actionProfiles->setChecked(false);
    m_analyticsView->refresh();
}

void MainWindow::showControlPanel() {
    m_stackedWidget->setCurrentWidget(m_controlPanel);
    m_actionPackages->setChecked(false);
    m_actionAnalytics->setChecked(false);
    m_actionControl->setChecked(true);
    m_actionSearch->setChecked(false);
    m_actionUpdates->setChecked(false);
    m_actionProfiles->setChecked(false);
}

void MainWindow::showSearchView() {
    m_stackedWidget->setCurrentWidget(m_searchView);
    m_actionPackages->setChecked(false);
    m_actionAnalytics->setChecked(false);
    m_actionControl->setChecked(false);
    m_actionSearch->setChecked(true);
    m_actionUpdates->setChecked(false);
    m_actionProfiles->setChecked(false);
}

void MainWindow::showUpdateManager() {
    m_stackedWidget->setCurrentWidget(m_updateManager);
    m_actionPackages->setChecked(false);
    m_actionAnalytics->setChecked(false);
    m_actionControl->setChecked(false);
    m_actionSearch->setChecked(false);
    m_actionUpdates->setChecked(true);
    m_actionProfiles->setChecked(false);
    
    // Auto-check for updates when switching to this view
    m_updateManager->checkForUpdates();
}

void MainWindow::showProfileView() {
    m_stackedWidget->setCurrentWidget(m_profileView);
    m_actionPackages->setChecked(false);
    m_actionAnalytics->setChecked(false);
    m_actionControl->setChecked(false);
    m_actionSearch->setChecked(false);
    m_actionUpdates->setChecked(false);
    m_actionProfiles->setChecked(true);
}

void MainWindow::refreshPackages() {
    m_statusLabel->setText("Refreshing package database...");
    qApp->processEvents();
    
    // Re-initialize libalpm to get fresh data
    m_packageManager->refresh();
    
    m_packageView->loadPackages();
    updateStatusBar();
    
    m_statusLabel->setText("Ready");
}

void MainWindow::toggleDarkMode() {
    Config::instance()->setDarkMode(m_actionDarkMode->isChecked());
}

void MainWindow::applyTheme() {
    bool dark = Config::instance()->darkMode();
    
    if (dark) {
        qApp->setStyleSheet(R"(
            QMainWindow, QWidget {
                background-color: #1e1e2e;
                color: #cdd6f4;
            }
            QToolBar {
                background-color: #181825;
                border: none;
                padding: 5px;
                spacing: 10px;
            }
            QToolBar QToolButton {
                background-color: transparent;
                border: none;
                border-radius: 8px;
                padding: 8px 12px;
                color: #cdd6f4;
                font-size: 12px;
            }
            QToolBar QToolButton:hover {
                background-color: #313244;
            }
            QToolBar QToolButton:checked {
                background-color: #45475a;
            }
            QTableView {
                background-color: #1e1e2e;
                alternate-background-color: #181825;
                color: #cdd6f4;
                border: 1px solid #313244;
                border-radius: 8px;
                gridline-color: #313244;
            }
            QTableView::item:selected {
                background-color: #45475a;
            }
            QTableView QHeaderView::section {
                background-color: #181825;
                color: #cdd6f4;
                padding: 8px;
                border: none;
                border-bottom: 2px solid #313244;
                font-weight: bold;
            }
            QLineEdit {
                background-color: #313244;
                color: #cdd6f4;
                border: 2px solid #45475a;
                border-radius: 8px;
                padding: 8px 12px;
                font-size: 14px;
            }
            QLineEdit:focus {
                border-color: #89b4fa;
            }
            QPushButton {
                background-color: #45475a;
                color: #cdd6f4;
                border: none;
                border-radius: 8px;
                padding: 10px 20px;
                font-weight: bold;
            }
            QPushButton:hover {
                background-color: #585b70;
            }
            QPushButton:pressed {
                background-color: #313244;
            }
            QPushButton#primaryButton {
                background-color: #89b4fa;
                color: #1e1e2e;
            }
            QPushButton#dangerButton {
                background-color: #f38ba8;
                color: #1e1e2e;
            }
            QComboBox {
                background-color: #313244;
                color: #cdd6f4;
                border: 2px solid #45475a;
                border-radius: 8px;
                padding: 8px;
            }
            QScrollBar:vertical {
                background-color: #181825;
                width: 12px;
                border-radius: 6px;
            }
            QScrollBar::handle:vertical {
                background-color: #45475a;
                border-radius: 6px;
                min-height: 30px;
            }
            QScrollBar::handle:vertical:hover {
                background-color: #585b70;
            }
            QGroupBox {
                border: 2px solid #313244;
                border-radius: 8px;
                margin-top: 12px;
                padding-top: 10px;
                font-weight: bold;
            }
            QGroupBox::title {
                subcontrol-origin: margin;
                left: 15px;
                padding: 0 5px;
            }
            QTextEdit, QPlainTextEdit {
                background-color: #313244;
                color: #cdd6f4;
                border: 2px solid #45475a;
                border-radius: 8px;
                padding: 8px;
            }
            QTabWidget::pane {
                border: 2px solid #313244;
                border-radius: 8px;
            }
            QTabBar::tab {
                background-color: #181825;
                color: #cdd6f4;
                padding: 10px 20px;
                border-top-left-radius: 8px;
                border-top-right-radius: 8px;
            }
            QTabBar::tab:selected {
                background-color: #313244;
            }
            QStatusBar {
                background-color: #181825;
                color: #a6adc8;
            }
            QLabel#statCard {
                background-color: #313244;
                border-radius: 12px;
                padding: 20px;
            }
        )");
    } else {
        qApp->setStyleSheet(R"(
            QMainWindow, QWidget {
                background-color: #eff1f5;
                color: #4c4f69;
            }
            QToolBar {
                background-color: #e6e9ef;
                border: none;
                padding: 5px;
                spacing: 10px;
            }
            QToolBar QToolButton {
                background-color: transparent;
                border: none;
                border-radius: 8px;
                padding: 8px 12px;
                color: #4c4f69;
                font-size: 12px;
            }
            QToolBar QToolButton:hover {
                background-color: #ccd0da;
            }
            QToolBar QToolButton:checked {
                background-color: #bcc0cc;
            }
            QTableView {
                background-color: #eff1f5;
                alternate-background-color: #e6e9ef;
                color: #4c4f69;
                border: 1px solid #ccd0da;
                border-radius: 8px;
                gridline-color: #ccd0da;
            }
            QTableView::item:selected {
                background-color: #bcc0cc;
            }
            QLineEdit {
                background-color: #ffffff;
                color: #4c4f69;
                border: 2px solid #ccd0da;
                border-radius: 8px;
                padding: 8px 12px;
            }
            QPushButton {
                background-color: #ccd0da;
                color: #4c4f69;
                border: none;
                border-radius: 8px;
                padding: 10px 20px;
                font-weight: bold;
            }
        )");
    }
    
    m_actionDarkMode->setText(dark ? "🌙 Dark" : "☀️ Light");
    m_actionDarkMode->setChecked(dark);
}

void MainWindow::showAbout() {
    QMessageBox::about(this, "About ArchMaster",
        "<h2>ArchMaster</h2>"
        "<p>Version 1.0.0</p>"
        "<p>A modern package management dashboard for Arch Linux.</p>"
        "<p>Built with Qt6 and libalpm.</p>"
        "<hr>"
        "<p>Features:</p>"
        "<ul>"
        "<li>📦 Package browsing with search and filters</li>"
        "<li>📝 Personal notes and tagging</li>"
        "<li>📊 Disk usage analytics</li>"
        "<li>🔗 Dependency visualization</li>"
        "<li>🌐 AUR integration</li>"
        "</ul>");
}

void MainWindow::exportData() {
    QString filePath = QFileDialog::getSaveFileName(this,
        "Export Data", Config::instance()->exportPath() + "/archmaster_data.json",
        "JSON Files (*.json)");
    
    if (!filePath.isEmpty()) {
        if (m_database->exportToJson(filePath)) {
            Config::instance()->setExportPath(QFileInfo(filePath).absolutePath());
            QMessageBox::information(this, "Export Complete", 
                "Data exported successfully to:\n" + filePath);
        } else {
            QMessageBox::warning(this, "Export Failed", 
                "Failed to export data:\n" + m_database->lastError());
        }
    }
}

void MainWindow::importData() {
    QString filePath = QFileDialog::getOpenFileName(this,
        "Import Data", Config::instance()->exportPath(),
        "JSON Files (*.json)");
    
    if (!filePath.isEmpty()) {
        if (m_database->importFromJson(filePath)) {
            QMessageBox::information(this, "Import Complete", 
                "Data imported successfully!");
            refreshPackages();
        } else {
            QMessageBox::warning(this, "Import Failed", 
                "Failed to import data:\n" + m_database->lastError());
        }
    }
}

void MainWindow::showSettings() {
    // TODO: Settings dialog
}

void MainWindow::onPackageSelected(const QString& packageName) {
    m_statusLabel->setText("Selected: " + packageName);
}

void MainWindow::updateStatusBar() {
    int total = m_packageManager->totalPackageCount();
    int explicit_count = m_packageManager->explicitPackageCount();
    int orphans = m_packageManager->orphanPackageCount();
    
    m_packageCountLabel->setText(QString("📦 %1 total | ✅ %2 explicit | ⚠️ %3 orphans")
        .arg(total).arg(explicit_count).arg(orphans));
}

void MainWindow::loadSettings() {
    Config* config = Config::instance();
    
    resize(config->windowSize());
    move(config->windowPosition());
    
    if (config->isMaximized()) {
        showMaximized();
    }
}

void MainWindow::saveSettings() {
    Config* config = Config::instance();
    
    config->setMaximized(isMaximized());
    if (!isMaximized()) {
        config->setWindowSize(size());
        config->setWindowPosition(pos());
    }
}

void MainWindow::setupShortcuts() {
    // Navigation shortcuts
    QShortcut* shortcutPackages = new QShortcut(QKeySequence("Ctrl+1"), this);
    connect(shortcutPackages, &QShortcut::activated, this, &MainWindow::showPackageView);
    
    QShortcut* shortcutAnalytics = new QShortcut(QKeySequence("Ctrl+2"), this);
    connect(shortcutAnalytics, &QShortcut::activated, this, &MainWindow::showAnalyticsView);
    
    QShortcut* shortcutControl = new QShortcut(QKeySequence("Ctrl+3"), this);
    connect(shortcutControl, &QShortcut::activated, this, &MainWindow::showControlPanel);
    
    // Action shortcuts
    QShortcut* shortcutRefresh = new QShortcut(QKeySequence("F5"), this);
    connect(shortcutRefresh, &QShortcut::activated, this, &MainWindow::refreshPackages);
    
    QShortcut* shortcutRefreshAlt = new QShortcut(QKeySequence("Ctrl+R"), this);
    connect(shortcutRefreshAlt, &QShortcut::activated, this, &MainWindow::refreshPackages);
    
    // Theme toggle
    QShortcut* shortcutTheme = new QShortcut(QKeySequence("Ctrl+D"), this);
    connect(shortcutTheme, &QShortcut::activated, this, [this]() {
        m_actionDarkMode->setChecked(!m_actionDarkMode->isChecked());
        toggleDarkMode();
    });
    
    // Export/Import
    QShortcut* shortcutExport = new QShortcut(QKeySequence("Ctrl+E"), this);
    connect(shortcutExport, &QShortcut::activated, this, &MainWindow::exportData);
    
    QShortcut* shortcutImport = new QShortcut(QKeySequence("Ctrl+I"), this);
    connect(shortcutImport, &QShortcut::activated, this, &MainWindow::importData);
    
    // Quit
    QShortcut* shortcutQuit = new QShortcut(QKeySequence("Ctrl+Q"), this);
    connect(shortcutQuit, &QShortcut::activated, this, &QMainWindow::close);
    
    // Focus search (will be handled by PackageView)
    QShortcut* shortcutSearch = new QShortcut(QKeySequence("Ctrl+F"), this);
    connect(shortcutSearch, &QShortcut::activated, this, [this]() {
        showPackageView();
        // PackageView will handle focus on its search field
    });
}

void MainWindow::closeEvent(QCloseEvent* event) {
    saveSettings();
    event->accept();
}

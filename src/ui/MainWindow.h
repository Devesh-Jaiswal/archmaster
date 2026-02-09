#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QToolBar>
#include <QStatusBar>
#include <QLabel>
#include <QProgressBar>
#include <memory>

class PackageManager;
class Database;
class AURClient;
class PackageView;
class AnalyticsView;
class ControlPanel;
class SearchView;
class UpdateManager;
class ProfileView;
class ProfileManager;
class LoadingOverlay;

class MainWindow : public QMainWindow {
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();
    
protected:
    void closeEvent(QCloseEvent* event) override;
    
private slots:
    void showPackageView();
    void showAnalyticsView();
    void showControlPanel();
    void showSearchView();
    void showUpdateManager();
    void showProfileView();
    void refreshPackages();
    void toggleDarkMode();
    void showAbout();
    void showSettings();
    
    void onPackageSelected(const QString& packageName);
    void updateStatusBar();
    
private:
    void setupUI();
    void setupToolbar();
    void setupStatusBar();
    void setupConnections();
    void setupShortcuts();
    void applyTheme();
    void loadSettings();
    void saveSettings();
    
    // Core services
    std::unique_ptr<PackageManager> m_packageManager;
    std::unique_ptr<Database> m_database;
    std::unique_ptr<AURClient> m_aurClient;
    
    // UI
    QStackedWidget* m_stackedWidget;
    QToolBar* m_toolbar;
    
    PackageView* m_packageView;
    AnalyticsView* m_analyticsView;
    ControlPanel* m_controlPanel;
    SearchView* m_searchView;
    UpdateManager* m_updateManager;
    ProfileView* m_profileView;
    std::unique_ptr<ProfileManager> m_profileManager;
    
    // Status bar widgets
    QLabel* m_statusLabel;
    QLabel* m_packageCountLabel;
    QProgressBar* m_progressBar;
    
    // Loading overlay
    LoadingOverlay* m_loadingOverlay;
    
    // Toolbar actions
    QAction* m_actionPackages;
    QAction* m_actionAnalytics;
    QAction* m_actionControl;
    QAction* m_actionRefresh;
    QAction* m_actionDarkMode;
    QAction* m_actionSearch;
    QAction* m_actionUpdates;
    QAction* m_actionProfiles;
};

#endif // MAINWINDOW_H

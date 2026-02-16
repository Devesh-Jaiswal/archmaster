#ifndef ANALYTICSVIEW_H
#define ANALYTICSVIEW_H

#include <QWidget>
#include <QLabel>
#include <QTableWidget>
#include <QChartView>
#include <QPieSeries>
#include <QBarSeries>
#include <QPushButton>
#include <QDateTime>
#include <QElapsedTimer>

class PackageManager;
class Database;

#include "models/Package.h"

class AnalyticsView : public QWidget {
    Q_OBJECT
    
public:
    explicit AnalyticsView(PackageManager* pm, Database* db, QWidget* parent = nullptr);
    
    void applyTheme(bool isDark);
    void refresh();
    
private slots:
    void onCleanOrphans();
    void onCleanCache();
    void refreshInBackground();
    
private:
    void setupUI();
    void updateStats();
    void updateHealthStatus();
    void updateDiskUsageChart();
    void updateTimelineChart();
    void updateTopPackages();
    void updateOrphansList();
    void updateRecentlyUpdated();
    void updateAurVsRepo();
    
    void expandDiskUsageChart();
    void expandTimelineChart();
    
    // Health check methods
    QDateTime getLastSyncTime();
    qint64 getCacheSize();
    void cachePackageData();
    
    PackageManager* m_packageManager;
    Database* m_database;
    
    // Health status cards
    QLabel* m_lastSyncLabel;
    QLabel* m_cacheSizeLabel;
    QLabel* m_orphansSummaryLabel;
    QPushButton* m_cleanOrphansBtn;
    QPushButton* m_cleanCacheBtn;
    
    // Stats cards
    QLabel* m_totalPackagesLabel;
    QLabel* m_explicitLabel;
    QLabel* m_depsLabel;
    QLabel* m_orphansLabel;
    QLabel* m_totalSizeLabel;
    QLabel* m_notesCountLabel;
    
    // Charts
    QChartView* m_diskUsageChart;
    QChartView* m_timelineChart;
    
    // Tables
    QTableWidget* m_topPackagesTable;
    QTableWidget* m_orphansTable;
    QTableWidget* m_recentlyUpdatedTable;
    
    // AUR stats labels
    QLabel* m_aurCountLabel;
    QLabel* m_repoCountLabel;
    
    // Cached data

    
    // Caching state
    bool m_dataLoaded = false;
    QElapsedTimer m_lastRefresh;
    QList<Package> m_cachedPackages;
};

#endif // ANALYTICSVIEW_H

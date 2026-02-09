#include "AnalyticsView.h"
#include "core/PackageManager.h"
#include "core/Database.h"
#include "models/Package.h"
#include "PrivilegedRunner.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QScrollArea>
#include <QHeaderView>
#include <QPushButton>
#include <QHeaderView>
#include <QtCharts/QChart>
#include <QtCharts/QPieSeries>
#include <QtCharts/QPieSlice>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>
#include <QtCharts/QDateTimeAxis>
#include <QtCharts/QLineSeries>
#include <algorithm>
#include "ChartPopup.h"
#include <QProcess>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>

AnalyticsView::AnalyticsView(PackageManager* pm, Database* db, QWidget* parent)
    : QWidget(parent)
    , m_packageManager(pm)
    , m_database(db)
{
    setupUI();
}

void AnalyticsView::setupUI() {
    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    
    QWidget* content = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout(content);
    mainLayout->setSpacing(20);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    
    // ==================== SYSTEM HEALTH SECTION ====================
    QGroupBox* healthGroup = new QGroupBox("🏥 System Health");
    healthGroup->setStyleSheet(R"(
        QGroupBox {
            font-size: 16px;
            font-weight: bold;
            padding-top: 10px;
        }
    )");
    QHBoxLayout* healthLayout = new QHBoxLayout(healthGroup);
    healthLayout->setSpacing(15);
    
    // Helper to create health status cards with action button
    auto createHealthCard = [](const QString& icon, const QString& title, 
                                QLabel*& statusLabel, QPushButton*& actionBtn,
                                const QString& btnText, const QString& color) -> QWidget* {
        QWidget* card = new QWidget();
        card->setObjectName("healthCard");
        card->setStyleSheet(QString(R"(
            QWidget#healthCard {
                background-color: #313244;
                border-radius: 12px;
                border-left: 4px solid %1;
                padding: 10px;
            }
        )").arg(color));
        
        QVBoxLayout* layout = new QVBoxLayout(card);
        layout->setSpacing(8);
        
        QLabel* titleLabel = new QLabel(icon + " " + title);
        titleLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #cdd6f4;");
        
        statusLabel = new QLabel("Loading...");
        statusLabel->setStyleSheet("font-size: 12px; color: #a6adc8;");
        statusLabel->setWordWrap(true);
        
        actionBtn = new QPushButton(btnText);
        actionBtn->setStyleSheet(QString(R"(
            QPushButton {
                background-color: %1;
                color: #1e1e2e;
                border: none;
                border-radius: 6px;
                padding: 6px 12px;
                font-weight: bold;
            }
            QPushButton:hover {
                background-color: #cdd6f4;
            }
        )").arg(color));
        actionBtn->setMaximumWidth(120);
        
        layout->addWidget(titleLabel);
        layout->addWidget(statusLabel);
        layout->addWidget(actionBtn);
        
        return card;
    };
    
    // Last Sync card (no action button)
    QWidget* syncCard = new QWidget();
    syncCard->setObjectName("healthCard");
    syncCard->setStyleSheet(R"(
        QWidget#healthCard {
            background-color: #313244;
            border-radius: 12px;
            border-left: 4px solid #89b4fa;
            padding: 10px;
        }
    )");
    QVBoxLayout* syncLayout = new QVBoxLayout(syncCard);
    QLabel* syncTitle = new QLabel("🕐 Last Sync");
    syncTitle->setStyleSheet("font-size: 14px; font-weight: bold; color: #cdd6f4;");
    m_lastSyncLabel = new QLabel("Loading...");
    m_lastSyncLabel->setStyleSheet("font-size: 12px; color: #a6adc8;");
    syncLayout->addWidget(syncTitle);
    syncLayout->addWidget(m_lastSyncLabel);
    syncLayout->addStretch();
    healthLayout->addWidget(syncCard);
    
    // Pacnew files card
    healthLayout->addWidget(createHealthCard("📄", ".pacnew Files", 
        m_pacnewLabel, m_viewPacnewBtn, "View", "#f9e2af"));
    connect(m_viewPacnewBtn, &QPushButton::clicked, this, &AnalyticsView::onViewPacnewFiles);
    
    // Cache size card
    healthLayout->addWidget(createHealthCard("💾", "Package Cache",
        m_cacheSizeLabel, m_cleanCacheBtn, "Clean", "#94e2d5"));
    connect(m_cleanCacheBtn, &QPushButton::clicked, this, &AnalyticsView::onCleanCache);
    
    // Orphans card
    healthLayout->addWidget(createHealthCard("⚠️", "Orphan Packages",
        m_orphansSummaryLabel, m_cleanOrphansBtn, "Clean All", "#fab387"));
    connect(m_cleanOrphansBtn, &QPushButton::clicked, this, &AnalyticsView::onCleanOrphans);
    
    mainLayout->addWidget(healthGroup);
    
    // ==================== STATS CARDS SECTION ====================
    QHBoxLayout* statsLayout = new QHBoxLayout();
    statsLayout->setSpacing(15);
    
    auto createStatCard = [](const QString& title, QLabel*& valueLabel) -> QWidget* {
        QWidget* card = new QWidget();
        card->setObjectName("statCard");
        card->setStyleSheet(R"(
            QWidget#statCard {
                background-color: #313244;
                border-radius: 12px;
                padding: 15px;
            }
        )");
        
        QVBoxLayout* layout = new QVBoxLayout(card);
        layout->setAlignment(Qt::AlignCenter);
        
        valueLabel = new QLabel("0");
        valueLabel->setStyleSheet("font-size: 32px; font-weight: bold; color: #89b4fa;");
        valueLabel->setAlignment(Qt::AlignCenter);
        
        QLabel* titleLabel = new QLabel(title);
        titleLabel->setStyleSheet("font-size: 12px; color: #a6adc8;");
        titleLabel->setAlignment(Qt::AlignCenter);
        
        layout->addWidget(valueLabel);
        layout->addWidget(titleLabel);
        
        return card;
    };
    
    statsLayout->addWidget(createStatCard("📦 Total Packages", m_totalPackagesLabel));
    statsLayout->addWidget(createStatCard("✅ Explicit", m_explicitLabel));
    statsLayout->addWidget(createStatCard("🔗 Dependencies", m_depsLabel));
    statsLayout->addWidget(createStatCard("⚠️ Orphans", m_orphansLabel));
    statsLayout->addWidget(createStatCard("💾 Total Size", m_totalSizeLabel));
    statsLayout->addWidget(createStatCard("📝 With Notes", m_notesCountLabel));
    
    mainLayout->addLayout(statsLayout);
    
    // Charts row
    QHBoxLayout* chartsLayout = new QHBoxLayout();
    chartsLayout->setSpacing(15);
    
    // Disk usage pie chart
    QGroupBox* diskGroup = new QGroupBox("💾 Disk Usage by Category");
    QVBoxLayout* diskLayout = new QVBoxLayout(diskGroup);
    
    m_diskUsageChart = new QChartView();
    m_diskUsageChart->setRenderHint(QPainter::Antialiasing);
    m_diskUsageChart->setMinimumHeight(400);
    diskLayout->addWidget(m_diskUsageChart);
    
    chartsLayout->addWidget(diskGroup);
    
    // Timeline chart
    QGroupBox* timelineGroup = new QGroupBox("📅 Installation Timeline");
    QVBoxLayout* timelineLayout = new QVBoxLayout(timelineGroup);
    
    m_timelineChart = new QChartView();
    m_timelineChart->setRenderHint(QPainter::Antialiasing);
    m_timelineChart->setMinimumHeight(400);
    timelineLayout->addWidget(m_timelineChart);
    
    chartsLayout->addWidget(timelineGroup);
    
    mainLayout->addLayout(chartsLayout);
    
    // Tables row
    QHBoxLayout* tablesLayout = new QHBoxLayout();
    tablesLayout->setSpacing(15);
    
    // Top packages by size
    QGroupBox* topGroup = new QGroupBox("🏆 Top 10 Largest Packages");
    QVBoxLayout* topLayout = new QVBoxLayout(topGroup);
    
    m_topPackagesTable = new QTableWidget();
    m_topPackagesTable->setColumnCount(2);
    m_topPackagesTable->setHorizontalHeaderLabels({"Package", "Size"});
    m_topPackagesTable->horizontalHeader()->setStretchLastSection(true);
    m_topPackagesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_topPackagesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    topLayout->addWidget(m_topPackagesTable);
    
    tablesLayout->addWidget(topGroup);
    
    // Orphans list
    QGroupBox* orphansGroup = new QGroupBox("⚠️ Orphan Packages");
    QVBoxLayout* orphansLayout = new QVBoxLayout(orphansGroup);
    
    QLabel* orphansHelp = new QLabel("These packages were installed as dependencies but are no longer required by any package.");
    orphansHelp->setWordWrap(true);
    orphansHelp->setStyleSheet("color: #f9e2af; font-size: 11px; margin-bottom: 10px;");
    orphansLayout->addWidget(orphansHelp);
    
    m_orphansTable = new QTableWidget();
    m_orphansTable->setColumnCount(3);
    m_orphansTable->setHorizontalHeaderLabels({"Package", "Size", "Installed"});
    m_orphansTable->horizontalHeader()->setStretchLastSection(true);
    m_orphansTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_orphansTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    orphansLayout->addWidget(m_orphansTable);
    
    tablesLayout->addWidget(orphansGroup);
    
    mainLayout->addLayout(tablesLayout);
    
    // Second row of tables - Recently Updated and AUR Stats
    QHBoxLayout* tables2Layout = new QHBoxLayout();
    tables2Layout->setSpacing(15);
    
    // Recently Updated Packages
    QGroupBox* recentGroup = new QGroupBox("🕐 Recently Updated (Last 7 Days)");
    QVBoxLayout* recentLayout = new QVBoxLayout(recentGroup);
    
    m_recentlyUpdatedTable = new QTableWidget();
    m_recentlyUpdatedTable->setColumnCount(3);
    m_recentlyUpdatedTable->setHorizontalHeaderLabels({"Package", "Version", "Updated"});
    m_recentlyUpdatedTable->horizontalHeader()->setStretchLastSection(true);
    m_recentlyUpdatedTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_recentlyUpdatedTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    recentLayout->addWidget(m_recentlyUpdatedTable);
    
    tables2Layout->addWidget(recentGroup);
    
    // AUR vs Repo Stats
    QGroupBox* aurGroup = new QGroupBox("📊 Package Sources");
    QVBoxLayout* aurLayout = new QVBoxLayout(aurGroup);
    aurLayout->setSpacing(15);
    
    // Repo count
    QHBoxLayout* repoRow = new QHBoxLayout();
    QLabel* repoIcon = new QLabel("📦");
    repoIcon->setStyleSheet("font-size: 24px;");
    QVBoxLayout* repoInfo = new QVBoxLayout();
    repoInfo->addWidget(new QLabel("Official Repositories"));
    m_repoCountLabel = new QLabel("Loading...");
    m_repoCountLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #89b4fa;");
    repoInfo->addWidget(m_repoCountLabel);
    repoRow->addWidget(repoIcon);
    repoRow->addLayout(repoInfo);
    repoRow->addStretch();
    aurLayout->addLayout(repoRow);
    
    // AUR count
    QHBoxLayout* aurRow = new QHBoxLayout();
    QLabel* aurIcon = new QLabel("🌐");
    aurIcon->setStyleSheet("font-size: 24px;");
    QVBoxLayout* aurInfo = new QVBoxLayout();
    aurInfo->addWidget(new QLabel("AUR Packages"));
    m_aurCountLabel = new QLabel("Loading...");
    m_aurCountLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #f9e2af;");
    aurInfo->addWidget(m_aurCountLabel);
    aurRow->addWidget(aurIcon);
    aurRow->addLayout(aurInfo);
    aurRow->addStretch();
    aurLayout->addLayout(aurRow);
    
    aurLayout->addStretch();
    
    tables2Layout->addWidget(aurGroup);
    
    mainLayout->addLayout(tables2Layout);
    mainLayout->addStretch();
    
    scrollArea->setWidget(content);
    
    QVBoxLayout* wrapperLayout = new QVBoxLayout(this);
    wrapperLayout->setContentsMargins(0, 0, 0, 0);
    wrapperLayout->addWidget(scrollArea);
}

void AnalyticsView::refresh() {
    updateHealthStatus();
    updateStats();
    updateDiskUsageChart();
    updateTimelineChart();
    updateTopPackages();
    updateOrphansList();
    updateRecentlyUpdated();
    updateAurVsRepo();
}

void AnalyticsView::updateStats() {
    m_totalPackagesLabel->setText(QString::number(m_packageManager->totalPackageCount()));
    m_explicitLabel->setText(QString::number(m_packageManager->explicitPackageCount()));
    m_depsLabel->setText(QString::number(m_packageManager->dependencyPackageCount()));
    m_orphansLabel->setText(QString::number(m_packageManager->orphanPackageCount()));
    
    // Format total size
    qint64 totalSize = m_packageManager->totalInstalledSize();
    QString sizeStr;
    if (totalSize < 1024 * 1024 * 1024) {
        sizeStr = QString::number(totalSize / (1024.0 * 1024.0), 'f', 1) + " MB";
    } else {
        sizeStr = QString::number(totalSize / (1024.0 * 1024.0 * 1024.0), 'f', 2) + " GB";
    }
    m_totalSizeLabel->setText(sizeStr);
    
    m_notesCountLabel->setText(QString::number(m_database->countPackagesWithNotes()));
}

void AnalyticsView::updateDiskUsageChart() {
    QList<Package> packages = m_packageManager->getAllPackages();
    
    // Calculate size by reason
    qint64 explicitSize = 0;
    qint64 depsSize = 0;
    
    for (const Package& pkg : packages) {
        if (pkg.isExplicit()) {
            explicitSize += pkg.installedSize;
        } else {
            depsSize += pkg.installedSize;
        }
    }
    
    QPieSeries* series = new QPieSeries();
    
    QPieSlice* explicitSlice = series->append("Explicit", explicitSize);
    explicitSlice->setColor(QColor("#89b4fa"));
    explicitSlice->setLabelVisible(true);
    explicitSlice->setLabel(QString("Explicit\n%1 GB").arg(explicitSize / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2));
    
    QPieSlice* depsSlice = series->append("Dependencies", depsSize);
    depsSlice->setColor(QColor("#a6e3a1"));
    depsSlice->setLabelVisible(true);
    depsSlice->setLabel(QString("Dependencies\n%1 GB").arg(depsSize / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2));
    
    QChart* chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("");
    chart->legend()->setVisible(false);
    chart->setBackgroundVisible(false);
    chart->setMargins(QMargins(0, 0, 0, 0));
    
    m_diskUsageChart->setChart(chart);
}

void AnalyticsView::updateTimelineChart() {
    QList<Package> packages = m_packageManager->getAllPackages();
    
    // Group by month
    QMap<QString, int> monthCounts;
    for (const Package& pkg : packages) {
        QString month = pkg.installDate.toString("yyyy-MM");
        monthCounts[month]++;
    }
    
    // Sort by date and take last 12 months
    QStringList months = monthCounts.keys();
    std::sort(months.begin(), months.end());
    if (months.size() > 12) {
        months = months.mid(months.size() - 12);
    }
    
    QBarSet* set = new QBarSet("Packages Installed");
    set->setColor(QColor("#cba6f7"));
    
    QStringList categories;
    for (const QString& month : months) {
        *set << monthCounts[month];
        categories << month.mid(5);  // Just show MM
    }
    
    QBarSeries* series = new QBarSeries();
    series->append(set);
    
    QChart* chart = new QChart();
    chart->addSeries(series);
    chart->setBackgroundVisible(false);
    chart->setMargins(QMargins(0, 0, 0, 0));
    chart->legend()->setVisible(false);
    
    QBarCategoryAxis* axisX = new QBarCategoryAxis();
    axisX->append(categories);
    axisX->setLabelsColor(QColor("#a6adc8"));
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);
    
    QValueAxis* axisY = new QValueAxis();
    axisY->setLabelsColor(QColor("#a6adc8"));
    axisY->setGridLineColor(QColor("#45475a"));
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);
    
    m_timelineChart->setChart(chart);
}

void AnalyticsView::updateTopPackages() {
    QMap<QString, qint64> sizes = m_packageManager->getSizeByPackage();
    
    // Sort by size
    QList<QPair<QString, qint64>> sorted;
    for (auto it = sizes.begin(); it != sizes.end(); ++it) {
        sorted.append(qMakePair(it.key(), it.value()));
    }
    std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) {
        return a.second > b.second;
    });
    
    // Take top 10
    m_topPackagesTable->setRowCount(qMin(10, sorted.size()));
    for (int i = 0; i < qMin(10, sorted.size()); ++i) {
        m_topPackagesTable->setItem(i, 0, new QTableWidgetItem(sorted[i].first));
        
        QString sizeStr;
        qint64 size = sorted[i].second;
        if (size < 1024 * 1024) {
            sizeStr = QString::number(size / 1024.0, 'f', 1) + " KB";
        } else if (size < 1024 * 1024 * 1024) {
            sizeStr = QString::number(size / (1024.0 * 1024.0), 'f', 1) + " MB";
        } else {
            sizeStr = QString::number(size / (1024.0 * 1024.0 * 1024.0), 'f', 2) + " GB";
        }
        m_topPackagesTable->setItem(i, 1, new QTableWidgetItem(sizeStr));
    }
}

void AnalyticsView::updateOrphansList() {
    QList<Package> orphans = m_packageManager->getOrphanPackages();
    
    // Sort by size
    std::sort(orphans.begin(), orphans.end(), [](const Package& a, const Package& b) {
        return a.installedSize > b.installedSize;
    });
    
    m_orphansTable->setRowCount(orphans.size());
    for (int i = 0; i < orphans.size(); ++i) {
        m_orphansTable->setItem(i, 0, new QTableWidgetItem(orphans[i].name));
        m_orphansTable->setItem(i, 1, new QTableWidgetItem(orphans[i].formattedSize()));
        m_orphansTable->setItem(i, 2, new QTableWidgetItem(orphans[i].installDate.toString("yyyy-MM-dd")));
    }
}
void AnalyticsView::expandDiskUsageChart() {
    QList<Package> packages = m_packageManager->getAllPackages();
    
    qint64 explicitSize = 0;
    qint64 depsSize = 0;
    
    for (const Package& pkg : packages) {
        if (pkg.isExplicit()) {
            explicitSize += pkg.installedSize;
        } else {
            depsSize += pkg.installedSize;
        }
    }
    
    QPieSeries* series = new QPieSeries();
    
    QPieSlice* explicitSlice = series->append("Explicit", explicitSize);
    explicitSlice->setColor(QColor("#89b4fa"));
    explicitSlice->setLabelVisible(true);
    explicitSlice->setLabel(QString("Explicit\n%1 GB").arg(explicitSize / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2));
    
    QPieSlice* depsSlice = series->append("Dependencies", depsSize);
    depsSlice->setColor(QColor("#a6e3a1"));
    depsSlice->setLabelVisible(true);
    depsSlice->setLabel(QString("Dependencies\n%1 GB").arg(depsSize / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2));
    
    QChart* chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("Disk Usage by Category");
    chart->legend()->setVisible(true);
    chart->setBackgroundBrush(QBrush(QColor("#1e1e2e")));
    chart->setTitleBrush(QBrush(QColor("#cdd6f4")));
    
    ChartPopup* popup = new ChartPopup(chart, "Disk Usage - Expanded View", this);
    popup->exec();
    delete popup;
}

void AnalyticsView::expandTimelineChart() {
    QList<Package> packages = m_packageManager->getAllPackages();
    
    QMap<QString, int> monthCounts;
    for (const Package& pkg : packages) {
        QString month = pkg.installDate.toString("yyyy-MM");
        monthCounts[month]++;
    }
    
    QStringList months = monthCounts.keys();
    std::sort(months.begin(), months.end());
    if (months.size() > 24) {
        months = months.mid(months.size() - 24);
    }
    
    QBarSet* set = new QBarSet("Packages Installed");
    set->setColor(QColor("#cba6f7"));
    
    QStringList categories;
    for (const QString& month : months) {
        *set << monthCounts[month];
        categories << month;
    }
    
    QBarSeries* series = new QBarSeries();
    series->append(set);
    
    QChart* chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("Installation Timeline (Last 24 Months)");
    chart->setBackgroundBrush(QBrush(QColor("#1e1e2e")));
    chart->setTitleBrush(QBrush(QColor("#cdd6f4")));
    chart->legend()->setVisible(false);
    
    QBarCategoryAxis* axisX = new QBarCategoryAxis();
    axisX->append(categories);
    axisX->setLabelsColor(QColor("#a6adc8"));
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);
    
    QValueAxis* axisY = new QValueAxis();
    axisY->setLabelsColor(QColor("#a6adc8"));
    axisY->setGridLineColor(QColor("#45475a"));
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);
    
    ChartPopup* popup = new ChartPopup(chart, "Installation Timeline - Expanded View", this);
    popup->exec();
    delete popup;
}

// ==================== HEALTH STATUS METHODS ====================

void AnalyticsView::updateHealthStatus() {
    // Last sync time
    QDateTime syncTime = getLastSyncTime();
    if (syncTime.isValid()) {
        qint64 daysAgo = syncTime.daysTo(QDateTime::currentDateTime());
        QString status;
        if (daysAgo == 0) {
            status = "Today";
        } else if (daysAgo == 1) {
            status = "Yesterday";
        } else {
            status = QString("%1 days ago").arg(daysAgo);
        }
        m_lastSyncLabel->setText(syncTime.toString("MMM d, yyyy") + "\n(" + status + ")");
    } else {
        m_lastSyncLabel->setText("Unknown");
    }
    
    // Pacnew files
    m_pacnewFiles = findPacnewFiles();
    if (m_pacnewFiles.isEmpty()) {
        m_pacnewLabel->setText("✅ No files need merging");
        m_viewPacnewBtn->setEnabled(false);
    } else {
        m_pacnewLabel->setText(QString("🔴 %1 file(s) need attention").arg(m_pacnewFiles.size()));
        m_viewPacnewBtn->setEnabled(true);
    }
    
    // Cache size
    qint64 cacheSize = getCacheSize();
    QString cacheSizeStr;
    if (cacheSize < 1024 * 1024 * 1024) {
        cacheSizeStr = QString::number(cacheSize / (1024.0 * 1024.0), 'f', 1) + " MB";
    } else {
        cacheSizeStr = QString::number(cacheSize / (1024.0 * 1024.0 * 1024.0), 'f', 2) + " GB";
    }
    m_cacheSizeLabel->setText(cacheSizeStr);
    m_cleanCacheBtn->setEnabled(cacheSize > 0);
    
    // Orphans summary
    int orphanCount = m_packageManager->orphanPackageCount();
    if (orphanCount == 0) {
        m_orphansSummaryLabel->setText("✅ No orphaned packages");
        m_cleanOrphansBtn->setEnabled(false);
    } else {
        m_orphansSummaryLabel->setText(QString("⚠️ %1 package(s) can be removed").arg(orphanCount));
        m_cleanOrphansBtn->setEnabled(true);
    }
}

QDateTime AnalyticsView::getLastSyncTime() {
    QDir syncDir("/var/lib/pacman/sync");
    if (!syncDir.exists()) return QDateTime();
    
    QDateTime latestTime;
    QStringList dbFiles = syncDir.entryList({"*.db"}, QDir::Files);
    
    for (const QString& file : dbFiles) {
        QFileInfo info(syncDir.filePath(file));
        if (!latestTime.isValid() || info.lastModified() > latestTime) {
            latestTime = info.lastModified();
        }
    }
    
    return latestTime;
}

QStringList AnalyticsView::findPacnewFiles() {
    QStringList pacnewFiles;
    
    QProcess proc;
    proc.start("find", {"/etc", "-name", "*.pacnew", "-type", "f"});
    proc.waitForFinished(5000);
    
    QString output = QString::fromUtf8(proc.readAllStandardOutput());
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    
    for (const QString& line : lines) {
        if (!line.trimmed().isEmpty()) {
            pacnewFiles.append(line.trimmed());
        }
    }
    
    return pacnewFiles;
}

qint64 AnalyticsView::getCacheSize() {
    QProcess proc;
    proc.start("du", {"-sb", "/var/cache/pacman/pkg"});
    proc.waitForFinished(10000);
    
    QString output = QString::fromUtf8(proc.readAllStandardOutput());
    QStringList parts = output.split('\t');
    
    if (!parts.isEmpty()) {
        return parts.first().toLongLong();
    }
    
    return 0;
}

// ==================== ACTION HANDLERS ====================

void AnalyticsView::onCleanOrphans() {
    int orphanCount = m_packageManager->orphanPackageCount();
    if (orphanCount == 0) {
        QMessageBox::information(this, "No Orphans", "There are no orphaned packages to remove.");
        return;
    }
    
    QMessageBox::StandardButton reply = QMessageBox::question(this, "Clean Orphans",
        QString("Remove %1 orphaned package(s)?\n\n"
                "This will free up disk space by removing\n"
                "packages that were installed as dependencies\n"
                "but are no longer needed.").arg(orphanCount),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        bool success = PrivilegedRunner::runCommand(
            "pacman -Rns $(pacman -Qtdq) --noconfirm",
            QString("Removing %1 orphaned package(s)").arg(orphanCount),
            this);
        
        if (success) {
            QMessageBox::information(this, "Success", 
                "Orphaned packages removed successfully!\n\nRefreshing dashboard...");
            refresh();
        }
    }
}

void AnalyticsView::onViewPacnewFiles() {
    if (m_pacnewFiles.isEmpty()) {
        QMessageBox::information(this, "No .pacnew Files", "There are no .pacnew files that need attention.");
        return;
    }
    
    QString message = ".pacnew files found:\n\n";
    for (const QString& file : m_pacnewFiles) {
        message += "• " + file + "\n";
    }
    message += "\nWould you like to run pacdiff to review and merge these files?";
    
    QMessageBox::StandardButton reply = QMessageBox::question(this, ".pacnew Files", message,
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        bool success = PrivilegedRunner::runCommand(
            "pacdiff",
            "Reviewing .pacnew configuration files",
            this);
        
        if (success) {
            refresh();
        }
    }
}

void AnalyticsView::onCleanCache() {
    qint64 cacheSize = getCacheSize();
    QString sizeStr;
    if (cacheSize < 1024 * 1024 * 1024) {
        sizeStr = QString::number(cacheSize / (1024.0 * 1024.0), 'f', 1) + " MB";
    } else {
        sizeStr = QString::number(cacheSize / (1024.0 * 1024.0 * 1024.0), 'f', 2) + " GB";
    }
    
    QMessageBox::StandardButton reply = QMessageBox::question(this, "Clean Package Cache",
        QString("Clean the package cache (%1)?\n\n"
                "This will remove old package versions,\n"
                "keeping only the most recent version\n"
                "of each package.").arg(sizeStr),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        bool success = PrivilegedRunner::runCommand(
            "paccache -rk1",
            "Cleaning package cache (keeping 1 version)",
            this);
        
        if (success) {
            QMessageBox::information(this, "Success", 
                "Package cache cleaned successfully!\n\nRefreshing dashboard...");
            refresh();
        }
    }
}

void AnalyticsView::updateRecentlyUpdated() {
    m_recentlyUpdatedTable->setRowCount(0);
    
    // Parse pacman.log for recent upgrades
    QFile logFile("/var/log/pacman.log");
    if (!logFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }
    
    // Read last 50KB of log
    if (logFile.size() > 50000) {
        logFile.seek(logFile.size() - 50000);
    }
    
    QTextStream in(&logFile);
    QDateTime cutoffDate = QDateTime::currentDateTime().addDays(-7);
    
    struct RecentPkg {
        QString name;
        QString version;
        QDateTime date;
    };
    QList<RecentPkg> recentPackages;
    
    QRegularExpression re(R"(\[(\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}[^\]]*)\] \[ALPM\] upgraded (\S+) \([^)]*-> ([^)]+)\))");
    
    while (!in.atEnd()) {
        QString line = in.readLine();
        QRegularExpressionMatch match = re.match(line);
        if (match.hasMatch()) {
            QString dateStr = match.captured(1);
            QDateTime dt = QDateTime::fromString(dateStr.left(19), Qt::ISODate);
            
            if (dt >= cutoffDate) {
                RecentPkg pkg;
                pkg.name = match.captured(2);
                pkg.version = match.captured(3);
                pkg.date = dt;
                recentPackages.append(pkg);
            }
        }
    }
    logFile.close();
    
    // Show most recent first, limit to 20
    std::sort(recentPackages.begin(), recentPackages.end(),
              [](const RecentPkg& a, const RecentPkg& b) { return a.date > b.date; });
    
    int count = qMin(recentPackages.size(), 20);
    m_recentlyUpdatedTable->setRowCount(count);
    
    for (int i = 0; i < count; ++i) {
        const RecentPkg& pkg = recentPackages[i];
        m_recentlyUpdatedTable->setItem(i, 0, new QTableWidgetItem(pkg.name));
        m_recentlyUpdatedTable->setItem(i, 1, new QTableWidgetItem(pkg.version));
        m_recentlyUpdatedTable->setItem(i, 2, new QTableWidgetItem(pkg.date.toString("MMM dd hh:mm")));
    }
}

void AnalyticsView::updateAurVsRepo() {
    // Use pacman -Qm to get foreign (AUR) packages - this is instant!
    QProcess aurProc;
    aurProc.start("pacman", {"-Qmq"});
    aurProc.waitForFinished(5000);
    int aurCount = QString::fromUtf8(aurProc.readAllStandardOutput())
                       .split('\n', Qt::SkipEmptyParts).size();
    
    // Total packages minus AUR = repo packages
    int total = m_packageManager->totalPackageCount();
    int repoCount = total - aurCount;
    
    m_repoCountLabel->setText(QString("%1 packages").arg(repoCount));
    m_aurCountLabel->setText(QString("%1 packages").arg(aurCount));
}

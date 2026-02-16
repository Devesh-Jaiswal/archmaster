#include "UpdateManager.h"
#include "PrivilegedRunner.h"
#include "core/PackageManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QProcess>
#include <QSplitter>
#include <QScrollArea>
#include <QRegularExpression>
#include <QFile>
#include <QTextStream>
#include <QDialog>
#include <QDialogButtonBox>

UpdateManager::UpdateManager(PackageManager* pm, QWidget* parent)
    : QWidget(parent)
    , m_packageManager(pm)
{
    setupUI();
    // Apply initial theme
    applyTheme(true);
}

void UpdateManager::applyTheme(bool isDark) {
    QString bgColor = isDark ? "#1e1e2e" : "#eff1f5";
    QString textColor = isDark ? "#cdd6f4" : "#4c4f69";
    QString borderColor = isDark ? "#45475a" : "#ccd0da";
    QString headerColor = isDark ? "#89b4fa" : "#1e66f5";
    QString subTextColor = isDark ? "#a6adc8" : "#6c6f85";
    QString inputBg = isDark ? "#313244" : "#e6e9ef";
    QString buttonBg = isDark ? "#89b4fa" : "#1e66f5";
    QString buttonText = isDark ? "#1e1e2e" : "#ffffff";
    QString secButtonBg = isDark ? "#f9e2af" : "#df8e1d"; // Yellow/Orange
    
    // Header Title
    QList<QLabel*> labels = this->findChildren<QLabel*>();
    for(auto label : labels) {
        if(label->text().contains("Update Manager")) {
             label->setStyleSheet(QString("font-size: 24px; font-weight: bold; color: %1;").arg(headerColor));
        } else if (label == m_statusLabel) {
             label->setStyleSheet(QString("color: %1; font-size: 14px; font-weight: bold;").arg(subTextColor));
        }
    }
    
    // Buttons
    QString mainBtnStyle = QString(R"(
        QPushButton {
            background-color: %1;
            color: %2;
            border: none;
            border-radius: 8px;
            padding: 10px 20px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: %3;
            color: %2;
        }
    )").arg(buttonBg, buttonText, isDark ? "#b4befe" : "#7287fd");
    
    QString secBtnStyle = QString(R"(
        QPushButton {
            background-color: %1;
            color: %2;
            border: none;
            border-radius: 8px;
            padding: 10px 20px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: %3;
        }
    )").arg(secButtonBg, isDark ? "#1e1e2e" : "#ffffff", isDark ? "#f5c2e7" : "#ea76cb");
    
    // Note: m_historyBtn uses secondary style, others use main or different.
    if(m_refreshBtn) m_refreshBtn->setStyleSheet(mainBtnStyle);
    if(m_historyBtn) m_historyBtn->setStyleSheet(secBtnStyle);
    
    if(m_updateAllBtn) m_updateAllBtn->setStyleSheet(mainBtnStyle);
    
    QString actionBtnStyle = QString(R"(
        QPushButton {
            background-color: %1;
            color: %2;
            border: none;
            border-radius: 6px;
            padding: 8px 16px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: %3;
        }
         QPushButton:disabled {
            background-color: %4;
            color: %5;
        }
    )").arg(
        isDark ? "#a6e3a1" : "#40a02b", // Green
        isDark ? "#1e1e2e" : "#ffffff",
        isDark ? "#94e2d5" : "#179299", // Teal
        isDark ? "#45475a" : "#ccd0da", // Disabled Bg
        isDark ? "#6c7086" : "#9ca0b0"  // Disabled Text
    );
    if(m_updateSelectedBtn) m_updateSelectedBtn->setStyleSheet(actionBtnStyle);
    
    // Link/Text Buttons
    QString textBtnStyle = QString("QPushButton { color: %1; border: none; font-weight: bold; background: transparent; } QPushButton:hover { text-decoration: underline; }").arg(headerColor);
    if(m_selectAllBtn) m_selectAllBtn->setStyleSheet(textBtnStyle);
    if(m_selectNoneBtn) m_selectNoneBtn->setStyleSheet(textBtnStyle);
    
    // Table
    if(m_updatesTable) {
        m_updatesTable->setStyleSheet(QString(R"(
            QTableWidget {
                background-color: %1;
                alternate-background-color: %2;
                color: %3;
                border: 1px solid %4;
                border-radius: 8px;
                gridline-color: %4;
                selection-background-color: %5;
                selection-color: %6;
            }
            QHeaderView::section {
                background-color: %2;
                color: %3;
                padding: 4px;
                border: none;
                border-bottom: 2px solid %4;
                font-weight: bold;
            }
            QTableCornerButton::section {
                background-color: %2;
                border: none;
                border-bottom: 2px solid %4;
            }
        )").arg(
            bgColor, 
            isDark ? "#313244" : "#e6e9ef", // Alt/Header
            textColor, borderColor,
            isDark ? "#45475a" : "#bcc0cc", // Selection Bg
            textColor
        ));
    }
    
    // Changelog
    if(m_changelogText) {
        m_changelogText->setStyleSheet(QString(R"(
            QTextEdit {
                background-color: %1;
                color: %2;
                border: 2px solid %3;
                border-radius: 8px;
                padding: 10px;
            }
        )").arg(inputBg, textColor, borderColor));
    }
    
    // GroupBoxes
    QString groupStyle = QString(R"(
        QGroupBox {
            font-size: 16px;
            font-weight: bold;
            padding-top: 15px; /* Increased padding */
            color: %1;
            border: 2px solid %2;
            border-radius: 8px;
            margin-top: 20px; /* Increased margin */
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            left: 10px;
            padding: 0 5px;
            background-color: transparent; /* Or matching background if needed */
        }
    )").arg(textColor, borderColor);
    
    QList<QGroupBox*> groups = this->findChildren<QGroupBox*>();
    for (QGroupBox* group : groups) {
        group->setStyleSheet(groupStyle);
    }
}

void UpdateManager::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    
    // Header with status and refresh
    QHBoxLayout* headerLayout = new QHBoxLayout();
    
    QLabel* titleLabel = new QLabel("📦 Update Manager");
    // Style applied by applyTheme
    headerLayout->addWidget(titleLabel);
    
    headerLayout->addStretch();
    
    m_statusLabel = new QLabel("Click 'Check for Updates' to scan for available updates");
    // Style applied by applyTheme
    headerLayout->addWidget(m_statusLabel);
    
    m_refreshBtn = new QPushButton("🔄 Check for Updates");
    // Style applied by applyTheme
    connect(m_refreshBtn, &QPushButton::clicked, this, &UpdateManager::onRefresh);
    headerLayout->addWidget(m_refreshBtn);
    
    m_historyBtn = new QPushButton("📜 History");
    // Style applied by applyTheme
    connect(m_historyBtn, &QPushButton::clicked, this, &UpdateManager::onShowHistory);
    headerLayout->addWidget(m_historyBtn);
    
    mainLayout->addLayout(headerLayout);
    
    // Progress bar
    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 0);
    m_progressBar->setVisible(false);
    m_progressBar->setMaximumHeight(6);
    mainLayout->addWidget(m_progressBar);
    
    // Main content splitter
    QSplitter* splitter = new QSplitter(Qt::Horizontal);
    
    // Left: Updates table
    QGroupBox* updatesGroup = new QGroupBox("Available Updates");
    // Style applied by applyTheme
    QVBoxLayout* updatesLayout = new QVBoxLayout(updatesGroup);
    
    // Selection buttons
    QHBoxLayout* selectionLayout = new QHBoxLayout();
    m_selectAllBtn = new QPushButton("Select All");
    m_selectNoneBtn = new QPushButton("Select None");
    m_selectAllBtn->setMaximumWidth(125);
    m_selectNoneBtn->setMaximumWidth(125);
    connect(m_selectAllBtn, &QPushButton::clicked, this, &UpdateManager::onSelectAll);
    connect(m_selectNoneBtn, &QPushButton::clicked, this, &UpdateManager::onSelectNone);
    selectionLayout->addWidget(m_selectAllBtn);
    selectionLayout->addWidget(m_selectNoneBtn);
    selectionLayout->addStretch();
    updatesLayout->addLayout(selectionLayout);
    
    m_updatesTable = new QTableWidget();
    m_updatesTable->setColumnCount(5);
    m_updatesTable->setHorizontalHeaderLabels({"", "Package", "Current", "→", "Available"});
    m_updatesTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_updatesTable->setColumnWidth(0, 30);
    m_updatesTable->setColumnWidth(2, 100);
    m_updatesTable->setColumnWidth(3, 30);
    m_updatesTable->setColumnWidth(4, 100);
    m_updatesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_updatesTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_updatesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(m_updatesTable, &QTableWidget::cellClicked, this, &UpdateManager::onUpdateClicked);
    // Style applied by applyTheme
    updatesLayout->addWidget(m_updatesTable);
    
    splitter->addWidget(updatesGroup);
    
    // Right: Changelog/details
    QGroupBox* detailsGroup = new QGroupBox("📋 What's New");
    // Style applied by applyTheme
    QVBoxLayout* detailsLayout = new QVBoxLayout(detailsGroup);
    
    m_changelogText = new QTextEdit();
    m_changelogText->setReadOnly(true);
    m_changelogText->setPlaceholderText("Select a package to view changelog and dependency changes");
    // Style applied by applyTheme
    m_changelogText->setPlaceholderText("Select a package to view changelog and dependency changes");
    // Style applied by applyTheme
    detailsLayout->addWidget(m_changelogText);
    
    splitter->addWidget(detailsGroup);
    splitter->setSizes({500, 400});
    
    mainLayout->addWidget(splitter);
    
    // Action buttons
    QHBoxLayout* actionLayout = new QHBoxLayout();
    actionLayout->addStretch();
    
    m_updateSelectedBtn = new QPushButton("⬆️ Update Selected");
    m_updateSelectedBtn->setMinimumHeight(45);
    m_updateSelectedBtn->setEnabled(false);
    // Style applied by applyTheme
    connect(m_updateSelectedBtn, &QPushButton::clicked, this, &UpdateManager::onUpdateSelected);
    actionLayout->addWidget(m_updateSelectedBtn);
    
    m_updateAllBtn = new QPushButton("⬆️ Update All");
    m_updateAllBtn->setMinimumHeight(45);
    m_updateAllBtn->setEnabled(false);
    // Style applied by applyTheme
    connect(m_updateAllBtn, &QPushButton::clicked, this, &UpdateManager::onUpdateAll);
    actionLayout->addWidget(m_updateAllBtn);
    
    mainLayout->addLayout(actionLayout);
}

void UpdateManager::checkForUpdates() {
    onRefresh();
}

void UpdateManager::onRefresh() {
    m_statusLabel->setText("Checking for updates...");
    m_progressBar->setVisible(true);
    m_refreshBtn->setEnabled(false);
    m_updates.clear();
    m_updatesTable->setRowCount(0);
    
    // Run checkupdates in background for official repos
    QProcess* proc = new QProcess(this);
    connect(proc, &QProcess::finished, this, [this, proc](int exitCode) {
        QString output = QString::fromUtf8(proc->readAllStandardOutput());
        proc->deleteLater();
        
        // Parse checkupdates output
        parseUpdatesOutput(output, false);
        
        // Now check AUR updates — try yay first, then paru
        m_statusLabel->setText("Checking AUR updates...");
        checkAURUpdates();
    });
    
    proc->start("checkupdates");
}

void UpdateManager::parseUpdatesOutput(const QString& output, bool isAUR) {
    if (output.trimmed().isEmpty()) return;
    
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    
    for (const QString& line : lines) {
        // Format: package-name current-version -> new-version
        QRegularExpression re(R"(^(\S+)\s+(\S+)\s+->\s+(\S+)$)");
        QRegularExpressionMatch match = re.match(line.trimmed());
        
        if (match.hasMatch()) {
            UpdateInfo info;
            info.name = match.captured(1);
            info.currentVersion = match.captured(2);
            info.newVersion = match.captured(3);
            info.selected = true;
            info.isAUR = isAUR;
            
            // Detect major version change (first digit changes)
            QRegularExpression majorRe(R"(^(\d+))");
            QRegularExpressionMatch currMatch = majorRe.match(info.currentVersion);
            QRegularExpressionMatch newMatch = majorRe.match(info.newVersion);
            info.isMajorUpdate = currMatch.hasMatch() && newMatch.hasMatch() &&
                                 currMatch.captured(1) != newMatch.captured(1);
            
            // Flag potential security updates
            info.isSecurityUpdate = !isAUR && (info.name.contains("linux") || 
                                    info.name.contains("openssl") ||
                                    info.name.contains("gnutls") ||
                                    info.name.contains("nss") ||
                                    info.name.contains("ca-certificates"));
            
            m_updates.append(info);
        }
    }
}

void UpdateManager::checkAURUpdates() {
    // Try yay first
    QProcess* aurProc = new QProcess(this);
    connect(aurProc, &QProcess::finished, this, [this, aurProc](int exitCode) {
        QString aurOutput = QString::fromUtf8(aurProc->readAllStandardOutput());
        aurProc->deleteLater();
        
        if (exitCode != 0 && aurOutput.trimmed().isEmpty()) {
            // yay not available, try paru
            QProcess* paruProc = new QProcess(this);
            connect(paruProc, &QProcess::finished, this, [this, paruProc](int exitCode) {
                QString paruOutput = QString::fromUtf8(paruProc->readAllStandardOutput());
                paruProc->deleteLater();
                
                parseUpdatesOutput(paruOutput, true);
                finalizeUpdatesList();
            });
            paruProc->start("paru", {"-Qua"});
            if (!paruProc->waitForStarted(2000)) {
                paruProc->deleteLater();
                finalizeUpdatesList();
            }
        } else {
            parseUpdatesOutput(aurOutput, true);
            finalizeUpdatesList();
        }
    });
    aurProc->start("yay", {"-Qua"});
    if (!aurProc->waitForStarted(2000)) {
        aurProc->deleteLater();
        // Neither yay nor paru — just finalize with repo updates only
        finalizeUpdatesList();
    }
}

void UpdateManager::finalizeUpdatesList() {
    m_progressBar->setVisible(false);
    m_refreshBtn->setEnabled(true);
    
    if (m_updates.isEmpty()) {
        m_statusLabel->setText("✅ System is up to date!");
        m_updateAllBtn->setEnabled(false);
        m_updateSelectedBtn->setEnabled(false);
        return;
    }
    
    // Count repo vs AUR
    int repoCount = 0, aurCount = 0;
    for (const UpdateInfo& u : m_updates) {
        if (u.isAUR) aurCount++; else repoCount++;
    }
    
    QString statusText = QString("📦 %1 update(s) available").arg(m_updates.size());
    if (aurCount > 0) {
        statusText += QString(" (%1 repo, %2 AUR 🌐)").arg(repoCount).arg(aurCount);
    }
    m_statusLabel->setText(statusText);
    m_updateAllBtn->setEnabled(true);
    m_updateSelectedBtn->setEnabled(true);
    
    // Populate table
    m_updatesTable->setRowCount(m_updates.size());
    for (int i = 0; i < m_updates.size(); ++i) {
        const UpdateInfo& info = m_updates[i];
        
        // Checkbox
        QTableWidgetItem* checkItem = new QTableWidgetItem();
        checkItem->setCheckState(Qt::Checked);
        m_updatesTable->setItem(i, 0, checkItem);
        
        // Package name with flags
        QString nameText = info.name;
        if (info.isAUR) nameText += " 🌐";
        if (info.isSecurityUpdate) nameText += " 🔒";
        if (info.isMajorUpdate) nameText += " ⚠️";
        QTableWidgetItem* nameItem = new QTableWidgetItem(nameText);
        if (info.isSecurityUpdate) {
            nameItem->setForeground(QColor("#f38ba8"));
        } else if (info.isMajorUpdate) {
            nameItem->setForeground(QColor("#f9e2af"));
        } else if (info.isAUR) {
            nameItem->setForeground(QColor("#cba6f7"));
        }
        m_updatesTable->setItem(i, 1, nameItem);
        
        // Versions
        m_updatesTable->setItem(i, 2, new QTableWidgetItem(info.currentVersion));
        m_updatesTable->setItem(i, 3, new QTableWidgetItem("→"));
        
        QTableWidgetItem* newItem = new QTableWidgetItem(info.newVersion);
        newItem->setForeground(QColor("#a6e3a1"));
        m_updatesTable->setItem(i, 4, newItem);
    }
}

void UpdateManager::onUpdateClicked(int row, int column) {
    if (column == 0 && row >= 0 && row < m_updates.size()) {
        // Toggle checkbox
        QTableWidgetItem* item = m_updatesTable->item(row, 0);
        m_updates[row].selected = (item->checkState() == Qt::Checked);
    }
    
    if (row >= 0 && row < m_updates.size()) {
        showUpdateDetails(row);
    }
}

void UpdateManager::showUpdateDetails(int row) {
    const UpdateInfo& info = m_updates[row];
    
    QString html = QString(
        "<h2>%1</h2>"
        "<p><b>Version change:</b> %2 → <span style='color: #a6e3a1;'>%3</span></p>"
    ).arg(info.name, info.currentVersion, info.newVersion);
    
    if (info.isSecurityUpdate) {
        html += "<p style='color: #f38ba8;'>🔒 <b>Security-related package</b> - Recommended to update</p>";
    }
    
    if (info.isMajorUpdate) {
        html += "<p style='color: #f9e2af;'>⚠️ <b>Major version change</b> - Review changelog carefully</p>";
    }
    
    // Try to get changelog
    QProcess proc;
    proc.start("pacman", {"-Qi", info.name});
    proc.waitForFinished(3000);
    QString pkgInfo = QString::fromUtf8(proc.readAllStandardOutput());
    
    // Extract description
    QRegularExpression descRe(R"(Description\s*:\s*(.+))");
    QRegularExpressionMatch descMatch = descRe.match(pkgInfo);
    if (descMatch.hasMatch()) {
        html += QString("<p><b>Description:</b> %1</p>").arg(descMatch.captured(1));
    }
    
    // Extract dependencies
    QRegularExpression depsRe(R"(Depends On\s*:\s*(.+))");
    QRegularExpressionMatch depsMatch = depsRe.match(pkgInfo);
    if (depsMatch.hasMatch()) {
        html += QString("<p><b>Dependencies:</b> %1</p>").arg(depsMatch.captured(1));
    }
    
    html += "<hr><p><i>Changelog information will be available after connecting to package sources.</i></p>";
    
    m_changelogText->setHtml(html);
}

void UpdateManager::onSelectAll() {
    for (int i = 0; i < m_updates.size(); ++i) {
        m_updates[i].selected = true;
        QTableWidgetItem* item = m_updatesTable->item(i, 0);
        if (item) item->setCheckState(Qt::Checked);
    }
}

void UpdateManager::onSelectNone() {
    for (int i = 0; i < m_updates.size(); ++i) {
        m_updates[i].selected = false;
        QTableWidgetItem* item = m_updatesTable->item(i, 0);
        if (item) item->setCheckState(Qt::Unchecked);
    }
}

void UpdateManager::onUpdateSelected() {
    QStringList repoPackages;
    QStringList aurPackages;
    for (const UpdateInfo& info : m_updates) {
        if (info.selected) {
            if (info.isAUR) {
                aurPackages.append(info.name);
            } else {
                repoPackages.append(info.name);
            }
        }
    }
    
    if (repoPackages.isEmpty() && aurPackages.isEmpty()) {
        QMessageBox::information(this, "No Selection", "Please select packages to update.");
        return;
    }
    
    bool success = true;
    
    // Update repo packages with pacman
    if (!repoPackages.isEmpty()) {
        QString command = QString("pacman -S %1").arg(repoPackages.join(" "));
        success = PrivilegedRunner::runCommand(
            command,
            QString("Updating %1 repo package(s)").arg(repoPackages.size()),
            this);
    }
    
    // Update AUR packages with yay/paru (no sudo needed)
    if (success && !aurPackages.isEmpty()) {
        QString aurHelper = QProcess().execute("which", {"yay"}) == 0 ? "yay" : "paru";
        QString command = QString("%1 -S %2").arg(aurHelper, aurPackages.join(" "));
        success = PrivilegedRunner::runCommand(
            command,
            QString("Updating %1 AUR package(s)").arg(aurPackages.size()),
            this);
    }
    
    if (success) {
        QMessageBox::information(this, "Success", "Packages updated successfully!");
        onRefresh(); // Refresh the list
    }
}

void UpdateManager::onUpdateAll() {
    int count = m_updates.size();
    
    QMessageBox::StandardButton reply = QMessageBox::question(this, "Update All",
        QString("Update all %1 package(s)?\n\n"
                "This will download and install all available updates.").arg(count),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply != QMessageBox::Yes) return;
    
    // Check if any AUR packages exist — use yay/paru for full system update
    bool hasAUR = false;
    for (const UpdateInfo& u : m_updates) {
        if (u.isAUR) { hasAUR = true; break; }
    }
    
    QString command;
    if (hasAUR) {
        // yay/paru handles both repo + AUR updates
        QString aurHelper = QProcess().execute("which", {"yay"}) == 0 ? "yay" : "paru";
        command = aurHelper + " -Syu";
    } else {
        command = "pacman -Syu";
    }
    
    bool success = PrivilegedRunner::runCommand(
        command,
        QString("Updating all %1 package(s)").arg(count),
        this);
    
    if (success) {
        QMessageBox::information(this, "Success", "System updated successfully!");
        onRefresh();
    }
}

void UpdateManager::parseChangelog(const QString& packageName) {
    // Placeholder for changelog parsing
    // Could use: pacman -Qc, arch wiki, or NEWS files
    Q_UNUSED(packageName);
}

void UpdateManager::onShowHistory() {
    // Parse /var/log/pacman.log for recent updates
    QFile logFile("/var/log/pacman.log");
    if (!logFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Cannot open /var/log/pacman.log");
        return;
    }
    
    // Read last portion of the file (last ~100KB)
    if (logFile.size() > 100000) {
        logFile.seek(logFile.size() - 100000);
    }
    
    QTextStream in(&logFile);
    QStringList lines;
    while (!in.atEnd()) {
        lines.append(in.readLine());
    }
    logFile.close();
    
    // Parse upgrade/install/remove actions
    struct HistoryEntry {
        QString date;
        QString action;
        QString package;
        QString oldVersion;
        QString newVersion;
    };
    
    QList<HistoryEntry> history;
    QRegularExpression upgradeRe(R"(\[(\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}[^\]]*)\] \[ALPM\] (upgraded|installed|removed) (\S+) \(([^)]*)\))");
    
    for (const QString& line : lines) {
        QRegularExpressionMatch match = upgradeRe.match(line);
        if (match.hasMatch()) {
            HistoryEntry entry;
            entry.date = match.captured(1).left(16).replace('T', ' '); // YYYY-MM-DD HH:MM
            entry.action = match.captured(2);
            entry.package = match.captured(3);
            
            QString versionInfo = match.captured(4);
            if (entry.action == "upgraded") {
                QStringList parts = versionInfo.split(" -> ");
                entry.oldVersion = parts.value(0);
                entry.newVersion = parts.value(1);
            } else if (entry.action == "installed") {
                entry.newVersion = versionInfo;
            } else if (entry.action == "removed") {
                entry.oldVersion = versionInfo;
            }
            
            history.append(entry);
        }
    }
    
    // Show in dialog
    QDialog dialog(this);
    dialog.setWindowTitle("Update History");
    dialog.setMinimumSize(700, 500);
    
    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    
    QLabel* titleLabel = new QLabel("📜 Recent Package Operations");
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #89b4fa;");
    layout->addWidget(titleLabel);
    
    QTableWidget* table = new QTableWidget();
    table->setColumnCount(5);
    table->setHorizontalHeaderLabels({"Date", "Action", "Package", "Old Version", "New Version"});
    table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setStyleSheet(R"(
        QTableWidget {
            background-color: #1e1e2e;
            border: 1px solid #45475a;
            border-radius: 8px;
        }
        QTableWidget::item {
            padding: 5px;
        }
        QHeaderView::section {
            background-color: #313244;
            color: #cdd6f4;
            padding: 8px;
            border: none;
        }
    )");
    
    // Show most recent first (reverse order)
    int maxEntries = qMin(history.size(), 200);
    table->setRowCount(maxEntries);
    
    for (int i = 0; i < maxEntries; ++i) {
        const HistoryEntry& entry = history[history.size() - 1 - i];
        
        table->setItem(i, 0, new QTableWidgetItem(entry.date));
        
        QTableWidgetItem* actionItem = new QTableWidgetItem(entry.action);
        if (entry.action == "upgraded") {
            actionItem->setForeground(QColor("#a6e3a1"));
        } else if (entry.action == "installed") {
            actionItem->setForeground(QColor("#89b4fa"));
        } else if (entry.action == "removed") {
            actionItem->setForeground(QColor("#f38ba8"));
        }
        table->setItem(i, 1, actionItem);
        
        table->setItem(i, 2, new QTableWidgetItem(entry.package));
        table->setItem(i, 3, new QTableWidgetItem(entry.oldVersion));
        table->setItem(i, 4, new QTableWidgetItem(entry.newVersion));
    }
    
    layout->addWidget(table);
    
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);
    
    dialog.exec();
}

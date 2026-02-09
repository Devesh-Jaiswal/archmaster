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
}

void UpdateManager::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    
    // Header with status and refresh
    QHBoxLayout* headerLayout = new QHBoxLayout();
    
    QLabel* titleLabel = new QLabel("📦 Update Manager");
    titleLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #89b4fa;");
    headerLayout->addWidget(titleLabel);
    
    headerLayout->addStretch();
    
    m_statusLabel = new QLabel("Click 'Check for Updates' to scan for available updates");
    m_statusLabel->setStyleSheet("color: #a6adc8;");
    headerLayout->addWidget(m_statusLabel);
    
    m_refreshBtn = new QPushButton("🔄 Check for Updates");
    m_refreshBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #89b4fa;
            color: #1e1e2e;
            border: none;
            border-radius: 8px;
            padding: 10px 20px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #b4befe;
        }
    )");
    connect(m_refreshBtn, &QPushButton::clicked, this, &UpdateManager::onRefresh);
    headerLayout->addWidget(m_refreshBtn);
    
    m_historyBtn = new QPushButton("📜 History");
    m_historyBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #f9e2af;
            color: #1e1e2e;
            border: none;
            border-radius: 8px;
            padding: 10px 20px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #f5c2e7;
        }
    )");
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
    m_updatesTable->setStyleSheet(R"(
        QTableWidget {
            background-color: #1e1e2e;
            border: 1px solid #45475a;
            border-radius: 8px;
        }
        QTableWidget::item {
            padding: 8px;
        }
        QTableWidget::item:selected {
            background-color: #45475a;
        }
        QHeaderView::section {
            background-color: #313244;
            color: #cdd6f4;
            padding: 8px;
            border: none;
        }
    )");
    updatesLayout->addWidget(m_updatesTable);
    
    splitter->addWidget(updatesGroup);
    
    // Right: Changelog/details
    QGroupBox* detailsGroup = new QGroupBox("📋 What's New");
    QVBoxLayout* detailsLayout = new QVBoxLayout(detailsGroup);
    
    m_changelogText = new QTextEdit();
    m_changelogText->setReadOnly(true);
    m_changelogText->setPlaceholderText("Select a package to view changelog and dependency changes");
    m_changelogText->setStyleSheet(R"(
        QTextEdit {
            background-color: #1e1e2e;
            color: #cdd6f4;
            border: 1px solid #45475a;
            border-radius: 8px;
            padding: 10px;
        }
    )");
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
    m_updateSelectedBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #94e2d5;
            color: #1e1e2e;
            border: none;
            border-radius: 8px;
            padding: 10px 25px;
            font-weight: bold;
            font-size: 14px;
        }
        QPushButton:hover {
            background-color: #a6e3a1;
        }
        QPushButton:disabled {
            background-color: #45475a;
            color: #6c7086;
        }
    )");
    connect(m_updateSelectedBtn, &QPushButton::clicked, this, &UpdateManager::onUpdateSelected);
    actionLayout->addWidget(m_updateSelectedBtn);
    
    m_updateAllBtn = new QPushButton("⬆️ Update All");
    m_updateAllBtn->setMinimumHeight(45);
    m_updateAllBtn->setEnabled(false);
    m_updateAllBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #a6e3a1;
            color: #1e1e2e;
            border: none;
            border-radius: 8px;
            padding: 10px 25px;
            font-weight: bold;
            font-size: 14px;
        }
        QPushButton:hover {
            background-color: #94e2d5;
        }
        QPushButton:disabled {
            background-color: #45475a;
            color: #6c7086;
        }
    )");
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
    
    // Run checkupdates in background
    QProcess* proc = new QProcess(this);
    connect(proc, &QProcess::finished, this, [this, proc](int exitCode) {
        m_progressBar->setVisible(false);
        m_refreshBtn->setEnabled(true);
        
        QString output = QString::fromUtf8(proc->readAllStandardOutput());
        proc->deleteLater();
        
        if (output.trimmed().isEmpty()) {
            m_statusLabel->setText("✅ System is up to date!");
            m_updateAllBtn->setEnabled(false);
            m_updateSelectedBtn->setEnabled(false);
            return;
        }
        
        // Parse checkupdates output
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
                
                // Detect major version change (first digit changes)
                QRegularExpression majorRe(R"(^(\d+))");
                QRegularExpressionMatch currMatch = majorRe.match(info.currentVersion);
                QRegularExpressionMatch newMatch = majorRe.match(info.newVersion);
                info.isMajorUpdate = currMatch.hasMatch() && newMatch.hasMatch() &&
                                     currMatch.captured(1) != newMatch.captured(1);
                
                // Flag potential security updates
                info.isSecurityUpdate = info.name.contains("linux") || 
                                        info.name.contains("openssl") ||
                                        info.name.contains("gnutls") ||
                                        info.name.contains("nss") ||
                                        info.name.contains("ca-certificates");
                
                m_updates.append(info);
            }
        }
        
        m_statusLabel->setText(QString("📦 %1 update(s) available").arg(m_updates.size()));
        m_updateAllBtn->setEnabled(!m_updates.isEmpty());
        m_updateSelectedBtn->setEnabled(!m_updates.isEmpty());
        
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
            if (info.isSecurityUpdate) nameText += " 🔒";
            if (info.isMajorUpdate) nameText += " ⚠️";
            QTableWidgetItem* nameItem = new QTableWidgetItem(nameText);
            if (info.isSecurityUpdate) {
                nameItem->setForeground(QColor("#f38ba8"));
            } else if (info.isMajorUpdate) {
                nameItem->setForeground(QColor("#f9e2af"));
            }
            m_updatesTable->setItem(i, 1, nameItem);
            
            // Versions
            m_updatesTable->setItem(i, 2, new QTableWidgetItem(info.currentVersion));
            m_updatesTable->setItem(i, 3, new QTableWidgetItem("→"));
            
            QTableWidgetItem* newItem = new QTableWidgetItem(info.newVersion);
            newItem->setForeground(QColor("#a6e3a1"));
            m_updatesTable->setItem(i, 4, newItem);
        }
    });
    
    proc->start("checkupdates");
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
    QStringList selectedPackages;
    for (const UpdateInfo& info : m_updates) {
        if (info.selected) {
            selectedPackages.append(info.name);
        }
    }
    
    if (selectedPackages.isEmpty()) {
        QMessageBox::information(this, "No Selection", "Please select packages to update.");
        return;
    }
    
    QString command = QString("pacman -Syu %1 --noconfirm").arg(selectedPackages.join(" "));
    
    bool success = PrivilegedRunner::runCommand(
        command,
        QString("Updating %1 selected package(s)").arg(selectedPackages.size()),
        this);
    
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
    
    bool success = PrivilegedRunner::runCommand(
        "pacman -Syu --noconfirm",
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

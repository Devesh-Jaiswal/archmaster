#include "ControlPanel.h"
#include "core/PackageManager.h"
#include "core/Database.h"
#include "models/Package.h"
#include "PrivilegedRunner.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QScrollArea>
#include <QDir>
#include <QDirIterator>
#include <QFile>

ControlPanel::ControlPanel(PackageManager* pm, Database* db, QWidget* parent)
    : QWidget(parent)
    , m_packageManager(pm)
    , m_database(db)
{
    setupUI();
    onRefreshOrphans();
}

void ControlPanel::setupUI() {
    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    
    QWidget* content = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout(content);
    mainLayout->setSpacing(20);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    
    // System Operations Group
    QGroupBox* sysGroup = new QGroupBox("🖥️ System Operations");
    QVBoxLayout* sysLayout = new QVBoxLayout(sysGroup);
    
    QLabel* warningLabel = new QLabel(
        "⚠️ These operations require root privileges. "
        "A terminal will open to execute the commands."
    );
    warningLabel->setWordWrap(true);
    warningLabel->setStyleSheet("color: #f9e2af; padding: 10px; background-color: #45475a; border-radius: 8px;");
    sysLayout->addWidget(warningLabel);
    
    QHBoxLayout* sysButtonsLayout = new QHBoxLayout();
    
    m_cleanCacheBtn = new QPushButton("🧹 Clean Cache");
    m_cleanCacheBtn->setToolTip("Run: sudo pacman -Sc");
    m_cleanCacheBtn->setStyleSheet("padding: 15px 30px; font-size: 14px;");
    connect(m_cleanCacheBtn, &QPushButton::clicked, this, &ControlPanel::onCleanCache);
    
    m_syncDbBtn = new QPushButton("📥 Sync Database");
    m_syncDbBtn->setToolTip("Run: sudo pacman -Sy");
    m_syncDbBtn->setStyleSheet("padding: 15px 30px; font-size: 14px;");
    connect(m_syncDbBtn, &QPushButton::clicked, this, &ControlPanel::onSyncDatabase);
    
    m_refreshKeysBtn = new QPushButton("🔑 Refresh Keyrings");
    m_refreshKeysBtn->setToolTip("Refresh pacman keyring");
    m_refreshKeysBtn->setStyleSheet("padding: 15px 30px; font-size: 14px;");
    connect(m_refreshKeysBtn, &QPushButton::clicked, this, &ControlPanel::onRefreshKeyrings);
    
    m_fixDbLockBtn = new QPushButton("🔓 Fix DB Lock");
    m_fixDbLockBtn->setToolTip("Remove /var/lib/pacman/db.lck");
    m_fixDbLockBtn->setStyleSheet("padding: 15px 30px; font-size: 14px; background-color: #f9e2af; color: #1e1e2e;");
    connect(m_fixDbLockBtn, &QPushButton::clicked, this, &ControlPanel::onFixDbLock);
    
    sysButtonsLayout->addWidget(m_cleanCacheBtn);
    sysButtonsLayout->addWidget(m_syncDbBtn);
    sysButtonsLayout->addWidget(m_refreshKeysBtn);
    sysButtonsLayout->addWidget(m_fixDbLockBtn);
    sysButtonsLayout->addStretch();
    
    sysLayout->addLayout(sysButtonsLayout);
    
    // Cache size info
    QHBoxLayout* cacheInfoLayout = new QHBoxLayout();
    cacheInfoLayout->addWidget(new QLabel("📦 Package Cache: "));
    m_cacheSizeLabel = new QLabel("Calculating...");
    m_cacheSizeLabel->setStyleSheet("font-weight: bold;");
    cacheInfoLayout->addWidget(m_cacheSizeLabel);
    cacheInfoLayout->addStretch();
    sysLayout->addLayout(cacheInfoLayout);
    
    // Calculate cache size
    QDir cacheDir("/var/cache/pacman/pkg");
    qint64 cacheSize = 0;
    QDirIterator it(cacheDir.absolutePath(), QDir::Files);
    while (it.hasNext()) {
        it.next();
        cacheSize += it.fileInfo().size();
    }
    m_cacheSizeLabel->setText(QString("%1 GB").arg(cacheSize / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2));
    
    mainLayout->addWidget(sysGroup);
    
    // Orphan Packages Group
    QGroupBox* orphansGroup = new QGroupBox("⚠️ Orphan Package Cleanup");
    QVBoxLayout* orphansLayout = new QVBoxLayout(orphansGroup);
    
    QLabel* orphanInfo = new QLabel(
        "Orphan packages are dependencies that are no longer required by any installed package. "
        "Removing them can free up disk space, but verify you don't need them first."
    );
    orphanInfo->setWordWrap(true);
    orphanInfo->setStyleSheet("color: #a6adc8; margin-bottom: 10px;");
    orphansLayout->addWidget(orphanInfo);
    
    QHBoxLayout* orphansTopLayout = new QHBoxLayout();
    m_refreshOrphansBtn = new QPushButton("🔄 Refresh List");
    connect(m_refreshOrphansBtn, &QPushButton::clicked, this, &ControlPanel::onRefreshOrphans);
    
    m_orphanSizeLabel = new QLabel();
    m_orphanSizeLabel->setStyleSheet("font-weight: bold; color: #f38ba8;");
    
    orphansTopLayout->addWidget(m_refreshOrphansBtn);
    orphansTopLayout->addWidget(m_orphanSizeLabel);
    orphansTopLayout->addStretch();
    orphansLayout->addLayout(orphansTopLayout);
    
    m_orphansList = new QListWidget();
    m_orphansList->setSelectionMode(QAbstractItemView::MultiSelection);
    m_orphansList->setMinimumHeight(200);
    orphansLayout->addWidget(m_orphansList);
    
    QHBoxLayout* orphanButtonsLayout = new QHBoxLayout();
    
    m_removeSelectedBtn = new QPushButton("🗑️ Remove Selected");
    m_removeSelectedBtn->setStyleSheet("background-color: #f38ba8; color: #1e1e2e; padding: 10px 20px;");
    connect(m_removeSelectedBtn, &QPushButton::clicked, this, &ControlPanel::onRemoveSelected);
    
    m_removeOrphansBtn = new QPushButton("🗑️ Remove All Orphans");
    m_removeOrphansBtn->setStyleSheet("background-color: #f38ba8; color: #1e1e2e; padding: 10px 20px;");
    connect(m_removeOrphansBtn, &QPushButton::clicked, this, &ControlPanel::onRemoveOrphans);
    
    orphanButtonsLayout->addWidget(m_removeSelectedBtn);
    orphanButtonsLayout->addWidget(m_removeOrphansBtn);
    orphanButtonsLayout->addStretch();
    orphansLayout->addLayout(orphanButtonsLayout);
    
    mainLayout->addWidget(orphansGroup);
    
    // Command Output Group
    QGroupBox* outputGroup = new QGroupBox("📋 Command Output");
    QVBoxLayout* outputLayout = new QVBoxLayout(outputGroup);
    
    m_statusLabel = new QLabel("Ready");
    m_statusLabel->setStyleSheet("font-weight: bold;");
    outputLayout->addWidget(m_statusLabel);
    
    m_progressBar = new QProgressBar();
    m_progressBar->setVisible(false);
    outputLayout->addWidget(m_progressBar);
    
    m_outputText = new QTextEdit();
    m_outputText->setReadOnly(true);
    m_outputText->setMinimumHeight(200);
    m_outputText->setStyleSheet("font-family: monospace; font-size: 12px;");
    outputLayout->addWidget(m_outputText);
    
    mainLayout->addWidget(outputGroup);
    mainLayout->addStretch();
    
    scrollArea->setWidget(content);
    
    QVBoxLayout* wrapperLayout = new QVBoxLayout(this);
    wrapperLayout->setContentsMargins(0, 0, 0, 0);
    wrapperLayout->addWidget(scrollArea);
}


void ControlPanel::onCleanCache() {
    QMessageBox::StandardButton reply = QMessageBox::question(this, 
        "Clean Package Cache",
        "This will run 'sudo pacman -Sc' to remove old package versions from cache.\n\nProceed?",
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        runCommand("Clean Cache", "sudo pacman -Sc");
    }
}

void ControlPanel::onRemoveOrphans() {
    QList<Package> orphans = m_packageManager->getOrphanPackages();
    
    if (orphans.isEmpty()) {
        QMessageBox::information(this, "No Orphans", "No orphan packages found.");
        return;
    }
    
    QStringList names;
    for (const Package& pkg : orphans) {
        // Skip if marked as keep
        if (m_database->isPackageMarkedKeep(pkg.name)) continue;
        names << pkg.name;
    }
    
    if (names.isEmpty()) {
        QMessageBox::information(this, "All Kept", 
            "All orphan packages are marked as 'Keep'. Nothing to remove.");
        return;
    }
    
    QMessageBox::StandardButton reply = QMessageBox::warning(this, 
        "Remove All Orphans",
        QString("This will remove %1 orphan packages:\n\n%2\n\nProceed?")
            .arg(names.size())
            .arg(names.join(", ")),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        runCommand("Remove Orphans", "sudo pacman -Rns " + names.join(" "));
    }
}

void ControlPanel::onRemoveSelected() {
    QList<QListWidgetItem*> selected = m_orphansList->selectedItems();
    
    if (selected.isEmpty()) {
        QMessageBox::information(this, "No Selection", "Please select packages to remove.");
        return;
    }
    
    QStringList names;
    for (QListWidgetItem* item : selected) {
        QString text = item->text();
        // Extract package name (before the size info)
        QString name = text.split(" ").first();
        names << name;
    }
    
    QMessageBox::StandardButton reply = QMessageBox::warning(this, 
        "Remove Selected Packages",
        QString("Remove these %1 packages?\n\n%2")
            .arg(names.size())
            .arg(names.join(", ")),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        runCommand("Remove Packages", "sudo pacman -Rns " + names.join(" "));
    }
}

void ControlPanel::onRefreshOrphans() {
    m_orphansList->clear();
    
    QList<Package> orphans = m_packageManager->getOrphanPackages();
    
    qint64 totalSize = 0;
    for (const Package& pkg : orphans) {
        QString itemText = QString("%1  (%2)")
            .arg(pkg.name)
            .arg(pkg.formattedSize());
        
        QListWidgetItem* item = new QListWidgetItem(itemText);
        
        // Mark keep packages differently
        if (m_database->isPackageMarkedKeep(pkg.name)) {
            item->setForeground(QColor("#a6e3a1"));
            item->setToolTip("📌 Marked as Keep");
        }
        
        m_orphansList->addItem(item);
        totalSize += pkg.installedSize;
    }
    
    QString sizeStr;
    if (totalSize < 1024 * 1024) {
        sizeStr = QString::number(totalSize / 1024.0, 'f', 1) + " KB";
    } else if (totalSize < 1024 * 1024 * 1024) {
        sizeStr = QString::number(totalSize / (1024.0 * 1024.0), 'f', 1) + " MB";
    } else {
        sizeStr = QString::number(totalSize / (1024.0 * 1024.0 * 1024.0), 'f', 2) + " GB";
    }
    
    m_orphanSizeLabel->setText(QString("%1 orphans (%2 total)").arg(orphans.size()).arg(sizeStr));
}

void ControlPanel::runCommand(const QString& title, const QString& command) {
    m_statusLabel->setText("Running: " + title);
    m_outputText->clear();
    m_outputText->append("$ " + command + "\n");
    m_outputText->append("Executing with elevated privileges...\n");
    
    // Use PrivilegedRunner for reliable inline execution with output dialog
    bool success = PrivilegedRunner::runCommand(command, title, this);
    
    if (success) {
        m_outputText->append("\n✅ Command completed successfully!");
        m_statusLabel->setText("✅ " + title + " completed");
        onRefreshOrphans(); // Refresh orphans list
    } else {
        m_outputText->append("\n❌ Command failed or was cancelled.");
        m_statusLabel->setText("❌ " + title + " failed");
    }
}

void ControlPanel::onSyncDatabase() {
    QMessageBox::StandardButton reply = QMessageBox::question(this, 
        "Sync Database",
        "This will sync the package database with 'sudo pacman -Sy'.\n\n"
        "Note: It's generally recommended to use 'pacman -Syu' instead.\n\nProceed?",
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        runCommand("Sync Database", "sudo pacman -Sy");
    }
}

void ControlPanel::onRefreshKeyrings() {
    QMessageBox::StandardButton reply = QMessageBox::question(this, 
        "Refresh Keyrings",
        "This will refresh the pacman keyring. This may take a few minutes.\n\n"
        "Commands to run:\n"
        "  sudo pacman-key --refresh-keys\n"
        "  sudo pacman-key --populate archlinux\n\nProceed?",
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        runCommand("Refresh Keyrings", "sudo pacman-key --refresh-keys && sudo pacman-key --populate archlinux");
    }
}

void ControlPanel::onFixDbLock() {
    // Check if lock file exists
    QFile lockFile("/var/lib/pacman/db.lck");
    
    if (!lockFile.exists()) {
        QMessageBox::information(this, "No Lock File", 
            "The database lock file does not exist.\n"
            "No action needed!");
        return;
    }
    
    QMessageBox::StandardButton reply = QMessageBox::warning(this, 
        "Fix Database Lock",
        "⚠️ Warning: Only use this if pacman is NOT currently running!\n\n"
        "This will remove /var/lib/pacman/db.lck\n\n"
        "If pacman is running, this could corrupt your database.\n\nProceed?",
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        runCommand("Fix DB Lock", "sudo rm -f /var/lib/pacman/db.lck");
    }
}

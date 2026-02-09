#include "PackageView.h"
#include "core/PackageManager.h"
#include "core/Database.h"
#include "core/AURClient.h"
#include "core/PacmanConfig.h"
#include "models/PackageListModel.h"
#include "PrivilegedRunner.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QListWidget>
#include <QScrollArea>
#include <QApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QInputDialog>
#include <QMessageBox>
#include <QProcess>
#include <QRegularExpression>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QEventLoop>
#include <QProgressDialog>

PackageView::PackageView(PackageManager* pm, Database* db, AURClient* aur, QWidget* parent)
    : QWidget(parent)
    , m_packageManager(pm)
    , m_database(db)
    , m_aurClient(aur)
{
    m_model = new PackageListModel(this);
    m_proxyModel = new PackageFilterProxyModel(this);
    m_proxyModel->setSourceModel(m_model);
    
    setupUI();
}

void PackageView::setupUI() {
    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    
    // Left panel - Package list
    QWidget* leftPanel = new QWidget();
    QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setSpacing(10);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    
    // Search bar
    QHBoxLayout* searchLayout = new QHBoxLayout();
    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText("🔍 Search packages...");
    m_searchEdit->setClearButtonEnabled(true);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &PackageView::onSearchTextChanged);
    
    m_filterCombo = new QComboBox();
    m_filterCombo->addItem("All Packages", PackageFilterProxyModel::FilterAll);
    m_filterCombo->addItem("Explicitly Installed", PackageFilterProxyModel::FilterExplicit);
    m_filterCombo->addItem("Dependencies", PackageFilterProxyModel::FilterDependency);
    m_filterCombo->addItem("⚠️ Orphans", PackageFilterProxyModel::FilterOrphan);
    m_filterCombo->addItem("📌 Marked Keep", PackageFilterProxyModel::FilterKeep);
    m_filterCombo->addItem("🔍 To Review", PackageFilterProxyModel::FilterReview);
    m_filterCombo->addItem("📦 Large (>100MB)", PackageFilterProxyModel::FilterLarge);
    connect(m_filterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &PackageView::onFilterChanged);
    
    searchLayout->addWidget(m_searchEdit, 1);
    searchLayout->addWidget(m_filterCombo);
    leftLayout->addLayout(searchLayout);
    
    // Table
    m_tableView = new QTableView();
    m_tableView->setModel(m_proxyModel);
    m_tableView->setAlternatingRowColors(true);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->setSortingEnabled(true);
    m_tableView->horizontalHeader()->setStretchLastSection(true);
    m_tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_tableView->verticalHeader()->setVisible(false);
    m_tableView->setShowGrid(false);
    
    // Set column widths
    m_tableView->setColumnWidth(PackageListModel::NameColumn, 200);
    m_tableView->setColumnWidth(PackageListModel::VersionColumn, 150);
    m_tableView->setColumnWidth(PackageListModel::SizeColumn, 100);
    m_tableView->setColumnWidth(PackageListModel::InstallDateColumn, 100);
    m_tableView->setColumnWidth(PackageListModel::ReasonColumn, 80);
    
    connect(m_tableView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &PackageView::onPackageClicked);
    
    leftLayout->addWidget(m_tableView);
    
    // Right panel - Details (scrollable)
    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setMinimumWidth(400);
    scrollArea->setMaximumWidth(500);
    
    QWidget* rightPanel = new QWidget();
    QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setSpacing(15);
    
    // Package info
    QGroupBox* infoGroup = new QGroupBox("Package Information");
    QVBoxLayout* infoLayout = new QVBoxLayout(infoGroup);
    
    m_packageNameLabel = new QLabel();
    m_packageNameLabel->setStyleSheet("font-size: 18px; font-weight: bold;");
    m_packageVersionLabel = new QLabel();
    m_packageSizeLabel = new QLabel();
    m_packageDateLabel = new QLabel();
    m_packageReasonLabel = new QLabel();
    m_packageDescLabel = new QLabel();
    m_packageDescLabel->setWordWrap(true);
    
    infoLayout->addWidget(m_packageNameLabel);
    infoLayout->addWidget(m_packageVersionLabel);
    infoLayout->addWidget(m_packageSizeLabel);
    infoLayout->addWidget(m_packageDateLabel);
    infoLayout->addWidget(m_packageReasonLabel);
    infoLayout->addWidget(new QLabel("<hr>"));
    infoLayout->addWidget(m_packageDescLabel);
    
    rightLayout->addWidget(infoGroup);
    
    // Dependencies
    QGroupBox* depsGroup = new QGroupBox("Dependencies");
    QVBoxLayout* depsLayout = new QVBoxLayout(depsGroup);
    
    m_dependsLabel = new QLabel("Depends: -");
    m_dependsLabel->setWordWrap(true);
    m_requiredByLabel = new QLabel("Required by: -");
    m_requiredByLabel->setWordWrap(true);
    
    depsLayout->addWidget(m_dependsLabel);
    depsLayout->addWidget(m_requiredByLabel);
    
    rightLayout->addWidget(depsGroup);
    
    // Notes
    QGroupBox* notesGroup = new QGroupBox("📝 Your Notes");
    QVBoxLayout* notesLayout = new QVBoxLayout(notesGroup);
    
    m_notesEdit = new QTextEdit();
    m_notesEdit->setPlaceholderText("Add your notes about this package here...");
    m_notesEdit->setMaximumHeight(100);
    
    m_saveNotesBtn = new QPushButton("💾 Save Notes");
    connect(m_saveNotesBtn, &QPushButton::clicked, this, &PackageView::onSaveNotes);
    
    notesLayout->addWidget(m_notesEdit);
    notesLayout->addWidget(m_saveNotesBtn);
    
    rightLayout->addWidget(notesGroup);
    
    // Tags
    QGroupBox* tagsGroup = new QGroupBox("🏷️ Tags");
    QVBoxLayout* tagsLayout = new QVBoxLayout(tagsGroup);
    
    QHBoxLayout* tagInputLayout = new QHBoxLayout();
    m_tagInput = new QLineEdit();
    m_tagInput->setPlaceholderText("Add tag...");
    m_addTagBtn = new QPushButton("+");
    m_addTagBtn->setMaximumWidth(40);
    connect(m_addTagBtn, &QPushButton::clicked, this, &PackageView::onAddTag);
    connect(m_tagInput, &QLineEdit::returnPressed, this, &PackageView::onAddTag);
    tagInputLayout->addWidget(m_tagInput);
    tagInputLayout->addWidget(m_addTagBtn);
    
    m_tagsList = new QListWidget();
    m_tagsList->setMaximumHeight(80);
    
    m_removeTagBtn = new QPushButton("Remove Selected Tag");
    connect(m_removeTagBtn, &QPushButton::clicked, this, &PackageView::onRemoveTag);
    
    tagsLayout->addLayout(tagInputLayout);
    tagsLayout->addWidget(m_tagsList);
    tagsLayout->addWidget(m_removeTagBtn);
    
    rightLayout->addWidget(tagsGroup);
    
    // Actions
    QGroupBox* actionsGroup = new QGroupBox("Actions");
    QVBoxLayout* actionsOuterLayout = new QVBoxLayout(actionsGroup);
    
    QHBoxLayout* actionsLayout1 = new QHBoxLayout();
    m_keepBtn = new QPushButton("📌 Keep");
    m_keepBtn->setCheckable(true);
    connect(m_keepBtn, &QPushButton::clicked, this, &PackageView::onToggleKeep);
    
    m_reviewBtn = new QPushButton("🔍 Review");
    m_reviewBtn->setCheckable(true);
    connect(m_reviewBtn, &QPushButton::clicked, this, &PackageView::onToggleReview);
    
    actionsLayout1->addWidget(m_keepBtn);
    actionsLayout1->addWidget(m_reviewBtn);
    actionsOuterLayout->addLayout(actionsLayout1);
    
    // Package management actions
    QHBoxLayout* actionsLayout2 = new QHBoxLayout();
    
    m_versionBtn = new QPushButton("🔄 Change Version");
    m_versionBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #89b4fa;
            color: #1e1e2e;
            border: none;
            border-radius: 6px;
            padding: 8px 12px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #b4befe;
        }
    )");
    connect(m_versionBtn, &QPushButton::clicked, this, &PackageView::onChangeVersion);
    
    m_pinBtn = new QPushButton("📌 Pin");
    m_pinBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #f9e2af;
            color: #1e1e2e;
            border: none;
            border-radius: 6px;
            padding: 8px 12px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #f5c2e7;
        }
    )");
    connect(m_pinBtn, &QPushButton::clicked, this, &PackageView::onTogglePin);
    
    m_removeBtn = new QPushButton("🗑️ Remove");
    m_removeBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #f38ba8;
            color: #1e1e2e;
            border: none;
            border-radius: 6px;
            padding: 8px 12px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #eba0ac;
        }
    )");
    connect(m_removeBtn, &QPushButton::clicked, this, &PackageView::onRemovePackage);
    
    actionsLayout2->addWidget(m_versionBtn);
    actionsLayout2->addWidget(m_pinBtn);
    actionsLayout2->addWidget(m_removeBtn);
    actionsOuterLayout->addLayout(actionsLayout2);
    
    rightLayout->addWidget(actionsGroup);
    
    rightLayout->addStretch();
    
    scrollArea->setWidget(rightPanel);
    
    // Splitter
    QSplitter* splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(leftPanel);
    splitter->addWidget(scrollArea);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 1);
    
    mainLayout->addWidget(splitter);
}

void PackageView::loadPackages() {
    QList<Package> packages = m_packageManager->getAllPackages();
    
    // Merge user data
    for (Package& pkg : packages) {
        PackageUserData userData = m_database->getPackageUserData(pkg.name);
        pkg.userNotes = userData.notes;
        pkg.userTags = userData.tags;
        pkg.isMarkedKeep = userData.markedKeep;
        pkg.isMarkedReview = userData.markedReview;
    }
    
    m_model->setPackages(packages);
    m_proxyModel->sort(PackageListModel::NameColumn, Qt::AscendingOrder);
}

void PackageView::onSearchTextChanged(const QString& text) {
    m_proxyModel->setSearchText(text);
}

void PackageView::onFilterChanged(int index) {
    PackageFilterProxyModel::FilterType type = 
        static_cast<PackageFilterProxyModel::FilterType>(m_filterCombo->currentData().toInt());
    m_proxyModel->setFilterType(type);
}

void PackageView::onPackageClicked(const QModelIndex& index) {
    if (!index.isValid()) return;
    
    Package pkg = index.data(PackageListModel::PackageRole).value<Package>();
    showPackageDetails(pkg);
    
    emit packageSelected(pkg.name);
}

void PackageView::showPackageDetails(const Package& pkg) {
    m_currentPackage = pkg.name;
    
    m_packageNameLabel->setText("📦 " + pkg.name);
    m_packageVersionLabel->setText("<b>Version:</b> " + pkg.version);
    m_packageSizeLabel->setText("<b>Size:</b> " + pkg.formattedSize());
    m_packageDateLabel->setText("<b>Installed:</b> " + pkg.installDate.toString("yyyy-MM-dd hh:mm"));
    m_packageReasonLabel->setText(QString("<b>Reason:</b> ") + 
        (pkg.isExplicit() ? QString::fromUtf8("✅ Explicitly installed") : QString::fromUtf8("📦 Installed as dependency")));
    m_packageDescLabel->setText(pkg.description);
    
    // Dependencies
    QString deps = pkg.depends.isEmpty() ? "None" : pkg.depends.join(", ");
    m_dependsLabel->setText("<b>Depends on:</b> " + deps);
    
    QString reqBy = pkg.requiredBy.isEmpty() ? "None" : pkg.requiredBy.join(", ");
    m_requiredByLabel->setText("<b>Required by:</b> " + reqBy);
    
    // User data
    PackageUserData userData = m_database->getPackageUserData(pkg.name);
    m_notesEdit->setText(userData.notes);
    
    m_tagsList->clear();
    m_tagsList->addItems(userData.tags);
    
    m_keepBtn->setChecked(userData.markedKeep);
    m_reviewBtn->setChecked(userData.markedReview);
    
    // Update pin button based on IgnorePkg status
    bool isPinned = PacmanConfig::isPackagePinned(pkg.name);
    m_pinBtn->setText(isPinned ? "📍 Unpin" : "📌 Pin");

}

void PackageView::onSaveNotes() {
    if (m_currentPackage.isEmpty()) return;
    
    m_database->setPackageNotes(m_currentPackage, m_notesEdit->toPlainText());
    
    // Update model
    Package pkg = m_model->getPackageByName(m_currentPackage);
    pkg.userNotes = m_notesEdit->toPlainText();
    m_model->updatePackage(pkg);
}

void PackageView::onAddTag() {
    if (m_currentPackage.isEmpty()) return;
    
    QString tag = m_tagInput->text().trimmed();
    if (tag.isEmpty()) return;
    
    m_database->addPackageTag(m_currentPackage, tag);
    m_tagInput->clear();
    updateTagsList();
    
    // Update model
    Package pkg = m_model->getPackageByName(m_currentPackage);
    pkg.userTags = m_database->getPackageTags(m_currentPackage);
    m_model->updatePackage(pkg);
}

void PackageView::onRemoveTag() {
    if (m_currentPackage.isEmpty()) return;
    
    QListWidgetItem* item = m_tagsList->currentItem();
    if (!item) return;
    
    m_database->removePackageTag(m_currentPackage, item->text());
    updateTagsList();
    
    // Update model
    Package pkg = m_model->getPackageByName(m_currentPackage);
    pkg.userTags = m_database->getPackageTags(m_currentPackage);
    m_model->updatePackage(pkg);
}

void PackageView::updateTagsList() {
    m_tagsList->clear();
    m_tagsList->addItems(m_database->getPackageTags(m_currentPackage));
}

void PackageView::onToggleKeep() {
    if (m_currentPackage.isEmpty()) return;
    
    m_database->setPackageKeep(m_currentPackage, m_keepBtn->isChecked());
    
    // Update model
    Package pkg = m_model->getPackageByName(m_currentPackage);
    pkg.isMarkedKeep = m_keepBtn->isChecked();
    m_model->updatePackage(pkg);
}

void PackageView::onToggleReview() {
    if (m_currentPackage.isEmpty()) return;
    
    m_database->setPackageReview(m_currentPackage, m_reviewBtn->isChecked());
    
    // Update model
    Package pkg = m_model->getPackageByName(m_currentPackage);
    pkg.isMarkedReview = m_reviewBtn->isChecked();
    m_model->updatePackage(pkg);
}

void PackageView::onRemovePackage() {
    if (m_currentPackage.isEmpty()) return;
    
    // Get required by using pacman -Qi
    QProcess proc;
    proc.start("pacman", {"-Qi", m_currentPackage});
    proc.waitForFinished(3000);
    QString output = QString::fromUtf8(proc.readAllStandardOutput());
    
    QStringList requiredBy;
    QRegularExpression reqRe(R"(Required By\s*:\s*(.+))");
    QRegularExpressionMatch match = reqRe.match(output);
    if (match.hasMatch()) {
        QString deps = match.captured(1).trimmed();
        if (deps != "None") {
            requiredBy = deps.split(QRegularExpression(R"(\s+)"), Qt::SkipEmptyParts);
        }
    }
    
    QString message = QString("Remove <b>%1</b>?").arg(m_currentPackage);
    if (!requiredBy.isEmpty()) {
        message += QString("\n\n⚠️ <b>Warning:</b> This package is required by:\n%1")
            .arg(requiredBy.join(", "));
    }
    
    QMessageBox::StandardButton reply = QMessageBox::question(this, "Remove Package",
        message, QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        bool success = PrivilegedRunner::runCommand(
            QString("pacman -Rs %1 --noconfirm").arg(m_currentPackage),
            QString("Removing package: %1").arg(m_currentPackage),
            this);
        
        if (success) {
            QMessageBox::information(this, "Success", 
                QString("Package %1 removed successfully!").arg(m_currentPackage));
            // Refresh the package list
            loadPackages();
            m_currentPackage.clear();
        }
    }
}

void PackageView::onChangeVersion() {
    if (m_currentPackage.isEmpty()) return;
    
    // Get current installed version
    QProcess proc;
    proc.start("pacman", {"-Qi", m_currentPackage});
    proc.waitForFinished(5000);
    QString pkgInfo = QString::fromUtf8(proc.readAllStandardOutput());
    
    QRegularExpression versionRe(R"(Version\s*:\s*(\S+))");
    QRegularExpressionMatch match = versionRe.match(pkgInfo);
    QString installedVersion = match.hasMatch() ? match.captured(1) : "unknown";
    
    // Check if package is from AUR (not in official repos)
    QProcess repoCheck;
    repoCheck.start("pacman", {"-Si", m_currentPackage});
    repoCheck.waitForFinished(5000);
    bool isAurPackage = (repoCheck.exitCode() != 0);
    
    if (isAurPackage) {
        // AUR package - show AUR-specific options
        QStringList options;
        options << QString("✓ Current: %1").arg(installedVersion);
        options << "🔄 Reinstall from AUR";
        options << "🌐 Open AUR Page";
        
        bool ok;
        QString choice = QInputDialog::getItem(this, "AUR Package",
            QString("%1 is an AUR package.\n\nAUR packages don't have archived versions.\nSelect an action:")
                .arg(m_currentPackage),
            options, 0, false, &ok);
        
        if (!ok || choice.isEmpty()) return;
        
        if (choice.contains("Reinstall")) {
            // Reinstall via yay/paru
            QString aurHelper = "yay";
            QProcess whichProc;
            whichProc.start("which", {"yay"});
            whichProc.waitForFinished(2000);
            if (whichProc.exitCode() != 0) {
                aurHelper = "paru";
            }
            
            bool success = PrivilegedRunner::runCommand(
                QString("%1 -S %2 --noconfirm").arg(aurHelper).arg(m_currentPackage),
                QString("Reinstalling AUR package: %1").arg(m_currentPackage),
                this);
            
            if (success) {
                QMessageBox::information(this, "Success",
                    QString("Package %1 reinstalled from AUR!").arg(m_currentPackage));
                refreshCurrentPackage();
            }
        } else if (choice.contains("Open AUR")) {
            QDesktopServices::openUrl(QUrl(
                QString("https://aur.archlinux.org/packages/%1").arg(m_currentPackage)));
        }
        return;
    }
    
    // Official repo package - fetch from Arch Linux Archive
    // Show progress while fetching
    QProgressDialog progress("Fetching available versions...", "Cancel", 0, 0, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.show();
    qApp->processEvents();
    
    // Fetch versions from Arch Linux Archive
    QNetworkAccessManager manager;
    QString archiveUrl = QString("https://archive.archlinux.org/packages/%1/%2/")
        .arg(m_currentPackage.at(0).toLower())
        .arg(m_currentPackage);
    
    QUrl url(archiveUrl);
    QNetworkRequest request(url);
    QNetworkReply* reply = manager.get(request);
    
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    
    progress.close();
    
    if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::warning(this, "Error", 
            QString("Failed to fetch versions from archive:\n%1").arg(reply->errorString()));
        reply->deleteLater();
        return;
    }
    
    QString html = QString::fromUtf8(reply->readAll());
    reply->deleteLater();
    
    // Parse versions from archive HTML
    // Format: packagename-version-arch.pkg.tar.zst DATE SIZE
    // Note: epoch versions like 2:8.0.1 are URL-encoded as %3A or literal colon
    QRegularExpression pkgRe(QString(R"(%1-((?:\d+:)?[\d\.\w]+-\d+)-x86_64\.pkg\.tar\.(?:zst|xz)[^\s]*\s+(\d+-\w+-\d+)\s+[\d:]+\s+([\d\.]+[KMG]?))")
        .arg(QRegularExpression::escape(m_currentPackage)));
    
    QRegularExpressionMatchIterator iter = pkgRe.globalMatch(html);
    
    struct VersionInfo {
        QString version;
        QString date;
        QString size;
    };
    QVector<VersionInfo> versionList;
    QSet<QString> seenVersions; // Avoid duplicates
    
    while (iter.hasNext()) {
        QRegularExpressionMatch m = iter.next();
        QString ver = m.captured(1);
        QString date = m.captured(2);
        QString size = m.captured(3);
        
        if (!seenVersions.contains(ver)) {
            seenVersions.insert(ver);
            versionList.append({ver, date, size});
        }
    }
    
    if (versionList.isEmpty()) {
        QMessageBox::information(this, "No Versions Found",
            QString("Could not find archived versions for %1.\n\n"
                    "This package might not be in the archive, or it may be an AUR package.")
            .arg(m_currentPackage));
        return;
    }
    
    // Sort by date (newest first) - versions are in chronological order in archive
    std::reverse(versionList.begin(), versionList.end());
    
    // Build display options with date info
    QStringList displayOptions;
    displayOptions << QString("⟳ Reinstall current: %1").arg(installedVersion);
    
    QStringList versions; // For extracting version on selection
    versions << installedVersion;
    
    for (const VersionInfo& info : versionList) {
        QString displayStr;
        if (info.version == installedVersion) {
            displayStr = QString("✓ %1 (%2, %3) [installed]").arg(info.version).arg(info.date).arg(info.size);
        } else {
            displayStr = QString("%1 (%2, %3)").arg(info.version).arg(info.date).arg(info.size);
        }
        displayOptions << displayStr;
        versions << info.version;
    }
    
    bool ok;
    QString choice = QInputDialog::getItem(this, "Select Version",
        QString("Available versions for %1:\n(Installed: %2)")
            .arg(m_currentPackage)
            .arg(installedVersion),
        displayOptions, 0, false, &ok);
    
    if (!ok || choice.isEmpty()) return;
    
    // Find selected index to get actual version
    int selectedIndex = displayOptions.indexOf(choice);
    if (selectedIndex < 0) return;
    
    if (choice.startsWith("⟳")) {
        // Reinstall current
        bool success = PrivilegedRunner::runCommand(
            QString("pacman -S %1 --noconfirm").arg(m_currentPackage),
            QString("Reinstalling package: %1").arg(m_currentPackage),
            this);
        
        if (success) {
            QMessageBox::information(this, "Success", 
                QString("Package %1 reinstalled successfully!").arg(m_currentPackage));
            refreshCurrentPackage();
        }
    } else if (!choice.startsWith("✓")) {
        // Get actual version from our list
        QString selectedVersion = versions.at(selectedIndex);
        
        // URL-encode colon for epoch versions (2:8.0.1 -> 2%3A8.0.1)
        QString urlVersion = selectedVersion;
        urlVersion.replace(":", "%3A");
        
        // Try .zst first, fall back to .xz for older packages
        QString pkgUrl = QString("https://archive.archlinux.org/packages/%1/%2/%2-%3-x86_64.pkg.tar.zst")
            .arg(m_currentPackage.at(0).toLower())
            .arg(m_currentPackage)
            .arg(urlVersion);
        
        bool success = PrivilegedRunner::runCommand(
            QString("pacman -U \"%1\" --noconfirm").arg(pkgUrl),
            QString("Installing %1 version %2").arg(m_currentPackage).arg(selectedVersion),
            this);
        
        if (!success) {
            // Try .xz format for older packages
            pkgUrl = QString("https://archive.archlinux.org/packages/%1/%2/%2-%3-x86_64.pkg.tar.xz")
                .arg(m_currentPackage.at(0).toLower())
                .arg(m_currentPackage)
                .arg(urlVersion);
            
            success = PrivilegedRunner::runCommand(
                QString("pacman -U \"%1\" --noconfirm").arg(pkgUrl),
                QString("Installing %1 version %2").arg(m_currentPackage).arg(selectedVersion),
                this);
        }
        
        if (success) {
            QMessageBox::information(this, "Success", 
                QString("Package %1 changed to version %2!").arg(m_currentPackage).arg(selectedVersion));
            refreshCurrentPackage();
        }
    }
}

void PackageView::refreshCurrentPackage() {
    if (m_currentPackage.isEmpty()) return;
    
    // Refresh package manager data
    m_packageManager->refresh();
    
    // Reload packages to get fresh data
    loadPackages();
    
    // Re-show details for current package
    Package pkg = m_model->getPackageByName(m_currentPackage);
    if (!pkg.name.isEmpty()) {
        showPackageDetails(pkg);
    }
}

void PackageView::onTogglePin() {
    if (m_currentPackage.isEmpty()) return;
    
    bool isPinned = PacmanConfig::isPackagePinned(m_currentPackage);
    
    if (isPinned) {
        // Unpin
        if (PacmanConfig::removeIgnoredPackage(m_currentPackage)) {
            m_pinBtn->setText("📌 Pin");
            QMessageBox::information(this, "Unpinned",
                QString("Package %1 has been unpinned.\n"
                        "It will be updated normally with pacman -Syu.")
                .arg(m_currentPackage));
        } else {
            QMessageBox::warning(this, "Error",
                "Failed to unpin package. Make sure you have the required permissions.");
        }
    } else {
        // Pin
        QMessageBox::StandardButton reply = QMessageBox::question(this, "Pin Package?",
            QString("Pin <b>%1</b>?\n\n"
                    "This will add it to IgnorePkg in pacman.conf.\n"
                    "The package will NOT be updated by pacman -Syu.\n\n"
                    "Note: This requires root access.")
            .arg(m_currentPackage),
            QMessageBox::Yes | QMessageBox::No);
        
        if (reply == QMessageBox::Yes) {
            if (PacmanConfig::addIgnoredPackage(m_currentPackage)) {
                m_pinBtn->setText("📍 Unpin");
                QMessageBox::information(this, "Pinned",
                    QString("Package %1 has been pinned.\n"
                            "It will NOT be updated by pacman -Syu.")
                    .arg(m_currentPackage));
            } else {
                QMessageBox::warning(this, "Error",
                    "Failed to pin package. Make sure you have the required permissions.");
            }
        }
    }
}

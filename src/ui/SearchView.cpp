#include "SearchView.h"
#include "core/AURClient.h"
#include "core/PackageManager.h"
#include "PrivilegedRunner.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QProcess>
#include <QSplitter>

SearchView::SearchView(PackageManager* pm, AURClient* aur, QWidget* parent)
    : QWidget(parent)
    , m_packageManager(pm)
    , m_aurClient(aur)
{
    setupUI();
}

void SearchView::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    
    // Header
    QLabel* headerLabel = new QLabel("🔍 Search Packages");
    headerLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #cdd6f4;");
    mainLayout->addWidget(headerLabel);
    
    QLabel* subLabel = new QLabel("Search for packages in AUR or official repositories");
    subLabel->setStyleSheet("color: #a6adc8; margin-bottom: 10px;");
    mainLayout->addWidget(subLabel);
    
    // Search bar
    QHBoxLayout* searchLayout = new QHBoxLayout();
    
    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText("Enter package name to search...");
    m_searchEdit->setMinimumHeight(40);
    m_searchEdit->setStyleSheet(R"(
        QLineEdit {
            background-color: #313244;
            border: 2px solid #45475a;
            border-radius: 8px;
            padding: 8px 15px;
            color: #cdd6f4;
            font-size: 14px;
        }
        QLineEdit:focus {
            border-color: #89b4fa;
        }
    )");
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &SearchView::performSearch);
    searchLayout->addWidget(m_searchEdit);
    
    m_sourceCombo = new QComboBox();
    m_sourceCombo->addItem("🌐 AUR", "aur");
    m_sourceCombo->addItem("📦 Official Repos", "repo");
    m_sourceCombo->addItem("📂 Find by File", "file");
    m_sourceCombo->setMinimumHeight(40);
    m_sourceCombo->setMinimumWidth(180);
    m_sourceCombo->setStyleSheet(R"(
        QComboBox {
            background-color: #313244;
            border: 2px solid #45475a;
            border-radius: 8px;
            padding: 8px 15px;
            color: #cdd6f4;
        }
        QComboBox::drop-down {
            border: none;
        }
        QComboBox::down-arrow {
            image: none;
            border: none;
        }
    )");
    searchLayout->addWidget(m_sourceCombo);
    
    m_searchBtn = new QPushButton("Search");
    m_searchBtn->setMinimumHeight(40);
    m_searchBtn->setMinimumWidth(100);
    m_searchBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #89b4fa;
            color: #1e1e2e;
            border: none;
            border-radius: 8px;
            font-weight: bold;
            font-size: 14px;
        }
        QPushButton:hover {
            background-color: #b4befe;
        }
    )");
    connect(m_searchBtn, &QPushButton::clicked, this, &SearchView::performSearch);
    searchLayout->addWidget(m_searchBtn);
    
    mainLayout->addLayout(searchLayout);
    
    // Splitter for results and details
    QSplitter* splitter = new QSplitter(Qt::Horizontal);
    
    // Results table
    QGroupBox* resultsGroup = new QGroupBox("Search Results");
    QVBoxLayout* resultsLayout = new QVBoxLayout(resultsGroup);
    
    m_statusLabel = new QLabel("Enter a search term and click Search");
    m_statusLabel->setStyleSheet("color: #a6adc8;");
    resultsLayout->addWidget(m_statusLabel);
    
    m_resultsTable = new QTableWidget();
    m_resultsTable->setColumnCount(4);
    m_resultsTable->setHorizontalHeaderLabels({"Name", "Version", "Status", "Description"});
    m_resultsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_resultsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_resultsTable->horizontalHeader()->setStretchLastSection(true);
    m_resultsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_resultsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_resultsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_resultsTable->verticalHeader()->setVisible(false);
    m_resultsTable->setAlternatingRowColors(true);
    connect(m_resultsTable, &QTableWidget::cellClicked, this, &SearchView::onResultClicked);
    resultsLayout->addWidget(m_resultsTable);
    
    splitter->addWidget(resultsGroup);
    
    // Details panel
    QGroupBox* detailsGroup = new QGroupBox("Package Details");
    QVBoxLayout* detailsLayout = new QVBoxLayout(detailsGroup);
    
    m_infoText = new QTextEdit();
    m_infoText->setReadOnly(true);
    m_infoText->setPlaceholderText("Select a package to view details");
    detailsLayout->addWidget(m_infoText);
    
    m_installBtn = new QPushButton("📥 Install Package");
    m_installBtn->setMinimumHeight(45);
    m_installBtn->setEnabled(false);
    m_installBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #a6e3a1;
            color: #1e1e2e;
            border: none;
            border-radius: 8px;
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
    connect(m_installBtn, &QPushButton::clicked, this, &SearchView::onInstallClicked);
    detailsLayout->addWidget(m_installBtn);
    
    splitter->addWidget(detailsGroup);
    splitter->setSizes({500, 400});
    
    mainLayout->addWidget(splitter);
}

void SearchView::performSearch() {
    QString query = m_searchEdit->text().trimmed();
    if (query.isEmpty()) {
        m_statusLabel->setText("Please enter a search term");
        return;
    }
    
    m_searchResults.clear();
    m_resultsTable->setRowCount(0);
    m_statusLabel->setText("Searching...");
    m_infoText->clear();
    m_installBtn->setEnabled(false);
    
    QString source = m_sourceCombo->currentData().toString();
    
    if (source == "aur") {
        searchAUR(query);
    } else if (source == "file") {
        searchByFile(query);
    } else {
        searchRepo(query);
    }
}

void SearchView::searchAUR(const QString& query) {
    // Disable controls during search
    m_searchBtn->setEnabled(false);
    m_searchEdit->setEnabled(false);
    m_statusLabel->setText("Searching AUR... ⏳");
    setCursor(Qt::WaitCursor);
    
    // Use AURClient to search
    connect(m_aurClient, &AURClient::searchCompleted, this, [this, query](const QList<AURPackage>& packages) {
        m_statusLabel->setText(QString("Found %1 packages in AUR").arg(packages.size()));
        m_resultsTable->setRowCount(0);
        m_searchResults.clear();
        
        // Convert to internal format
        for (const AURPackage& pkg : packages) {
            PackageResult result;
            result.name = pkg.name;
            result.version = pkg.version;
            result.description = pkg.description;
            result.source = "aur";
            result.installed = m_packageManager->packageExists(pkg.name);
            m_searchResults.append(result);
        }
        
        // Sort: exact matches first, then startsWith, then alphabetical
        QString searchTerm = query.toLower();
        std::sort(m_searchResults.begin(), m_searchResults.end(),
            [&searchTerm](const PackageResult& a, const PackageResult& b) {
                bool aExact = (a.name.toLower() == searchTerm);
                bool bExact = (b.name.toLower() == searchTerm);
                if (aExact != bExact) return aExact;
                
                bool aStarts = a.name.toLower().startsWith(searchTerm);
                bool bStarts = b.name.toLower().startsWith(searchTerm);
                if (aStarts != bStarts) return aStarts;
                
                return a.name < b.name;
            });
        
        // Display sorted results
        m_resultsTable->setRowCount(qMin(m_searchResults.size(), 100));
        for (int i = 0; i < m_resultsTable->rowCount(); ++i) {
            const PackageResult& result = m_searchResults[i];
            
            m_resultsTable->setItem(i, 0, new QTableWidgetItem(result.name));
            m_resultsTable->setItem(i, 1, new QTableWidgetItem(result.version));
            
            QString status = result.installed ? "✅ Installed" : "⬇️ Available";
            QTableWidgetItem* statusItem = new QTableWidgetItem(status);
            statusItem->setForeground(result.installed ? QColor("#a6e3a1") : QColor("#89b4fa"));
            m_resultsTable->setItem(i, 2, statusItem);
            
            m_resultsTable->setItem(i, 3, new QTableWidgetItem(result.description));
        }
        
        // Re-enable controls
        m_searchBtn->setEnabled(true);
        m_searchEdit->setEnabled(true);
        setCursor(Qt::ArrowCursor);
    }, Qt::SingleShotConnection);
    
    // Error handling
    connect(m_aurClient, &AURClient::error, this, [this](const QString& errorMsg) {
        m_statusLabel->setText("Error: " + errorMsg);
        m_searchBtn->setEnabled(true);
        m_searchEdit->setEnabled(true);
        setCursor(Qt::ArrowCursor);
    }, Qt::SingleShotConnection);
    
    m_aurClient->search(query);
}

void SearchView::searchRepo(const QString& query) {
    // Disable controls during search
    m_searchBtn->setEnabled(false);
    m_searchEdit->setEnabled(false);
    m_statusLabel->setText("Searching Repos... ⏳");
    setCursor(Qt::WaitCursor);
    
    QProcess* proc = new QProcess(this);
    connect(proc, &QProcess::finished, this, [this, proc](int exitCode) {
        m_searchBtn->setEnabled(true);
        m_searchEdit->setEnabled(true);
        setCursor(Qt::ArrowCursor);
        
        if (exitCode != 0) {
            m_statusLabel->setText("Error: Failed to search repositories. Check if another pacman instance is running.");
            proc->deleteLater();
            return;
        }
        
        QString output = QString::fromUtf8(proc->readAllStandardOutput());
        proc->deleteLater();
        
        m_searchResults.clear();
        QStringList lines = output.split('\n');
        
        QString currentName, currentVersion, currentDesc;
        bool inPackage = false;
        
        for (const QString& line : lines) {
            if (line.isEmpty()) continue;
            
            // Description lines start with spaces
            if (line.startsWith("    ") || line.startsWith("\t")) {
                if (inPackage) {
                    if (!currentDesc.isEmpty()) currentDesc += " ";
                    currentDesc += line.trimmed();
                }
            } else {
                // Save previous package if exists
                if (inPackage && !currentName.isEmpty()) {
                    PackageResult result;
                    result.name = currentName;
                    result.version = currentVersion;
                    result.description = currentDesc;
                    result.source = "repo";
                    result.installed = m_packageManager->packageExists(result.name);
                    m_searchResults.append(result);
                }
                
                // Parse new package line: repo/pkgname version [installed]
                QRegularExpression re(R"(^(\w+)/(\S+)\s+(\S+))");
                QRegularExpressionMatch match = re.match(line);
                
                if (match.hasMatch()) {
                    currentName = match.captured(2);
                    currentVersion = match.captured(3);
                    currentDesc.clear();
                    inPackage = true;
                } else {
                    inPackage = false;
                }
            }
        }
        
        // Don't forget last package
        if (inPackage && !currentName.isEmpty()) {
            PackageResult result;
            result.name = currentName;
            result.version = currentVersion;
            result.description = currentDesc;
            result.source = "repo";
            result.installed = m_packageManager->packageExists(result.name);
            m_searchResults.append(result);
        }
        
        // Sort: exact matches first, then by name
        QString searchTerm = m_searchEdit->text().trimmed().toLower();
        std::sort(m_searchResults.begin(), m_searchResults.end(), 
            [&searchTerm](const PackageResult& a, const PackageResult& b) {
                bool aExact = (a.name.toLower() == searchTerm);
                bool bExact = (b.name.toLower() == searchTerm);
                if (aExact != bExact) return aExact;
                
                bool aStarts = a.name.toLower().startsWith(searchTerm);
                bool bStarts = b.name.toLower().startsWith(searchTerm);
                if (aStarts != bStarts) return aStarts;
                
                return a.name < b.name;
            });
        
        m_resultsTable->setRowCount(m_searchResults.size());
        for (int i = 0; i < m_searchResults.size(); ++i) {
            const PackageResult& result = m_searchResults[i];
            
            m_resultsTable->setItem(i, 0, new QTableWidgetItem(result.name));
            m_resultsTable->setItem(i, 1, new QTableWidgetItem(result.version));
            
            QString status = result.installed ? "✅ Installed" : "⬇️ Available";
            QTableWidgetItem* statusItem = new QTableWidgetItem(status);
            statusItem->setForeground(result.installed ? QColor("#a6e3a1") : QColor("#89b4fa"));
            m_resultsTable->setItem(i, 2, statusItem);
            
            m_resultsTable->setItem(i, 3, new QTableWidgetItem(result.description));
        }
        
        m_statusLabel->setText(QString("Found %1 packages in official repos").arg(m_searchResults.size()));
    });
    
    proc->start("pacman", {"-Ss", query});
}

void SearchView::onResultClicked(int row, int column) {
    Q_UNUSED(column)
    showPackageInfo(row);
}

void SearchView::showPackageInfo(int row) {
    if (row < 0 || row >= m_searchResults.size()) return;
    
    const PackageResult& pkg = m_searchResults[row];
    
    QString info = QString(
        "<h2>%1</h2>"
        "<p><b>Version:</b> %2</p>"
        "<p><b>Source:</b> %3</p>"
        "<p><b>Status:</b> %4</p>"
        "<hr>"
        "<p>%5</p>"
    ).arg(pkg.name)
     .arg(pkg.version)
     .arg(pkg.source == "aur" ? "Arch User Repository (AUR)" : "Official Repository")
     .arg(pkg.installed ? "Installed" : "Not installed")
     .arg(pkg.description);
    
    m_infoText->setHtml(info);
    
    m_installBtn->setEnabled(!pkg.installed);
    if (pkg.installed) {
        m_installBtn->setText("✅ Already Installed");
    } else if (pkg.source == "aur") {
        m_installBtn->setText("📥 Install from AUR (requires yay/paru)");
    } else {
        m_installBtn->setText("📥 Install Package (requires sudo)");
    }
}

void SearchView::onInstallClicked() {
    int row = m_resultsTable->currentRow();
    if (row < 0 || row >= m_searchResults.size()) return;
    
    const PackageResult& pkg = m_searchResults[row];
    
    if (pkg.installed) {
        // Remove package
        QMessageBox::StandardButton reply = QMessageBox::question(this, "Remove Package",
            QString("Remove <b>%1</b>?\n\n"
                    "This will uninstall the package and optionally\n"
                    "remove orphaned dependencies.").arg(pkg.name),
            QMessageBox::Yes | QMessageBox::No);
        
        if (reply == QMessageBox::Yes) {
            bool success = PrivilegedRunner::runCommand(
                QString("pacman -Rs %1 --noconfirm").arg(pkg.name),
                QString("Removing package: %1").arg(pkg.name),
                this);
            
            if (success) {
                QMessageBox::information(this, "Success", 
                    QString("Package %1 removed successfully!").arg(pkg.name));
                // Update the UI
                m_searchResults[row].installed = false;
                showPackageInfo(row);
            }
        }
    } else {
        // Install package
        QString command;
        QString description;
        
        if (pkg.source == "aur") {
            // For AUR, we need an AUR helper
            command = QString("yay -S %1 --noconfirm || paru -S %1 --noconfirm").arg(pkg.name);
            description = QString("Installing AUR package: %1").arg(pkg.name);
        } else {
            command = QString("pacman -S %1 --noconfirm").arg(pkg.name);
            description = QString("Installing package: %1").arg(pkg.name);
        }
        
        bool success = PrivilegedRunner::runCommand(command, description, this);
        
        if (success) {
            QMessageBox::information(this, "Success", 
                QString("Package %1 installed successfully!").arg(pkg.name));
            // Update the UI
            m_searchResults[row].installed = true;
            showPackageInfo(row);
        }
    }
}

void SearchView::searchByFile(const QString& filename) {
    // Search for packages that provide a specific file using pacman -F
    m_searchBtn->setEnabled(false);
    m_searchEdit->setEnabled(false);
    m_statusLabel->setText("Searching files... ⏳");
    setCursor(Qt::WaitCursor);
    
    QProcess* proc = new QProcess(this);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, proc, filename](int exitCode, QProcess::ExitStatus status) {
        proc->deleteLater();
        
        QString output = QString::fromUtf8(proc->readAllStandardOutput());
        QString error = QString::fromUtf8(proc->readAllStandardError());
        
        // Check if file database needs sync
        if (error.contains("database") || output.isEmpty()) {
            m_statusLabel->setText("⚠️ File database may need sync. Click Sync button.");
            m_searchBtn->setEnabled(true);
            m_searchEdit->setEnabled(true);
            setCursor(Qt::ArrowCursor);
            
            // Show hint about syncing
            QMessageBox::information(this, "Sync Required",
                "The file database may need to be updated.\n\n"
                "Run 'sudo pacman -Fy' to sync the file database.\n"
                "You can also use the Search tab again after syncing.");
            return;
        }
        
        // Parse output: repo/package version
        //   /path/to/file
        m_searchResults.clear();
        m_resultsTable->setRowCount(0);
        
        QStringList lines = output.split('\n', Qt::SkipEmptyParts);
        QString currentPkg, currentVersion, currentRepo;
        QStringList currentFiles;
        
        for (const QString& line : lines) {
            if (!line.startsWith(' ') && !line.startsWith('\t')) {
                // Package line: repo/package version
                if (!currentPkg.isEmpty()) {
                    // Save previous package
                    PackageResult result;
                    result.name = currentPkg;
                    result.version = currentVersion;
                    result.description = currentFiles.join(", ");
                    result.source = currentRepo;
                    result.installed = m_packageManager->packageExists(currentPkg);
                    m_searchResults.append(result);
                }
                
                // Parse new package
                QStringList parts = line.split(' ', Qt::SkipEmptyParts);
                if (parts.size() >= 2) {
                    QString repoAndName = parts[0];
                    int slashPos = repoAndName.indexOf('/');
                    if (slashPos > 0) {
                        currentRepo = repoAndName.left(slashPos);
                        currentPkg = repoAndName.mid(slashPos + 1);
                    } else {
                        currentPkg = repoAndName;
                        currentRepo = "repo";
                    }
                    currentVersion = parts[1];
                    currentFiles.clear();
                }
            } else {
                // File line
                QString file = line.trimmed();
                if (!file.isEmpty()) {
                    currentFiles.append(file);
                }
            }
        }
        
        // Don't forget last package
        if (!currentPkg.isEmpty()) {
            PackageResult result;
            result.name = currentPkg;
            result.version = currentVersion;
            result.description = currentFiles.join(", ");
            result.source = currentRepo;
            result.installed = m_packageManager->packageExists(currentPkg);
            m_searchResults.append(result);
        }
        
        // Display results
        m_resultsTable->setRowCount(m_searchResults.size());
        for (int i = 0; i < m_searchResults.size(); ++i) {
            const PackageResult& result = m_searchResults[i];
            
            m_resultsTable->setItem(i, 0, new QTableWidgetItem(result.name));
            m_resultsTable->setItem(i, 1, new QTableWidgetItem(result.version));
            
            QString status = result.installed ? "✅ Installed" : "⬇️ Available";
            QTableWidgetItem* statusItem = new QTableWidgetItem(status);
            statusItem->setForeground(result.installed ? QColor("#a6e3a1") : QColor("#89b4fa"));
            m_resultsTable->setItem(i, 2, statusItem);
            
            // Show files in description column
            m_resultsTable->setItem(i, 3, new QTableWidgetItem(QString("📂 %1").arg(result.description)));
        }
        
        m_statusLabel->setText(QString("Found %1 packages providing '%2'")
            .arg(m_searchResults.size()).arg(filename));
        
        m_searchBtn->setEnabled(true);
        m_searchEdit->setEnabled(true);
        setCursor(Qt::ArrowCursor);
    });
    
    proc->start("pacman", {"-F", filename});
}

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
    // Apply initial theme based on Config or default
    // We'll let MainWindow call applyTheme, but we should have a default
}

void SearchView::applyTheme(bool isDark) {
    QString bgColor = isDark ? "#1e1e2e" : "#eff1f5";
    QString textColor = isDark ? "#cdd6f4" : "#4c4f69";
    QString subTextColor = isDark ? "#a6adc8" : "#6c6f85";
    QString borderColor = isDark ? "#45475a" : "#ccd0da";
    QString headerColor = isDark ? "#89b4fa" : "#1e66f5";
    QString inputBg = isDark ? "#313244" : "#e6e9ef";
    
    // Header
    QList<QLabel*> labels = findChildren<QLabel*>();
    for(auto label : labels) {
        if(label->text().contains("Search Packages")) {
             label->setStyleSheet(QString("font-size: 24px; font-weight: bold; color: %1;").arg(textColor));
        } else if(label->text().contains("Search for packages in")) {
             label->setStyleSheet(QString("color: %1; margin-bottom: 10px;").arg(subTextColor));
        } else if(label == m_statusLabel) {
             label->setStyleSheet(QString("color: %1;").arg(subTextColor));
        }
    }
    
    // Input fields style
    QString inputStyle = QString(R"(
        QLineEdit {
            background-color: %1;
            border: 2px solid %2;
            border-radius: 8px;
            padding: 8px 15px;
            color: %3;
            font-size: 14px;
        }
        QLineEdit:focus {
            border-color: %4;
        }
    )").arg(inputBg, borderColor, textColor, headerColor);
    
    if(m_searchEdit) m_searchEdit->setStyleSheet(inputStyle);
    
    // Combo box
    if(m_sourceCombo) {
        m_sourceCombo->setStyleSheet(QString(R"(
            QComboBox {
                background-color: %1;
                border: 2px solid %2;
                border-radius: 8px;
                padding: 8px 15px;
                color: %3;
            }
            QComboBox::drop-down {
                border: none;
            }
        )").arg(inputBg, borderColor, textColor));
    }
    
    // Search Button
    if(m_searchBtn) {
        m_searchBtn->setStyleSheet(QString(R"(
            QPushButton {
                background-color: %1;
                color: %2;
                border: none;
                border-radius: 8px;
                font-weight: bold;
                font-size: 14px;
            }
            QPushButton:hover {
                background-color: %3;
            }
        )").arg(headerColor, isDark ? "#1e1e2e" : "#ffffff", isDark ? "#b4befe" : "#7287fd"));
    }
    
    // Install Button
    if(m_installBtn) {
        m_installBtn->setStyleSheet(QString(R"(
            QPushButton {
                background-color: %1;
                color: %2;
                border: none;
                border-radius: 8px;
                font-weight: bold;
                font-size: 14px;
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
            isDark ? "#45475a" : "#ccd0da",
            isDark ? "#6c7086" : "#9ca0b0"
        ));
    }
    
    // Table View
    if (m_resultsTable) {
        m_resultsTable->setStyleSheet(QString(R"(
            QTableWidget {
                background-color: %1;
                alternate-background-color: %2;
                color: %3;
                border: 1px solid %4;
                border-radius: 8px;
                selection-background-color: %5;
                selection-color: %3;
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
            isDark ? "#45475a" : "#bcc0cc")); // Selection Bg
            
        // We also need to update row colors dynamically, but they are set in performSearch.
        // We can't easily retroactively update items without iterating them.
        // So we just rely on TableWidget style for main text, and custom items will be rebuilt on search.
        // HOWEVER, we SHOULD update existing items if possible.
        for(int i=0; i<m_resultsTable->rowCount(); i++) {
            // Update status item color
             QTableWidgetItem* statusItem = m_resultsTable->item(i, 2);
             if (statusItem) {
                 bool installed = statusItem->text().contains("Installed");
                 if (installed) statusItem->setForeground(QColor(isDark ? "#a6e3a1" : "#40a02b"));
                 else statusItem->setForeground(QColor(isDark ? "#89b4fa" : "#1e66f5"));
             }
        }
    }
}

void SearchView::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    
    // Header
    QLabel* headerLabel = new QLabel("🔍 Search Packages");
    // Style applied by applyTheme
    mainLayout->addWidget(headerLabel);
    
    QLabel* subLabel = new QLabel("Search for packages in AUR or official repositories");
    // Style applied by applyTheme
    mainLayout->addWidget(subLabel);
    
    // Search bar
    QHBoxLayout* searchLayout = new QHBoxLayout();
    
    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText("Enter package name to search...");
    m_searchEdit->setMinimumHeight(40);
    m_searchEdit->setMinimumHeight(40);
    // Style applied by applyTheme
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &SearchView::performSearch);
    searchLayout->addWidget(m_searchEdit);
    
    m_sourceCombo = new QComboBox();
    m_sourceCombo->addItem("🌐 AUR", "aur");
    m_sourceCombo->addItem("📦 Official Repos", "repo");
    m_sourceCombo->addItem("📂 Find by File", "file");
    m_sourceCombo->setMinimumHeight(40);
    m_sourceCombo->setMinimumWidth(180);
    m_sourceCombo->setMinimumHeight(40);
    m_sourceCombo->setMinimumWidth(180);
    // Style applied by applyTheme
    searchLayout->addWidget(m_sourceCombo);
    
    m_searchBtn = new QPushButton("Search");
    m_searchBtn->setMinimumHeight(40);
    m_searchBtn->setMinimumWidth(100);
    m_searchBtn->setMinimumHeight(40);
    m_searchBtn->setMinimumWidth(100);
    // Style applied by applyTheme
    connect(m_searchBtn, &QPushButton::clicked, this, &SearchView::performSearch);
    searchLayout->addWidget(m_searchBtn);
    
    mainLayout->addLayout(searchLayout);
    
    // Splitter for results and details
    QSplitter* splitter = new QSplitter(Qt::Horizontal);
    
    // Results table
    QGroupBox* resultsGroup = new QGroupBox("Search Results");
    QVBoxLayout* resultsLayout = new QVBoxLayout(resultsGroup);
    
    m_statusLabel = new QLabel("Enter a search term and click Search");
    m_statusLabel = new QLabel("Enter a search term and click Search");
    // Style applied by applyTheme
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
    m_installBtn = new QPushButton("📥 Install Package");
    m_installBtn->setMinimumHeight(45);
    m_installBtn->setEnabled(false);
    // Style applied by applyTheme
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
    
    m_installBtn->setEnabled(true);
    if (pkg.installed) {
        m_installBtn->setText("🗑️ Remove Package");
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
                QString("pacman -Rs %1").arg(pkg.name),
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
            command = QString("yay -S %1 || paru -S %1").arg(pkg.name);
            description = QString("Installing AUR package: %1").arg(pkg.name);
        } else {
            command = QString("pacman -S %1").arg(pkg.name);
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

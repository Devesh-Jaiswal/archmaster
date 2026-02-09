#include "ProfileView.h"
#include "core/ProfileManager.h"
#include "core/PackageManager.h"
#include "PrivilegedRunner.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QSplitter>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QMenu>
#include <QProcess>
#include <QDialogButtonBox>

ProfileView::ProfileView(ProfileManager* pm, PackageManager* pkgMgr, QWidget* parent)
    : QWidget(parent)
    , m_profileManager(pm)
    , m_packageManager(pkgMgr)
{
    setupUI();
    refreshProfiles();
}

void ProfileView::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    
    // Header
    QLabel* headerLabel = new QLabel("📋 Package Profiles");
    headerLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #cdd6f4;");
    mainLayout->addWidget(headerLabel);
    
    QLabel* subLabel = new QLabel("Save and restore package configurations for quick environment setup");
    subLabel->setStyleSheet("color: #a6adc8; margin-bottom: 10px;");
    mainLayout->addWidget(subLabel);
    
    // Main content splitter
    QSplitter* splitter = new QSplitter(Qt::Horizontal);
    
    // Left panel - Profile list
    QGroupBox* profileGroup = new QGroupBox("Available Profiles");
    QVBoxLayout* profileLayout = new QVBoxLayout(profileGroup);
    
    m_profileList = new QListWidget();
    m_profileList->setStyleSheet(R"(
        QListWidget {
            background-color: #313244;
            border: 1px solid #45475a;
            border-radius: 8px;
            color: #cdd6f4;
            font-size: 14px;
        }
        QListWidget::item {
            padding: 10px;
            border-bottom: 1px solid #45475a;
        }
        QListWidget::item:selected {
            background-color: #89b4fa;
            color: #1e1e2e;
        }
        QListWidget::item:hover {
            background-color: #45475a;
        }
    )");
    connect(m_profileList, &QListWidget::currentRowChanged, this, &ProfileView::onProfileSelected);
    profileLayout->addWidget(m_profileList);
    
    // Profile action buttons
    QHBoxLayout* profileBtnLayout = new QHBoxLayout();
    
    m_createBtn = new QPushButton("➕ Create from System");
    m_createBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #a6e3a1;
            color: #1e1e2e;
            border: none;
            border-radius: 6px;
            padding: 8px 12px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #94e2d5;
        }
    )");
    connect(m_createBtn, &QPushButton::clicked, this, &ProfileView::onCreateClicked);
    
    m_deleteBtn = new QPushButton("🗑️ Delete");
    m_deleteBtn->setStyleSheet(R"(
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
    connect(m_deleteBtn, &QPushButton::clicked, this, &ProfileView::onDeleteClicked);
    
    m_createCustomBtn = new QPushButton("✨ Create Custom");
    m_createCustomBtn->setStyleSheet(R"(
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
    connect(m_createCustomBtn, &QPushButton::clicked, this, &ProfileView::onCreateCustomClicked);
    
    m_editBtn = new QPushButton("✏️ Edit");
    m_editBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #cba6f7;
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
    m_editBtn->setEnabled(false);
    connect(m_editBtn, &QPushButton::clicked, this, &ProfileView::onEditClicked);
    
    // Row 1: Create buttons
    profileBtnLayout->addWidget(m_createBtn);
    profileBtnLayout->addWidget(m_createCustomBtn);
    profileLayout->addLayout(profileBtnLayout);
    
    // Row 2: Edit/Delete/Export/Import
    QHBoxLayout* profileBtnLayout2 = new QHBoxLayout();
    
    m_importBtn = new QPushButton("📥 Import");
    m_importBtn->setStyleSheet(R"(
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
    connect(m_importBtn, &QPushButton::clicked, this, &ProfileView::onImportClicked);
    
    m_exportBtn = new QPushButton("📤 Export");
    m_exportBtn->setToolTip("Export selected profile");
    m_exportBtn->setStyleSheet(R"(
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
    connect(m_exportBtn, &QPushButton::clicked, this, &ProfileView::onExportClicked);
    
    profileBtnLayout2->addWidget(m_editBtn);
    profileBtnLayout2->addWidget(m_deleteBtn);
    profileBtnLayout2->addWidget(m_importBtn);
    profileBtnLayout2->addWidget(m_exportBtn);
    profileLayout->addLayout(profileBtnLayout2);
    
    splitter->addWidget(profileGroup);
    
    // Right panel - Profile details
    QGroupBox* detailsGroup = new QGroupBox("Profile Details");
    QVBoxLayout* detailsLayout = new QVBoxLayout(detailsGroup);
    
    m_profileNameLabel = new QLabel("Select a profile");
    m_profileNameLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #89b4fa;");
    detailsLayout->addWidget(m_profileNameLabel);
    
    m_profileDescLabel = new QLabel("");
    m_profileDescLabel->setStyleSheet("color: #a6adc8; margin-bottom: 10px;");
    m_profileDescLabel->setWordWrap(true);
    detailsLayout->addWidget(m_profileDescLabel);
    
    QLabel* pkgLabel = new QLabel("📦 Packages in this profile (Select to install specific):");
    pkgLabel->setStyleSheet("color: #cdd6f4; font-weight: bold;");
    detailsLayout->addWidget(pkgLabel);
    
    m_packageList = new QListWidget();
    m_packageList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_packageList->setStyleSheet(R"(
        QListWidget {
            background-color: #1e1e2e;
            border: 1px solid #45475a;
            border-radius: 8px;
            color: #cdd6f4;
        }
        QListWidget::item {
            padding: 5px;
        }
        QListWidget::item:hover {
            background-color: #45475a;
        }
        QListWidget::item:selected {
            background-color: #585b70;
            color: #cdd6f4;
        }
    )");
    m_packageList->setToolTip("Double-click a package to view details. Select multiple to install specific packages.");
    connect(m_packageList, &QListWidget::itemDoubleClicked, this, &ProfileView::onPackageDoubleClicked);
    detailsLayout->addWidget(m_packageList);
    
    // Action buttons
    QHBoxLayout* actionLayout = new QHBoxLayout();
    
    m_installBtn = new QPushButton("⬇️ Install Selected / All");
    m_installBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #89b4fa;
            color: #1e1e2e;
            border: none;
            border-radius: 8px;
            padding: 12px 20px;
            font-weight: bold;
            font-size: 14px;
        }
        QPushButton:hover {
            background-color: #b4befe;
        }
    )");
    connect(m_installBtn, &QPushButton::clicked, this, &ProfileView::onInstallClicked);
    
    actionLayout->addWidget(m_installBtn);
    detailsLayout->addLayout(actionLayout);
    
    splitter->addWidget(detailsGroup);
    splitter->setSizes({300, 500});
    
    mainLayout->addWidget(splitter);
    
    // Style groups
    setStyleSheet(R"(
        QGroupBox {
            font-weight: bold;
            color: #89b4fa;
            border: 1px solid #45475a;
            border-radius: 8px;
            margin-top: 10px;
            padding-top: 10px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 5px;
        }
    )");
}

void ProfileView::refreshProfiles() {
    m_profileList->clear();
    
    QList<PackageProfile> profiles = m_profileManager->getAllProfiles();
    QStringList userProfileNames;
    
    // Identify user profile names
    for (const PackageProfile& p : profiles) {
        if (!p.isBuiltIn) {
            userProfileNames.append(p.name);
        }
    }
    
    for (const PackageProfile& profile : profiles) {
        // Skip built-in profiles that are shadowed by user profiles
        if (profile.isBuiltIn && userProfileNames.contains(profile.name)) {
            continue;
        }
        
        QString displayName = profile.name;
        if (profile.isBuiltIn) {
            displayName += " (built-in)";
        }
        
        QListWidgetItem* item = new QListWidgetItem(displayName);
        item->setData(Qt::UserRole, profile.name);
        m_profileList->addItem(item);
    }
    
    m_deleteBtn->setEnabled(false);
}

void ProfileView::onProfileSelected(int index) {
    if (index < 0) return;
    
    QListWidgetItem* item = m_profileList->item(index);
    if (!item) return;
    
    QString profileName = item->data(Qt::UserRole).toString();
    m_selectedProfile = profileName;
    
    PackageProfile profile = m_profileManager->getProfile(profileName);
    
    m_profileNameLabel->setText(profile.name);
    m_profileDescLabel->setText(profile.description);
    
    m_packageList->clear();
    
    for (const QString& pkg : profile.packages) {
        QString status = m_packageManager->packageExists(pkg) ? "✅" : "⬇️";
        QListWidgetItem* item = new QListWidgetItem(QString("%1 %2").arg(status).arg(pkg));
        item->setData(Qt::UserRole, pkg);
        m_packageList->addItem(item);
    }
    
    // Allow editing all profiles (built-ins will be shadowed)
    m_editBtn->setEnabled(true);
    
    // Enable delete only for user profiles
    // If it's a shadowed built-in profile, delete will "reset" it (remove the shadow)
    bool isShadowed = false;
    if (!profile.isBuiltIn) {
        // Check if there is a built-in profile with the same name
        for (const PackageProfile& p : m_profileManager->getAllProfiles()) {
            if (p.isBuiltIn && p.name == profile.name) {
                isShadowed = true;
                break;
            }
        }
    }
    
    // Always enable delete/reset
    m_deleteBtn->setEnabled(true);
    
    if (isShadowed) {
        m_deleteBtn->setText("🔄 Reset");
        m_deleteBtn->setToolTip("Reset to built-in defaults");
    } else {
        m_deleteBtn->setText("🗑️ Delete");
        m_deleteBtn->setToolTip(profile.isBuiltIn ? "Remove this built-in profile" : "Delete this profile");
    }
}

void ProfileView::onInstallClicked() {
    if (m_selectedProfile.isEmpty()) return;
    
    PackageProfile profile = m_profileManager->getProfile(m_selectedProfile);
    if (profile.packages.isEmpty()) return;
    
    QStringList candidates;
    bool usingSelection = false;
    
    QList<QListWidgetItem*> selectedItems = m_packageList->selectedItems();
    if (!selectedItems.isEmpty()) {
        usingSelection = true;
        for (QListWidgetItem* item : selectedItems) {
            candidates.append(item->data(Qt::UserRole).toString());
        }
    } else {
        candidates = profile.packages;
    }
    
    // Filter out already installed packages
    QStringList toInstall;
    for (const QString& pkg : candidates) {
        if (!m_packageManager->packageExists(pkg)) {
            toInstall.append(pkg);
        }
    }
    
    if (toInstall.isEmpty()) {
        QString msg = usingSelection 
            ? "All selected packages are already installed!"
            : "All packages in this profile are already installed!";
        QMessageBox::information(this, "Nothing to Install", msg);
        return;
    }
    
    QMessageBox::StandardButton reply = QMessageBox::question(this, "Install Packages?",
        QString("Install %1 packages from profile '%2'?\n\n%3")
            .arg(toInstall.size())
            .arg(profile.name)
            .arg(toInstall.join(", ")),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply != QMessageBox::Yes) return;
    
    QString command = QString("pacman -S %1 --noconfirm --needed").arg(toInstall.join(" "));
    
    bool success = PrivilegedRunner::runCommand(command,
        QString("Installing profile: %1").arg(profile.name), this);
    
    if (success) {
        QMessageBox::information(this, "Success",
            QString("Profile '%1' installed successfully!").arg(profile.name));
        onProfileSelected(m_profileList->currentRow()); // Refresh status
    }
}



void ProfileView::onCreateClicked() {
    bool ok;
    QString name = QInputDialog::getText(this, "Create Profile",
        "Enter a name for the new profile:", QLineEdit::Normal, "My Development Setup", &ok);
    
    if (!ok || name.isEmpty()) return;
    
    QString desc = QInputDialog::getText(this, "Profile Description",
        "Enter a description (optional):", QLineEdit::Normal, "", &ok);
    
    PackageProfile profile = m_profileManager->createFromInstalled();
    profile.name = name;
    profile.description = desc.isEmpty() ? "Custom profile" : desc;
    
    if (m_profileManager->saveProfile(profile)) {
        QMessageBox::information(this, "Profile Created",
            QString("Profile '%1' created with %2 packages.")
                .arg(name).arg(profile.packages.size()));
        refreshProfiles();
    } else {
        QMessageBox::warning(this, "Error", "Failed to save profile.");
    }
}


void ProfileView::onDeleteClicked() {
    if (m_selectedProfile.isEmpty()) return;
    
    PackageProfile profile = m_profileManager->getProfile(m_selectedProfile);
    
    // Check if this is a shadow of a built-in profile
    bool isShadowed = false;
    if (!profile.isBuiltIn) {
        for (const PackageProfile& p : m_profileManager->getAllProfiles()) {
            if (p.isBuiltIn && p.name == profile.name) {
                isShadowed = true;
                break;
            }
        }
    }
    
    QString title, text;
    if (isShadowed) {
        title = "Reset Profile?";
        text = QString("Reset '%1' to its built-in defaults?\n\nYour custom changes will be lost.").arg(profile.name);
    } else if (profile.isBuiltIn) {
        title = "Delete Built-in Profile?";
        text = QString("This will hide the built-in profile '%1'.\n\nContinue?").arg(profile.name);
    } else {
        title = "Delete Profile?";
        text = QString("Delete profile '%1'?\n\nThis cannot be undone.").arg(profile.name);
    }
    
    QMessageBox::StandardButton reply = QMessageBox::question(this, title, text,
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        m_profileManager->deleteProfile(m_selectedProfile);
        m_selectedProfile.clear(); // Clear execution state
        refreshProfiles(); // This will re-appear as the built-in version if it was shadowed, or disappear if deleted
    }
}



void ProfileView::onCreateCustomClicked() {
    bool ok;
    QString name = QInputDialog::getText(this, "Create Custom Profile",
        "Enter a name for the new profile:", QLineEdit::Normal, "My Custom Profile", &ok);
    
    if (!ok || name.isEmpty()) return;
    
    QString desc = QInputDialog::getText(this, "Profile Description",
        "Enter a description (optional):", QLineEdit::Normal, "", &ok);
    
    // Create a package picker dialog
    QDialog picker(this);
    picker.setWindowTitle("Select Packages");
    picker.setMinimumSize(600, 500);
    
    QVBoxLayout* layout = new QVBoxLayout(&picker);
    
    QLabel* infoLabel = new QLabel("Select packages to include in the profile.\nOnly explicitly installed packages are shown.");
    infoLabel->setStyleSheet("color: #a6adc8; margin-bottom: 10px;");
    layout->addWidget(infoLabel);
    
    // Search box
    QLineEdit* searchBox = new QLineEdit();
    searchBox->setPlaceholderText("🔍 Filter packages...");
    searchBox->setStyleSheet("padding: 8px; border-radius: 6px; background-color: #313244; color: #cdd6f4;");
    layout->addWidget(searchBox);
    
    // Package list with checkboxes
    QListWidget* pkgList = new QListWidget();
    pkgList->setStyleSheet(R"(
        QListWidget {
            background-color: #1e1e2e;
            border: 1px solid #45475a;
            border-radius: 8px;
            color: #cdd6f4;
        }
        QListWidget::item {
            padding: 5px;
        }
    )");
    
    // Get explicit packages
    QList<Package> allPkgs = m_packageManager->getExplicitPackages();
    for (const Package& pkg : allPkgs) {
        QListWidgetItem* item = new QListWidgetItem(pkg.name);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
        item->setData(Qt::UserRole, pkg.version);
        pkgList->addItem(item);
    }
    
    layout->addWidget(pkgList);
    
    // Filter functionality
    connect(searchBox, &QLineEdit::textChanged, [pkgList](const QString& text) {
        for (int i = 0; i < pkgList->count(); ++i) {
            QListWidgetItem* item = pkgList->item(i);
            item->setHidden(!item->text().contains(text, Qt::CaseInsensitive));
        }
    });
    
    // Select/Deselect all buttons
    QHBoxLayout* selectBtns = new QHBoxLayout();
    QPushButton* selectAllBtn = new QPushButton("Select All");
    QPushButton* selectNoneBtn = new QPushButton("Select None");
    connect(selectAllBtn, &QPushButton::clicked, [pkgList]() {
        for (int i = 0; i < pkgList->count(); ++i) {
            if (!pkgList->item(i)->isHidden())
                pkgList->item(i)->setCheckState(Qt::Checked);
        }
    });
    connect(selectNoneBtn, &QPushButton::clicked, [pkgList]() {
        for (int i = 0; i < pkgList->count(); ++i) {
            pkgList->item(i)->setCheckState(Qt::Unchecked);
        }
    });
    selectBtns->addWidget(selectAllBtn);
    selectBtns->addWidget(selectNoneBtn);
    selectBtns->addStretch();
    layout->addLayout(selectBtns);
    
    // Dialog buttons
    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, &picker, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &picker, &QDialog::reject);
    layout->addWidget(buttons);
    
    if (picker.exec() != QDialog::Accepted) return;
    
    // Collect selected packages
    QStringList selectedPkgs;
    for (int i = 0; i < pkgList->count(); ++i) {
        if (pkgList->item(i)->checkState() == Qt::Checked) {
            selectedPkgs.append(pkgList->item(i)->text());
        }
    }
    
    if (selectedPkgs.isEmpty()) {
        QMessageBox::warning(this, "No Packages", "Please select at least one package.");
        return;
    }
    
    PackageProfile profile;
    profile.name = name;
    profile.description = desc.isEmpty() ? "Custom profile" : desc;
    profile.packages = selectedPkgs;
    profile.isBuiltIn = false;
    
    if (m_profileManager->saveProfile(profile)) {
        QMessageBox::information(this, "Profile Created",
            QString("Profile '%1' created with %2 packages.").arg(name).arg(selectedPkgs.size()));
        refreshProfiles();
    } else {
        QMessageBox::warning(this, "Error", "Failed to save profile.");
    }
}

void ProfileView::onEditClicked() {
    if (m_selectedProfile.isEmpty()) return;
    
    PackageProfile profile = m_profileManager->getProfile(m_selectedProfile);
    
    // Allow editing any profile (built-ins will be saved as user profiles)
    
    // Edit dialog with package list
    QDialog editor(this);
    editor.setWindowTitle("Edit Profile: " + profile.name);
    editor.setMinimumSize(500, 400);
    
    QVBoxLayout* layout = new QVBoxLayout(&editor);
    
    // Name and description
    QFormLayout* formLayout = new QFormLayout();
    QLineEdit* nameEdit = new QLineEdit(profile.name);
    QLineEdit* descEdit = new QLineEdit(profile.description);
    formLayout->addRow("Name:", nameEdit);
    formLayout->addRow("Description:", descEdit);
    layout->addLayout(formLayout);
    
    // Package list with remove buttons
    QLabel* pkgLabel = new QLabel("Packages (right-click to remove):");
    layout->addWidget(pkgLabel);
    
    QListWidget* pkgList = new QListWidget();
    pkgList->setContextMenuPolicy(Qt::CustomContextMenu);
    for (const QString& pkg : profile.packages) {
        pkgList->addItem(pkg);
    }
    
    connect(pkgList, &QListWidget::customContextMenuRequested, [pkgList, this](const QPoint& pos) {
        QListWidgetItem* item = pkgList->itemAt(pos);
        if (!item) return;
        
        QMenu menu;
        menu.addAction("🗑️ Remove from profile", [pkgList, item]() {
            delete pkgList->takeItem(pkgList->row(item));
        });
        menu.addAction("📄 View details", [this, item]() {
            showPackageDetails(item->text());
        });
        menu.exec(pkgList->mapToGlobal(pos));
    });
    
    layout->addWidget(pkgList);
    
    // Add package button
    QPushButton* addPkgBtn = new QPushButton("➕ Add Package");
    connect(addPkgBtn, &QPushButton::clicked, [this, pkgList]() {
        bool ok;
        QString pkg = QInputDialog::getText(this, "Add Package",
            "Enter package name:", QLineEdit::Normal, "", &ok);
        if (ok && !pkg.isEmpty()) {
            pkgList->addItem(pkg);
        }
    });
    layout->addWidget(addPkgBtn);
    
    // Dialog buttons
    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, &editor, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &editor, &QDialog::reject);
    layout->addWidget(buttons);
    
    if (editor.exec() != QDialog::Accepted) return;
    
    // Collect packages
    QStringList newPkgs;
    for (int i = 0; i < pkgList->count(); ++i) {
        newPkgs.append(pkgList->item(i)->text());
    }
    
    profile.name = nameEdit->text();
    profile.description = descEdit->text();
    profile.packages = newPkgs;
    profile.isBuiltIn = false; // Always save as user profile (creates copy if was built-in)
    
    if (m_profileManager->saveProfile(profile)) {
        QMessageBox::information(this, "Saved", "Profile updated successfully.");
        m_selectedProfile = profile.name;
        refreshProfiles();
    } else {
        QMessageBox::warning(this, "Error", "Failed to save profile.");
    }
}

void ProfileView::onPackageDoubleClicked(QListWidgetItem* item) {
    if (!item) return;
    
    QString pkgName = item->data(Qt::UserRole).toString();
    if (pkgName.isEmpty()) {
        // Fallback or just return
        return; 
    }
    
    showPackageDetails(pkgName);
}

void ProfileView::showPackageDetails(const QString& pkgName) {
    QProcess proc;
    proc.start("pacman", {"-Si", pkgName});
    proc.waitForFinished(5000);
    QString info = QString::fromUtf8(proc.readAllStandardOutput());
    
    if (info.isEmpty()) {
        // Try -Qi for installed packages
        proc.start("pacman", {"-Qi", pkgName});
        proc.waitForFinished(5000);
        info = QString::fromUtf8(proc.readAllStandardOutput());
    }
    
    if (info.isEmpty()) {
        QMessageBox::information(this, pkgName, "Package not found in repositories or installed.");
        return;
    }
    
    // Show in dialog
    QDialog dialog(this);
    dialog.setWindowTitle("Package: " + pkgName);
    dialog.setMinimumSize(500, 400);
    
    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    
    QTextEdit* textEdit = new QTextEdit();
    textEdit->setReadOnly(true);
    textEdit->setPlainText(info);
    textEdit->setStyleSheet("font-family: monospace; background-color: #1e1e2e; color: #cdd6f4;");
    layout->addWidget(textEdit);
    
    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);
    
    dialog.exec();
}

void ProfileView::onImportClicked() {
    QString filename = QFileDialog::getOpenFileName(this, "Import Profile",
        QDir::homePath(), "JSON Files (*.json)");
    
    if (filename.isEmpty()) return;
    
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Error", "Failed to open file.");
        return;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    
    if (doc.isNull()) {
        QMessageBox::warning(this, "Error", "Invalid JSON file.");
        return;
    }
    
    PackageProfile profile;
    
    if (doc.isObject()) {
        // Single profile
        profile = PackageProfile::fromJson(doc.object());
    } else if (doc.isArray() && !doc.array().isEmpty()) {
        // Array of profiles - import first one
        profile = PackageProfile::fromJson(doc.array().first().toObject());
    } else {
        QMessageBox::warning(this, "Error", "No valid profile found in file.");
        return;
    }
    
    if (profile.name.isEmpty() || profile.packages.isEmpty()) {
        QMessageBox::warning(this, "Error", "Profile has no name or packages.");
        return;
    }
    
    profile.isBuiltIn = false; // Imported profiles are user profiles
    
    if (m_profileManager->saveProfile(profile)) {
        QMessageBox::information(this, "Imported",
            QString("Profile '%1' imported with %2 packages.")
                .arg(profile.name).arg(profile.packages.size()));
        refreshProfiles();
    } else {
        QMessageBox::warning(this, "Error", "Failed to save imported profile.");
    }
}

void ProfileView::onExportClicked() {
    if (m_selectedProfile.isEmpty()) return;
    
    PackageProfile profile = m_profileManager->getProfile(m_selectedProfile);
    
    QString filename = QFileDialog::getSaveFileName(this, "Export Profile",
        QDir::homePath() + "/" + profile.name.simplified().replace(" ", "_") + ".json",
        "JSON Files (*.json)");
    
    if (filename.isEmpty()) return;
    
    QFile file(filename);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(profile.toJson()).toJson(QJsonDocument::Indented));
        file.close();
        QMessageBox::information(this, "Exported",
            QString("Profile exported to:\n%1").arg(filename));
    } else {
        QMessageBox::warning(this, "Error", "Failed to export profile.");
    }
}

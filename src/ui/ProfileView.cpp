#include "ProfileView.h"
#include "core/ProfileManager.h"
#include "core/PackageManager.h"
#include "PrivilegedRunner.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QSplitter>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>

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
    
    // Row 2: Edit/Import/Delete
    QHBoxLayout* profileBtnLayout2 = new QHBoxLayout();
    profileBtnLayout2->addWidget(m_importBtn);
    profileBtnLayout2->addWidget(m_editBtn);
    profileBtnLayout2->addWidget(m_deleteBtn);
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
    
    QLabel* pkgLabel = new QLabel("📦 Packages in this profile:");
    pkgLabel->setStyleSheet("color: #cdd6f4; font-weight: bold;");
    detailsLayout->addWidget(pkgLabel);
    
    m_packageList = new QListWidget();
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
    )");
    m_packageList->setToolTip("Double-click a package to view details");
    connect(m_packageList, &QListWidget::itemDoubleClicked, this, &ProfileView::onPackageDoubleClicked);
    detailsLayout->addWidget(m_packageList);
    
    // Action buttons
    QHBoxLayout* actionLayout = new QHBoxLayout();
    
    m_installBtn = new QPushButton("⬇️ Install All Packages");
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
    
    m_exportBtn = new QPushButton("💾 Export to File");
    m_exportBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #f9e2af;
            color: #1e1e2e;
            border: none;
            border-radius: 8px;
            padding: 12px 20px;
            font-weight: bold;
            font-size: 14px;
        }
        QPushButton:hover {
            background-color: #f5c2e7;
        }
    )");
    connect(m_exportBtn, &QPushButton::clicked, this, &ProfileView::onExportClicked);
    
    actionLayout->addWidget(m_installBtn);
    actionLayout->addWidget(m_exportBtn);
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
    for (const PackageProfile& profile : profiles) {
        QString displayName = profile.name;
        if (profile.isBuiltIn) {
            displayName += " (built-in)";
        }
        m_profileList->addItem(displayName);
    }
    
    m_deleteBtn->setEnabled(false);
}

void ProfileView::onProfileSelected(int index) {
    if (index < 0) return;
    
    QList<PackageProfile> profiles = m_profileManager->getAllProfiles();
    if (index >= profiles.size()) return;
    
    const PackageProfile& profile = profiles[index];
    m_selectedProfile = profile.name;
    
    m_profileNameLabel->setText(profile.name);
    m_profileDescLabel->setText(profile.description);
    
    m_packageList->clear();
    
    for (const QString& pkg : profile.packages) {
        QString status = m_packageManager->packageExists(pkg) ? "✅" : "⬇️";
        m_packageList->addItem(QString("%1 %2").arg(status).arg(pkg));
    }
    
    // Only allow deleting user profiles
    m_deleteBtn->setEnabled(!profile.isBuiltIn);
}

void ProfileView::onInstallClicked() {
    if (m_selectedProfile.isEmpty()) return;
    
    PackageProfile profile = m_profileManager->getProfile(m_selectedProfile);
    if (profile.packages.isEmpty()) return;
    
    // Filter out already installed packages
    QStringList toInstall;
    for (const QString& pkg : profile.packages) {
        if (!m_packageManager->packageExists(pkg)) {
            toInstall.append(pkg);
        }
    }
    
    if (toInstall.isEmpty()) {
        QMessageBox::information(this, "All Installed",
            "All packages in this profile are already installed!");
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
    if (profile.isBuiltIn) {
        QMessageBox::warning(this, "Cannot Delete", "Built-in profiles cannot be deleted.");
        return;
    }
    
    QMessageBox::StandardButton reply = QMessageBox::question(this, "Delete Profile?",
        QString("Delete profile '%1'?\n\nThis cannot be undone.").arg(m_selectedProfile),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        m_profileManager->deleteProfile(m_selectedProfile);
        m_selectedProfile.clear();
        refreshProfiles();
    }
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

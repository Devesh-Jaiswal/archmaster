#include "ProfileManager.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QProcess>

QJsonObject PackageProfile::toJson() const {
    QJsonObject obj;
    obj["name"] = name;
    obj["description"] = description;
    obj["packages"] = QJsonArray::fromStringList(packages);
    return obj;
}

PackageProfile PackageProfile::fromJson(const QJsonObject& obj) {
    PackageProfile profile;
    profile.name = obj["name"].toString();
    profile.description = obj["description"].toString();
    
    QJsonArray pkgArray = obj["packages"].toArray();
    for (const QJsonValue& val : pkgArray) {
        profile.packages.append(val.toString());
    }
    
    return profile;
}

ProfileManager::ProfileManager() {
    loadDeletedBuiltIns(); // Load hidden built-ins first
    initBuiltInProfiles();
    loadProfiles();
}

void ProfileManager::initBuiltInProfiles() {
    // Base Development
    PackageProfile baseDev;
    baseDev.name = "🔧 Base Development";
    baseDev.description = "Essential development tools";
    baseDev.packages = {"base-devel", "git", "make", "cmake", "gcc", "gdb", "valgrind"};
    baseDev.isBuiltIn = true;
    m_builtInProfiles.append(baseDev);
    
    // Python Development
    PackageProfile pythonDev;
    pythonDev.name = "🐍 Python Development";
    pythonDev.description = "Python development environment";
    pythonDev.packages = {"python", "python-pip", "python-virtualenv", "ipython", "python-pytest", "python-black", "python-pylint"};
    pythonDev.isBuiltIn = true;
    m_builtInProfiles.append(pythonDev);
    
    // Web Development
    PackageProfile webDev;
    webDev.name = "🌐 Web Development";
    webDev.description = "Web development stack";
    webDev.packages = {"nodejs", "npm", "yarn", "typescript", "deno"};
    webDev.isBuiltIn = true;
    m_builtInProfiles.append(webDev);
    
    // Rust Development
    PackageProfile rustDev;
    rustDev.name = "🦀 Rust Development";
    rustDev.description = "Rust development environment";
    rustDev.packages = {"rust", "cargo", "rustfmt", "rust-analyzer"};
    rustDev.isBuiltIn = true;
    m_builtInProfiles.append(rustDev);
    
    // C/C++ Development
    PackageProfile cppDev;
    cppDev.name = "⚙️ C/C++ Development";
    cppDev.description = "C/C++ development tools";
    cppDev.packages = {"gcc", "clang", "cmake", "ninja", "gdb", "lldb", "clang-tools-extra"};
    cppDev.isBuiltIn = true;
    m_builtInProfiles.append(cppDev);
    
    // Container/DevOps
    PackageProfile devops;
    devops.name = "🐳 Container & DevOps";
    devops.description = "Container and DevOps tools";
    devops.packages = {"docker", "docker-compose", "kubectl", "helm", "terraform"};
    devops.isBuiltIn = true;
    m_builtInProfiles.append(devops);
    
    // Database Tools
    PackageProfile dbTools;
    dbTools.name = "🗄️ Database Tools";
    dbTools.description = "Database management tools";
    dbTools.packages = {"postgresql", "mariadb", "sqlite", "redis", "mongodb"};
    dbTools.isBuiltIn = true;
    m_builtInProfiles.append(dbTools);
}

QString ProfileManager::profilesPath() const {
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return dataPath + "/profiles.json";
}

void ProfileManager::loadProfiles() {
    m_userProfiles.clear();
    
    QFile file(profilesPath());
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    
    if (!doc.isArray()) return;
    
    QJsonArray array = doc.array();
    for (const QJsonValue& val : array) {
        if (val.isObject()) {
            PackageProfile profile = PackageProfile::fromJson(val.toObject());
            m_userProfiles.append(profile);
        }
    }
}

bool ProfileManager::saveProfile(const PackageProfile& profile) {
    // Check if profile with this name exists
    for (int i = 0; i < m_userProfiles.size(); ++i) {
        if (m_userProfiles[i].name == profile.name) {
            m_userProfiles[i] = profile;
            break;
        }
    }
    
    // Add new profile if not found
    bool found = false;
    for (const PackageProfile& p : m_userProfiles) {
        if (p.name == profile.name) {
            found = true;
            break;
        }
    }
    if (!found) {
        m_userProfiles.append(profile);
    }
    
    // Save to file
    QJsonArray array;
    for (const PackageProfile& p : m_userProfiles) {
        array.append(p.toJson());
    }
    
    QString path = profilesPath();
    QDir().mkpath(QFileInfo(path).absolutePath());
    
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    
    file.write(QJsonDocument(array).toJson());
    file.close();
    return true;
}

bool ProfileManager::deleteProfile(const QString& name) {
    bool removedUser = false;
    for (int i = 0; i < m_userProfiles.size(); ++i) {
        if (m_userProfiles[i].name == name) {
            m_userProfiles.removeAt(i);
            removedUser = true;
            break;
        }
    }
    
    // Save updated user profiles if changed
    if (removedUser) {
        QJsonArray array;
        for (const PackageProfile& p : m_userProfiles) {
            array.append(p.toJson());
        }
        
        QFile file(profilesPath());
        if (file.open(QIODevice::WriteOnly)) {
            file.write(QJsonDocument(array).toJson());
            file.close();
        }
    }
    
    // Check if it's a built-in profile (even if we just removed the shadow)
    bool isBuiltIn = false;
    for (const PackageProfile& p : m_builtInProfiles) {
        if (p.name == name) {
            isBuiltIn = true;
            break;
        }
    }
    
    // Only tombstone if we didn't just remove a user profile (which would be a reset)
    // OR if we want to support "Delete completely" in one go, but for now let's make it two steps:
    // 1. Delete (Reset) -> reveals built-in
    // 2. Delete (Hide) -> hides built-in
    if (isBuiltIn && !removedUser) {
        if (!m_removedBuiltInProfiles.contains(name)) {
            m_removedBuiltInProfiles.append(name);
            saveDeletedBuiltIns();
            return true;
        }
    }
    
    return removedUser;
}

QList<PackageProfile> ProfileManager::getAllProfiles() const {
    QList<PackageProfile> all;
    
    // Add built-ins that aren't deleted
    for (const PackageProfile& p : m_builtInProfiles) {
        if (!m_removedBuiltInProfiles.contains(p.name)) {
            all.append(p);
        }
    }
    
    all.append(m_userProfiles);
    return all;
}

QList<PackageProfile> ProfileManager::getBuiltInProfiles() const {
    return m_builtInProfiles;
}

QList<PackageProfile> ProfileManager::getUserProfiles() const {
    return m_userProfiles;
}

PackageProfile ProfileManager::getProfile(const QString& name) const {
    // Check user profiles first to allow overriding built-ins
    for (const PackageProfile& p : m_userProfiles) {
        if (p.name == name) return p;
    }
    for (const PackageProfile& p : m_builtInProfiles) {
        if (p.name == name) return p;
    }
    return PackageProfile();
}

PackageProfile ProfileManager::createFromInstalled() const {
    PackageProfile profile;
    profile.name = "My Packages";
    profile.description = "Exported from current system";
    
    // Get explicitly installed packages
    QProcess proc;
    proc.start("pacman", {"-Qeq"});
    proc.waitForFinished(30000);
    
    QString output = QString::fromUtf8(proc.readAllStandardOutput());
    profile.packages = output.split('\n', Qt::SkipEmptyParts);
    
    return profile;
}


QString ProfileManager::builtInProfilesPath() const {
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return dataPath + "/deleted_builtins.json";
}

void ProfileManager::loadDeletedBuiltIns() {
    m_removedBuiltInProfiles.clear();
    
    QFile file(builtInProfilesPath());
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    
    if (!doc.isArray()) return;
    
    QJsonArray array = doc.array();
    for (const QJsonValue& val : array) {
        m_removedBuiltInProfiles.append(val.toString());
    }
}

void ProfileManager::saveDeletedBuiltIns() {
    QString path = builtInProfilesPath();
    QDir().mkpath(QFileInfo(path).absolutePath());
    
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return;
    }
    
    QJsonArray array;
    for (const QString& name : m_removedBuiltInProfiles) {
        array.append(name);
    }
    
    file.write(QJsonDocument(array).toJson());
    file.close();
}

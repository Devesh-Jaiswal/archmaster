#include "PackageManager.h"
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <algorithm>

PackageManager::PackageManager(QObject* parent)
    : QObject(parent)
{
}

PackageManager::~PackageManager() {
    if (m_handle) {
        alpm_release(m_handle);
        m_handle = nullptr;
    }
}

bool PackageManager::initialize() {
    if (m_initialized) return true;
    
    alpm_errno_t err;
    m_handle = alpm_initialize(m_rootDir.toUtf8().constData(), 
                                m_dbPath.toUtf8().constData(), 
                                &err);
    
    if (!m_handle) {
        setError(QString("Failed to initialize alpm: %1").arg(alpm_strerror(err)));
        return false;
    }
    
    m_localDb = alpm_get_localdb(m_handle);
    if (!m_localDb) {
        setError("Failed to get local database");
        alpm_release(m_handle);
        m_handle = nullptr;
        return false;
    }
    
    m_initialized = true;
    qDebug() << "PackageManager initialized successfully";
    return true;
}

void PackageManager::refresh() {
    // Re-initialize to get fresh data from the database
    if (m_handle) {
        alpm_release(m_handle);
        m_handle = nullptr;
        m_localDb = nullptr;
        m_initialized = false;
    }
    
    initialize();
    // Note: Don't emit packagesChanged here - caller handles UI updates
}

Package PackageManager::alpmPackageToPackage(alpm_pkg_t* pkg) {
    Package p;
    
    p.name = QString::fromUtf8(alpm_pkg_get_name(pkg));
    p.version = QString::fromUtf8(alpm_pkg_get_version(pkg));
    p.description = QString::fromUtf8(alpm_pkg_get_desc(pkg));
    p.url = QString::fromUtf8(alpm_pkg_get_url(pkg));
    p.packager = QString::fromUtf8(alpm_pkg_get_packager(pkg));
    p.architecture = QString::fromUtf8(alpm_pkg_get_arch(pkg));
    
    p.installedSize = alpm_pkg_get_isize(pkg);
    p.downloadSize = alpm_pkg_get_size(pkg);
    
    p.installDate = QDateTime::fromSecsSinceEpoch(alpm_pkg_get_installdate(pkg));
    p.buildDate = QDateTime::fromSecsSinceEpoch(alpm_pkg_get_builddate(pkg));
    
    alpm_pkgreason_t reason = alpm_pkg_get_reason(pkg);
    p.installReason = (reason == ALPM_PKG_REASON_EXPLICIT) ? "explicit" : "dependency";
    
    // Groups
    alpm_list_t* groups = alpm_pkg_get_groups(pkg);
    for (alpm_list_t* i = groups; i; i = alpm_list_next(i)) {
        p.groups << QString::fromUtf8(static_cast<const char*>(i->data));
    }
    
    // Licenses
    alpm_list_t* licenses = alpm_pkg_get_licenses(pkg);
    for (alpm_list_t* i = licenses; i; i = alpm_list_next(i)) {
        p.licenses << QString::fromUtf8(static_cast<const char*>(i->data));
    }
    
    // Dependencies
    alpm_list_t* depends = alpm_pkg_get_depends(pkg);
    for (alpm_list_t* i = depends; i; i = alpm_list_next(i)) {
        alpm_depend_t* dep = static_cast<alpm_depend_t*>(i->data);
        p.depends << QString::fromUtf8(dep->name);
    }
    
    // Optional dependencies
    alpm_list_t* optdeps = alpm_pkg_get_optdepends(pkg);
    for (alpm_list_t* i = optdeps; i; i = alpm_list_next(i)) {
        alpm_depend_t* dep = static_cast<alpm_depend_t*>(i->data);
        QString optdep = QString::fromUtf8(dep->name);
        if (dep->desc) {
            optdep += QString(": %1").arg(QString::fromUtf8(dep->desc));
        }
        p.optDepends << optdep;
    }
    
    // Required by (reverse dependencies)
    alpm_list_t* requiredby = alpm_pkg_compute_requiredby(pkg);
    for (alpm_list_t* i = requiredby; i; i = alpm_list_next(i)) {
        p.requiredBy << QString::fromUtf8(static_cast<const char*>(i->data));
    }
    FREELIST(requiredby);
    
    // Optional for
    alpm_list_t* optionalfor = alpm_pkg_compute_optionalfor(pkg);
    for (alpm_list_t* i = optionalfor; i; i = alpm_list_next(i)) {
        p.optionalFor << QString::fromUtf8(static_cast<const char*>(i->data));
    }
    FREELIST(optionalfor);
    
    // Provides
    alpm_list_t* provides = alpm_pkg_get_provides(pkg);
    for (alpm_list_t* i = provides; i; i = alpm_list_next(i)) {
        alpm_depend_t* prov = static_cast<alpm_depend_t*>(i->data);
        p.provides << QString::fromUtf8(prov->name);
    }
    
    // Conflicts
    alpm_list_t* conflicts = alpm_pkg_get_conflicts(pkg);
    for (alpm_list_t* i = conflicts; i; i = alpm_list_next(i)) {
        alpm_depend_t* conf = static_cast<alpm_depend_t*>(i->data);
        p.conflicts << QString::fromUtf8(conf->name);
    }
    
    // Replaces
    alpm_list_t* replaces = alpm_pkg_get_replaces(pkg);
    for (alpm_list_t* i = replaces; i; i = alpm_list_next(i)) {
        alpm_depend_t* rep = static_cast<alpm_depend_t*>(i->data);
        p.replaces << QString::fromUtf8(rep->name);
    }
    
    return p;
}

QList<Package> PackageManager::getAllPackages() {
    QList<Package> packages;
    if (!m_initialized) return packages;
    
    alpm_list_t* pkgcache = alpm_db_get_pkgcache(m_localDb);
    for (alpm_list_t* i = pkgcache; i; i = alpm_list_next(i)) {
        alpm_pkg_t* pkg = static_cast<alpm_pkg_t*>(i->data);
        packages.append(alpmPackageToPackage(pkg));
    }
    
    return packages;
}

QList<Package> PackageManager::getExplicitPackages() {
    QList<Package> packages;
    if (!m_initialized) return packages;
    
    alpm_list_t* pkgcache = alpm_db_get_pkgcache(m_localDb);
    for (alpm_list_t* i = pkgcache; i; i = alpm_list_next(i)) {
        alpm_pkg_t* pkg = static_cast<alpm_pkg_t*>(i->data);
        if (alpm_pkg_get_reason(pkg) == ALPM_PKG_REASON_EXPLICIT) {
            packages.append(alpmPackageToPackage(pkg));
        }
    }
    
    return packages;
}

QList<Package> PackageManager::getDependencyPackages() {
    QList<Package> packages;
    if (!m_initialized) return packages;
    
    alpm_list_t* pkgcache = alpm_db_get_pkgcache(m_localDb);
    for (alpm_list_t* i = pkgcache; i; i = alpm_list_next(i)) {
        alpm_pkg_t* pkg = static_cast<alpm_pkg_t*>(i->data);
        if (alpm_pkg_get_reason(pkg) == ALPM_PKG_REASON_DEPEND) {
            packages.append(alpmPackageToPackage(pkg));
        }
    }
    
    return packages;
}

QList<Package> PackageManager::getOrphanPackages() {
    QList<Package> packages;
    if (!m_initialized) return packages;
    
    alpm_list_t* pkgcache = alpm_db_get_pkgcache(m_localDb);
    for (alpm_list_t* i = pkgcache; i; i = alpm_list_next(i)) {
        alpm_pkg_t* pkg = static_cast<alpm_pkg_t*>(i->data);
        
        // Orphan = installed as dependency but nothing requires it
        if (alpm_pkg_get_reason(pkg) == ALPM_PKG_REASON_DEPEND) {
            alpm_list_t* requiredby = alpm_pkg_compute_requiredby(pkg);
            alpm_list_t* optionalfor = alpm_pkg_compute_optionalfor(pkg);
            
            if (!requiredby && !optionalfor) {
                packages.append(alpmPackageToPackage(pkg));
            }
            
            FREELIST(requiredby);
            FREELIST(optionalfor);
        }
    }
    
    return packages;
}

QList<Package> PackageManager::searchPackages(const QString& query) {
    QList<Package> packages;
    if (!m_initialized || query.isEmpty()) return packages;
    
    QString lowerQuery = query.toLower();
    
    alpm_list_t* pkgcache = alpm_db_get_pkgcache(m_localDb);
    for (alpm_list_t* i = pkgcache; i; i = alpm_list_next(i)) {
        alpm_pkg_t* pkg = static_cast<alpm_pkg_t*>(i->data);
        
        QString name = QString::fromUtf8(alpm_pkg_get_name(pkg)).toLower();
        QString desc = QString::fromUtf8(alpm_pkg_get_desc(pkg)).toLower();
        
        if (name.contains(lowerQuery) || desc.contains(lowerQuery)) {
            packages.append(alpmPackageToPackage(pkg));
        }
    }
    
    return packages;
}

Package PackageManager::getPackageInfo(const QString& name) {
    Package p;
    if (!m_initialized) return p;
    
    alpm_pkg_t* pkg = alpm_db_get_pkg(m_localDb, name.toUtf8().constData());
    if (pkg) {
        p = alpmPackageToPackage(pkg);
    }
    
    return p;
}

bool PackageManager::packageExists(const QString& name) {
    if (!m_initialized) return false;
    return alpm_db_get_pkg(m_localDb, name.toUtf8().constData()) != nullptr;
}

QStringList PackageManager::getDependencies(const QString& packageName) {
    Package pkg = getPackageInfo(packageName);
    return pkg.depends;
}

QStringList PackageManager::getReverseDependencies(const QString& packageName) {
    Package pkg = getPackageInfo(packageName);
    return pkg.requiredBy;
}

QMap<QString, QStringList> PackageManager::getDependencyTree(const QString& packageName, int depth) {
    QMap<QString, QStringList> tree;
    if (!m_initialized || depth <= 0) return tree;
    
    QStringList deps = getDependencies(packageName);
    tree[packageName] = deps;
    
    if (depth > 1) {
        for (const QString& dep : deps) {
            if (!tree.contains(dep)) {
                QMap<QString, QStringList> subtree = getDependencyTree(dep, depth - 1);
                for (auto it = subtree.begin(); it != subtree.end(); ++it) {
                    if (!tree.contains(it.key())) {
                        tree[it.key()] = it.value();
                    }
                }
            }
        }
    }
    
    return tree;
}

int PackageManager::totalPackageCount() {
    if (!m_initialized) return 0;
    return alpm_list_count(alpm_db_get_pkgcache(m_localDb));
}

int PackageManager::explicitPackageCount() {
    return getExplicitPackages().size();
}

int PackageManager::dependencyPackageCount() {
    return getDependencyPackages().size();
}

int PackageManager::orphanPackageCount() {
    return getOrphanPackages().size();
}

qint64 PackageManager::totalInstalledSize() {
    qint64 total = 0;
    if (!m_initialized) return total;
    
    alpm_list_t* pkgcache = alpm_db_get_pkgcache(m_localDb);
    for (alpm_list_t* i = pkgcache; i; i = alpm_list_next(i)) {
        alpm_pkg_t* pkg = static_cast<alpm_pkg_t*>(i->data);
        total += alpm_pkg_get_isize(pkg);
    }
    
    return total;
}

QMap<QString, qint64> PackageManager::getSizeByPackage() {
    QMap<QString, qint64> sizes;
    if (!m_initialized) return sizes;
    
    alpm_list_t* pkgcache = alpm_db_get_pkgcache(m_localDb);
    for (alpm_list_t* i = pkgcache; i; i = alpm_list_next(i)) {
        alpm_pkg_t* pkg = static_cast<alpm_pkg_t*>(i->data);
        QString name = QString::fromUtf8(alpm_pkg_get_name(pkg));
        sizes[name] = alpm_pkg_get_isize(pkg);
    }
    
    return sizes;
}

QString PackageManager::getPackageOwningFile(const QString& filePath) {
    if (!m_initialized) return QString();
    
    alpm_list_t* pkgcache = alpm_db_get_pkgcache(m_localDb);
    for (alpm_list_t* i = pkgcache; i; i = alpm_list_next(i)) {
        alpm_pkg_t* pkg = static_cast<alpm_pkg_t*>(i->data);
        alpm_filelist_t* filelist = alpm_pkg_get_files(pkg);
        
        for (size_t j = 0; j < filelist->count; j++) {
            QString pkgFile = QString("/") + QString::fromUtf8(filelist->files[j].name);
            if (pkgFile == filePath) {
                return QString::fromUtf8(alpm_pkg_get_name(pkg));
            }
        }
    }
    
    return QString();
}

QStringList PackageManager::getPackageFiles(const QString& packageName) {
    QStringList files;
    if (!m_initialized) return files;
    
    alpm_pkg_t* pkg = alpm_db_get_pkg(m_localDb, packageName.toUtf8().constData());
    if (!pkg) return files;
    
    alpm_filelist_t* filelist = alpm_pkg_get_files(pkg);
    for (size_t i = 0; i < filelist->count; i++) {
        files << QString("/") + QString::fromUtf8(filelist->files[i].name);
    }
    
    return files;
}

bool PackageManager::removePackage(const QString& name, bool cascade) {
    // This requires root privileges - implement with polkit or pkexec
    emit operationError("Package removal requires root privileges. Use 'sudo pacman -R " + name + "'");
    return false;
}

bool PackageManager::updateSystem() {
    emit operationError("System update requires root privileges. Use 'sudo pacman -Syu'");
    return false;
}

bool PackageManager::cleanCache(bool keepInstalled) {
    emit operationError("Cache cleaning requires root privileges. Use 'sudo pacman -Sc'");
    return false;
}

void PackageManager::setError(const QString& error) {
    m_lastError = error;
    qWarning() << "PackageManager error:" << error;
}

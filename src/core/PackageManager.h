#ifndef PACKAGEMANAGER_H
#define PACKAGEMANAGER_H

#include <QObject>
#include <QList>
#include <QMap>
#include <QString>
#include <memory>
#include <alpm.h>
#include "models/Package.h"

class PackageManager : public QObject {
    Q_OBJECT
    
public:
    explicit PackageManager(QObject* parent = nullptr);
    ~PackageManager();
    
    // Initialization
    bool initialize();
    void refresh();  // Re-read database
    bool isInitialized() const { return m_initialized; }
    QString lastError() const { return m_lastError; }
    
    // Package queries
    QList<Package> getAllPackages();
    QList<Package> getExplicitPackages();
    QList<Package> getDependencyPackages();
    QList<Package> getOrphanPackages();
    QList<Package> searchPackages(const QString& query);
    Package getPackageInfo(const QString& name);
    bool packageExists(const QString& name);
    
    // Dependency information
    QStringList getDependencies(const QString& packageName);
    QStringList getReverseDependencies(const QString& packageName);
    QMap<QString, QStringList> getDependencyTree(const QString& packageName, int depth = 3);
    
    // Statistics
    int totalPackageCount();
    int explicitPackageCount();
    int dependencyPackageCount();
    int orphanPackageCount();
    qint64 totalInstalledSize();
    QMap<QString, qint64> getSizeByPackage();
    
    // Package operations (requires root)
    bool removePackage(const QString& name, bool cascade = false);
    bool updateSystem();
    bool cleanCache(bool keepInstalled = true);
    
    // File ownership
    QString getPackageOwningFile(const QString& filePath);
    QStringList getPackageFiles(const QString& packageName);
    
signals:
    void packagesChanged();
    void operationProgress(const QString& message, int percent);
    void operationError(const QString& error);
    void operationCompleted(bool success, const QString& message);
    
private:
    Package alpmPackageToPackage(alpm_pkg_t* pkg);
    alpm_list_t* getLocalDatabase();
    void setError(const QString& error);
    
    alpm_handle_t* m_handle = nullptr;
    alpm_db_t* m_localDb = nullptr;
    bool m_initialized = false;
    QString m_lastError;
    QString m_rootDir = "/";
    QString m_dbPath = "/var/lib/pacman/";
};

#endif // PACKAGEMANAGER_H

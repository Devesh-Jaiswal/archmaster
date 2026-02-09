#ifndef PACMANCONFIG_H
#define PACMANCONFIG_H

#include <QString>
#include <QStringList>

class PacmanConfig {
public:
    // Get list of currently pinned (ignored) packages
    static QStringList getIgnoredPackages();
    
    // Add a package to IgnorePkg list
    static bool addIgnoredPackage(const QString& packageName);
    
    // Remove a package from IgnorePkg list
    static bool removeIgnoredPackage(const QString& packageName);
    
    // Check if a package is pinned
    static bool isPackagePinned(const QString& packageName);
    
private:
    static QString configPath();
    static QString readConfig();
    static bool writeConfig(const QString& content);
};

#endif // PACMANCONFIG_H

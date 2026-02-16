#ifndef PACKAGE_H
#define PACKAGE_H

#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QVariant>

struct Package {
    QString name;
    QString version;
    QString description;
    QString url;
    QString packager;
    QString architecture;
    
    qint64 installedSize = 0;      // in bytes
    qint64 downloadSize = 0;       // in bytes
    
    QDateTime installDate;
    QDateTime buildDate;
    
    QString installReason;     // "explicit" or "dependency"
    
    QStringList groups;
    QStringList licenses;
    QStringList depends;
    QStringList optDepends;
    QStringList requiredBy;    // reverse dependencies
    QStringList optionalFor;
    QStringList provides;
    QStringList conflicts;
    QStringList replaces;
    
    // User data (from our database)
    QString userNotes;
    QStringList userTags;
    bool isMarkedKeep = false;
    bool isMarkedReview = false;
    QDateTime lastAccessed;    // tracked via file atime
    
    // Computed properties
    bool isOrphan() const {
        return installReason == "dependency" && requiredBy.isEmpty() && optionalFor.isEmpty();
    }
    
    bool isExplicit() const {
        return installReason == "explicit";
    }
    
    QString formattedSize() const {
        if (installedSize < 1024) return QString::number(installedSize) + " B";
        if (installedSize < 1024 * 1024) return QString::number(installedSize / 1024.0, 'f', 1) + " KB";
        if (installedSize < 1024 * 1024 * 1024) return QString::number(installedSize / (1024.0 * 1024.0), 'f', 1) + " MB";
        return QString::number(installedSize / (1024.0 * 1024.0 * 1024.0), 'f', 2) + " GB";
    }
    
    // For QVariant support
    bool operator==(const Package& other) const {
        return name == other.name && version == other.version;
    }
};

Q_DECLARE_METATYPE(Package)

#endif // PACKAGE_H

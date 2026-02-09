#ifndef DATABASE_H
#define DATABASE_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QSqlDatabase>
#include <QMap>
#include "models/Package.h"

struct PackageUserData {
    QString packageName;
    QString notes;
    QStringList tags;
    bool markedKeep = false;
    bool markedReview = false;
    QDateTime lastViewed;
};

class Database : public QObject {
    Q_OBJECT
    
public:
    explicit Database(QObject* parent = nullptr);
    ~Database();
    
    bool initialize(const QString& path = QString());
    bool isInitialized() const { return m_initialized; }
    QString lastError() const { return m_lastError; }
    
    // User data operations
    bool savePackageUserData(const PackageUserData& data);
    PackageUserData getPackageUserData(const QString& packageName);
    QList<PackageUserData> getAllUserData();
    bool deletePackageUserData(const QString& packageName);
    
    // Convenience methods
    bool setPackageNotes(const QString& packageName, const QString& notes);
    QString getPackageNotes(const QString& packageName);
    
    bool setPackageTags(const QString& packageName, const QStringList& tags);
    QStringList getPackageTags(const QString& packageName);
    bool addPackageTag(const QString& packageName, const QString& tag);
    bool removePackageTag(const QString& packageName, const QString& tag);
    
    bool setPackageKeep(const QString& packageName, bool keep);
    bool isPackageMarkedKeep(const QString& packageName);
    
    bool setPackageReview(const QString& packageName, bool review);
    bool isPackageMarkedReview(const QString& packageName);
    
    // Tag management
    QStringList getAllTags();
    QStringList getPackagesWithTag(const QString& tag);
    
    // Statistics
    int countPackagesWithNotes();
    int countPackagesMarkedKeep();
    int countPackagesMarkedReview();
    
    // Export/Import
    bool exportToJson(const QString& filePath);
    bool importFromJson(const QString& filePath);
    
signals:
    void dataChanged(const QString& packageName);
    
private:
    bool createTables();
    void setError(const QString& error);
    
    QSqlDatabase m_db;
    bool m_initialized = false;
    QString m_lastError;
    QString m_dbPath;
};

#endif // DATABASE_H

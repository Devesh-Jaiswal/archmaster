#include "Database.h"
#include <QDebug>
#include <QDir>
#include <QStandardPaths>
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>

Database::Database(QObject* parent)
    : QObject(parent)
{
}

Database::~Database() {
    if (m_db.isOpen()) {
        m_db.close();
    }
}

bool Database::initialize(const QString& path) {
    if (m_initialized) return true;
    
    // Determine database path
    if (path.isEmpty()) {
        QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(dataDir);
        m_dbPath = dataDir + "/archmaster.db";
    } else {
        m_dbPath = path;
    }
    
    m_db = QSqlDatabase::addDatabase("QSQLITE", "archmaster_db");
    m_db.setDatabaseName(m_dbPath);
    
    if (!m_db.open()) {
        setError(QString("Failed to open database: %1").arg(m_db.lastError().text()));
        return false;
    }
    
    if (!createTables()) {
        return false;
    }
    
    m_initialized = true;
    qDebug() << "Database initialized at:" << m_dbPath;
    return true;
}

bool Database::createTables() {
    QSqlQuery query(m_db);
    
    // Main package user data table
    QString createPackageData = R"(
        CREATE TABLE IF NOT EXISTS package_user_data (
            package_name TEXT PRIMARY KEY,
            notes TEXT DEFAULT '',
            tags TEXT DEFAULT '',
            marked_keep INTEGER DEFAULT 0,
            marked_review INTEGER DEFAULT 0,
            last_viewed TEXT DEFAULT ''
        )
    )";
    
    if (!query.exec(createPackageData)) {
        setError(QString("Failed to create package_user_data table: %1").arg(query.lastError().text()));
        return false;
    }
    
    // Tags table for quick lookup
    QString createTags = R"(
        CREATE TABLE IF NOT EXISTS tags (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            tag_name TEXT UNIQUE NOT NULL
        )
    )";
    
    if (!query.exec(createTags)) {
        setError(QString("Failed to create tags table: %1").arg(query.lastError().text()));
        return false;
    }
    
    // Settings table
    QString createSettings = R"(
        CREATE TABLE IF NOT EXISTS settings (
            key TEXT PRIMARY KEY,
            value TEXT
        )
    )";
    
    if (!query.exec(createSettings)) {
        setError(QString("Failed to create settings table: %1").arg(query.lastError().text()));
        return false;
    }
    
    return true;
}

bool Database::savePackageUserData(const PackageUserData& data) {
    if (!m_initialized) return false;
    
    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT OR REPLACE INTO package_user_data 
        (package_name, notes, tags, marked_keep, marked_review, last_viewed)
        VALUES (?, ?, ?, ?, ?, ?)
    )");
    
    query.addBindValue(data.packageName);
    query.addBindValue(data.notes);
    query.addBindValue(data.tags.join(","));
    query.addBindValue(data.markedKeep ? 1 : 0);
    query.addBindValue(data.markedReview ? 1 : 0);
    query.addBindValue(data.lastViewed.toString(Qt::ISODate));
    
    if (!query.exec()) {
        setError(QString("Failed to save user data: %1").arg(query.lastError().text()));
        return false;
    }
    
    emit dataChanged(data.packageName);
    return true;
}

PackageUserData Database::getPackageUserData(const QString& packageName) {
    PackageUserData data;
    data.packageName = packageName;
    
    if (!m_initialized) return data;
    
    QSqlQuery query(m_db);
    query.prepare("SELECT notes, tags, marked_keep, marked_review, last_viewed FROM package_user_data WHERE package_name = ?");
    query.addBindValue(packageName);
    
    if (query.exec() && query.next()) {
        data.notes = query.value(0).toString();
        QString tagsStr = query.value(1).toString();
        data.tags = tagsStr.isEmpty() ? QStringList() : tagsStr.split(",");
        data.markedKeep = query.value(2).toInt() != 0;
        data.markedReview = query.value(3).toInt() != 0;
        data.lastViewed = QDateTime::fromString(query.value(4).toString(), Qt::ISODate);
    }
    
    return data;
}

QList<PackageUserData> Database::getAllUserData() {
    QList<PackageUserData> list;
    if (!m_initialized) return list;
    
    QSqlQuery query(m_db);
    query.exec("SELECT package_name, notes, tags, marked_keep, marked_review, last_viewed FROM package_user_data");
    
    while (query.next()) {
        PackageUserData data;
        data.packageName = query.value(0).toString();
        data.notes = query.value(1).toString();
        QString tagsStr = query.value(2).toString();
        data.tags = tagsStr.isEmpty() ? QStringList() : tagsStr.split(",");
        data.markedKeep = query.value(3).toInt() != 0;
        data.markedReview = query.value(4).toInt() != 0;
        data.lastViewed = QDateTime::fromString(query.value(5).toString(), Qt::ISODate);
        list.append(data);
    }
    
    return list;
}

bool Database::deletePackageUserData(const QString& packageName) {
    if (!m_initialized) return false;
    
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM package_user_data WHERE package_name = ?");
    query.addBindValue(packageName);
    
    if (!query.exec()) {
        setError(QString("Failed to delete user data: %1").arg(query.lastError().text()));
        return false;
    }
    
    emit dataChanged(packageName);
    return true;
}

bool Database::setPackageNotes(const QString& packageName, const QString& notes) {
    PackageUserData data = getPackageUserData(packageName);
    data.notes = notes;
    return savePackageUserData(data);
}

QString Database::getPackageNotes(const QString& packageName) {
    return getPackageUserData(packageName).notes;
}

bool Database::setPackageTags(const QString& packageName, const QStringList& tags) {
    PackageUserData data = getPackageUserData(packageName);
    data.tags = tags;
    return savePackageUserData(data);
}

QStringList Database::getPackageTags(const QString& packageName) {
    return getPackageUserData(packageName).tags;
}

bool Database::addPackageTag(const QString& packageName, const QString& tag) {
    PackageUserData data = getPackageUserData(packageName);
    if (!data.tags.contains(tag)) {
        data.tags.append(tag);
        return savePackageUserData(data);
    }
    return true;
}

bool Database::removePackageTag(const QString& packageName, const QString& tag) {
    PackageUserData data = getPackageUserData(packageName);
    data.tags.removeAll(tag);
    return savePackageUserData(data);
}

bool Database::setPackageKeep(const QString& packageName, bool keep) {
    PackageUserData data = getPackageUserData(packageName);
    data.markedKeep = keep;
    return savePackageUserData(data);
}

bool Database::isPackageMarkedKeep(const QString& packageName) {
    return getPackageUserData(packageName).markedKeep;
}

bool Database::setPackageReview(const QString& packageName, bool review) {
    PackageUserData data = getPackageUserData(packageName);
    data.markedReview = review;
    return savePackageUserData(data);
}

bool Database::isPackageMarkedReview(const QString& packageName) {
    return getPackageUserData(packageName).markedReview;
}

QStringList Database::getAllTags() {
    QStringList tags;
    if (!m_initialized) return tags;
    
    QSqlQuery query(m_db);
    query.exec("SELECT DISTINCT tags FROM package_user_data WHERE tags != ''");
    
    QSet<QString> uniqueTags;
    while (query.next()) {
        QStringList pkgTags = query.value(0).toString().split(",");
        for (const QString& tag : pkgTags) {
            if (!tag.isEmpty()) {
                uniqueTags.insert(tag);
            }
        }
    }
    
    tags = uniqueTags.values();
    tags.sort();
    return tags;
}

QStringList Database::getPackagesWithTag(const QString& tag) {
    QStringList packages;
    if (!m_initialized) return packages;
    
    QSqlQuery query(m_db);
    query.prepare("SELECT package_name, tags FROM package_user_data WHERE tags LIKE ?");
    query.addBindValue("%" + tag + "%");
    
    while (query.exec() && query.next()) {
        QStringList tags = query.value(1).toString().split(",");
        if (tags.contains(tag)) {
            packages.append(query.value(0).toString());
        }
    }
    
    return packages;
}

int Database::countPackagesWithNotes() {
    if (!m_initialized) return 0;
    
    QSqlQuery query(m_db);
    query.exec("SELECT COUNT(*) FROM package_user_data WHERE notes != ''");
    return query.next() ? query.value(0).toInt() : 0;
}

int Database::countPackagesMarkedKeep() {
    if (!m_initialized) return 0;
    
    QSqlQuery query(m_db);
    query.exec("SELECT COUNT(*) FROM package_user_data WHERE marked_keep = 1");
    return query.next() ? query.value(0).toInt() : 0;
}

int Database::countPackagesMarkedReview() {
    if (!m_initialized) return 0;
    
    QSqlQuery query(m_db);
    query.exec("SELECT COUNT(*) FROM package_user_data WHERE marked_review = 1");
    return query.next() ? query.value(0).toInt() : 0;
}

bool Database::exportToJson(const QString& filePath) {
    if (!m_initialized) return false;
    
    QJsonArray array;
    QList<PackageUserData> allData = getAllUserData();
    
    for (const PackageUserData& data : allData) {
        QJsonObject obj;
        obj["package_name"] = data.packageName;
        obj["notes"] = data.notes;
        obj["tags"] = QJsonArray::fromStringList(data.tags);
        obj["marked_keep"] = data.markedKeep;
        obj["marked_review"] = data.markedReview;
        obj["last_viewed"] = data.lastViewed.toString(Qt::ISODate);
        array.append(obj);
    }
    
    QJsonDocument doc(array);
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        setError(QString("Failed to open file for writing: %1").arg(filePath));
        return false;
    }
    
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

bool Database::importFromJson(const QString& filePath) {
    if (!m_initialized) return false;
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        setError(QString("Failed to open file for reading: %1").arg(filePath));
        return false;
    }
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    file.close();
    
    if (error.error != QJsonParseError::NoError) {
        setError(QString("JSON parse error: %1").arg(error.errorString()));
        return false;
    }
    
    QJsonArray array = doc.array();
    for (const QJsonValue& val : array) {
        QJsonObject obj = val.toObject();
        
        PackageUserData data;
        data.packageName = obj["package_name"].toString();
        data.notes = obj["notes"].toString();
        
        QJsonArray tagsArray = obj["tags"].toArray();
        for (const QJsonValue& tag : tagsArray) {
            data.tags.append(tag.toString());
        }
        
        data.markedKeep = obj["marked_keep"].toBool();
        data.markedReview = obj["marked_review"].toBool();
        data.lastViewed = QDateTime::fromString(obj["last_viewed"].toString(), Qt::ISODate);
        
        savePackageUserData(data);
    }
    
    return true;
}

void Database::setError(const QString& error) {
    m_lastError = error;
    qWarning() << "Database error:" << error;
}

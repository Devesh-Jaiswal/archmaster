#ifndef PROFILEMANAGER_H
#define PROFILEMANAGER_H

#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>

struct PackageProfile {
    QString name;
    QString description;
    QStringList packages;
    bool isBuiltIn = false;
    
    QJsonObject toJson() const;
    static PackageProfile fromJson(const QJsonObject& obj);
};

class ProfileManager {
public:
    ProfileManager();
    
    // Get all profiles (built-in + user)
    QList<PackageProfile> getAllProfiles() const;
    
    // Get built-in profiles
    QList<PackageProfile> getBuiltInProfiles() const;
    
    // Get user profiles
    QList<PackageProfile> getUserProfiles() const;
    
    // Create a new user profile
    bool saveProfile(const PackageProfile& profile);
    
    // Delete a user profile
    bool deleteProfile(const QString& name);
    
    // Get profile by name
    PackageProfile getProfile(const QString& name) const;
    
    // Create profile from currently installed explicit packages
    PackageProfile createFromInstalled() const;
    
private:
    void loadProfiles();
    void initBuiltInProfiles();
    QString profilesPath() const;
    
    QList<PackageProfile> m_builtInProfiles;
    QList<PackageProfile> m_userProfiles;
};

#endif // PROFILEMANAGER_H

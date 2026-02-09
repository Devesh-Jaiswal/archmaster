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
    QList<PackageProfile> m_userProfiles;
    QList<PackageProfile> m_builtInProfiles;
    QStringList m_removedBuiltInProfiles; // Names of built-in profiles that have been "deleted"
    
    QString profilesPath() const;
    QString builtInProfilesPath() const; // For deleted built-ins persistence
    void loadProfiles();
    void initBuiltInProfiles();
    void loadDeletedBuiltIns();
    void saveDeletedBuiltIns();
};

#endif // PROFILEMANAGER_H

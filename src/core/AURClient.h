#ifndef AURCLIENT_H
#define AURCLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

struct AURPackage {
    QString name;
    QString version;
    QString description;
    QString url;
    QString maintainer;
    QString packageBase;
    
    int numVotes = 0;
    double popularity = 0.0;
    
    QDateTime firstSubmitted;
    QDateTime lastModified;
    
    bool outOfDate = false;
    QDateTime outOfDateTime;
    
    QStringList depends;
    QStringList makeDepends;
    QStringList optDepends;
    QStringList conflicts;
    QStringList provides;
    QStringList replaces;
    QStringList keywords;
    QStringList license;
    
    QString aurUrl() const {
        return QString("https://aur.archlinux.org/packages/%1").arg(name);
    }
};

class AURClient : public QObject {
    Q_OBJECT
    
public:
    explicit AURClient(QObject* parent = nullptr);
    ~AURClient();
    
    // Search AUR
    void search(const QString& query);
    void searchByMaintainer(const QString& maintainer);
    void searchByName(const QString& name);
    
    // Get package info
    void getPackageInfo(const QString& packageName);
    void getPackageInfo(const QStringList& packageNames);
    
    // Get orphan packages (no maintainer)
    void getOrphanPackages();
    
    // Check for updates
    void checkForUpdates(const QMap<QString, QString>& installedPackages);
    
    bool isLoading() const { return m_loading; }
    QString lastError() const { return m_lastError; }
    
signals:
    void searchCompleted(const QList<AURPackage>& packages);
    void packageInfoReceived(const AURPackage& package);
    void packagesInfoReceived(const QList<AURPackage>& packages);
    void updatesAvailable(const QList<QPair<QString, QString>>& updates); // name, newVersion
    void error(const QString& errorMessage);
    void loadingChanged(bool loading);
    
private slots:
    void onSearchReply(QNetworkReply* reply);
    void onInfoReply(QNetworkReply* reply);
    
private:
    AURPackage parsePackage(const QJsonObject& obj);
    void setLoading(bool loading);
    void setError(const QString& error);
    
    QNetworkAccessManager* m_networkManager;
    bool m_loading = false;
    QString m_lastError;
    
    static const QString API_BASE;
};

#endif // AURCLIENT_H

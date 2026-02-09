#include "AURClient.h"
#include <QDebug>
#include <QUrlQuery>

const QString AURClient::API_BASE = "https://aur.archlinux.org/rpc/v5";

AURClient::AURClient(QObject* parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
}

AURClient::~AURClient() {
}

void AURClient::search(const QString& query) {
    if (query.length() < 2) {
        setError("Search query must be at least 2 characters");
        return;
    }
    
    setLoading(true);
    
    QUrl url(API_BASE + "/search/" + query);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "ArchMaster/1.0");
    
    QNetworkReply* reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onSearchReply(reply);
    });
}

void AURClient::searchByMaintainer(const QString& maintainer) {
    setLoading(true);
    
    QUrl url(API_BASE + "/search/" + maintainer);
    QUrlQuery query;
    query.addQueryItem("by", "maintainer");
    url.setQuery(query);
    
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "ArchMaster/1.0");
    
    QNetworkReply* reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onSearchReply(reply);
    });
}

void AURClient::searchByName(const QString& name) {
    setLoading(true);
    
    QUrl url(API_BASE + "/search/" + name);
    QUrlQuery query;
    query.addQueryItem("by", "name");
    url.setQuery(query);
    
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "ArchMaster/1.0");
    
    QNetworkReply* reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onSearchReply(reply);
    });
}

void AURClient::getPackageInfo(const QString& packageName) {
    getPackageInfo(QStringList() << packageName);
}

void AURClient::getPackageInfo(const QStringList& packageNames) {
    if (packageNames.isEmpty()) return;
    
    setLoading(true);
    
    QUrl url(API_BASE + "/info");
    QUrlQuery query;
    for (const QString& name : packageNames) {
        query.addQueryItem("arg[]", name);
    }
    url.setQuery(query);
    
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "ArchMaster/1.0");
    
    QNetworkReply* reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onInfoReply(reply);
    });
}

void AURClient::getOrphanPackages() {
    // Empty maintainer search returns orphans
    setLoading(true);
    
    QUrl url(API_BASE + "/search/");
    QUrlQuery query;
    query.addQueryItem("by", "maintainer");
    url.setQuery(query);
    
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "ArchMaster/1.0");
    
    QNetworkReply* reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onSearchReply(reply);
    });
}

void AURClient::checkForUpdates(const QMap<QString, QString>& installedPackages) {
    if (installedPackages.isEmpty()) return;
    
    // Get info for all installed AUR packages
    QStringList names = installedPackages.keys();
    
    // Split into chunks of 200 (API limit)
    const int chunkSize = 200;
    for (int i = 0; i < names.size(); i += chunkSize) {
        QStringList chunk = names.mid(i, chunkSize);
        
        setLoading(true);
        
        QUrl url(API_BASE + "/info");
        QUrlQuery query;
        for (const QString& name : chunk) {
            query.addQueryItem("arg[]", name);
        }
        url.setQuery(query);
        
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::UserAgentHeader, "ArchMaster/1.0");
        
        QNetworkReply* reply = m_networkManager->get(request);
        connect(reply, &QNetworkReply::finished, this, [this, reply, installedPackages]() {
            setLoading(false);
            reply->deleteLater();
            
            if (reply->error() != QNetworkReply::NoError) {
                setError(reply->errorString());
                return;
            }
            
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            QJsonObject root = doc.object();
            
            if (root["type"].toString() == "error") {
                setError(root["error"].toString());
                return;
            }
            
            QList<QPair<QString, QString>> updates;
            QJsonArray results = root["results"].toArray();
            
            for (const QJsonValue& val : results) {
                QJsonObject obj = val.toObject();
                QString name = obj["Name"].toString();
                QString newVersion = obj["Version"].toString();
                QString installedVersion = installedPackages.value(name);
                
                if (!installedVersion.isEmpty() && newVersion != installedVersion) {
                    updates.append(qMakePair(name, newVersion));
                }
            }
            
            if (!updates.isEmpty()) {
                emit updatesAvailable(updates);
            }
        });
    }
}

void AURClient::onSearchReply(QNetworkReply* reply) {
    setLoading(false);
    reply->deleteLater();
    
    if (reply->error() != QNetworkReply::NoError) {
        setError(reply->errorString());
        return;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QJsonObject root = doc.object();
    
    if (root["type"].toString() == "error") {
        setError(root["error"].toString());
        return;
    }
    
    QList<AURPackage> packages;
    QJsonArray results = root["results"].toArray();
    
    for (const QJsonValue& val : results) {
        packages.append(parsePackage(val.toObject()));
    }
    
    emit searchCompleted(packages);
}

void AURClient::onInfoReply(QNetworkReply* reply) {
    setLoading(false);
    reply->deleteLater();
    
    if (reply->error() != QNetworkReply::NoError) {
        setError(reply->errorString());
        return;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QJsonObject root = doc.object();
    
    if (root["type"].toString() == "error") {
        setError(root["error"].toString());
        return;
    }
    
    QList<AURPackage> packages;
    QJsonArray results = root["results"].toArray();
    
    for (const QJsonValue& val : results) {
        packages.append(parsePackage(val.toObject()));
    }
    
    if (packages.size() == 1) {
        emit packageInfoReceived(packages.first());
    } else {
        emit packagesInfoReceived(packages);
    }
}

AURPackage AURClient::parsePackage(const QJsonObject& obj) {
    AURPackage pkg;
    
    pkg.name = obj["Name"].toString();
    pkg.version = obj["Version"].toString();
    pkg.description = obj["Description"].toString();
    pkg.url = obj["URL"].toString();
    pkg.maintainer = obj["Maintainer"].toString();
    pkg.packageBase = obj["PackageBase"].toString();
    
    pkg.numVotes = obj["NumVotes"].toInt();
    pkg.popularity = obj["Popularity"].toDouble();
    
    pkg.firstSubmitted = QDateTime::fromSecsSinceEpoch(obj["FirstSubmitted"].toInteger());
    pkg.lastModified = QDateTime::fromSecsSinceEpoch(obj["LastModified"].toInteger());
    
    if (!obj["OutOfDate"].isNull()) {
        pkg.outOfDate = true;
        pkg.outOfDateTime = QDateTime::fromSecsSinceEpoch(obj["OutOfDate"].toInteger());
    }
    
    // Parse arrays
    auto parseArray = [](const QJsonValue& val) -> QStringList {
        QStringList list;
        if (val.isArray()) {
            for (const QJsonValue& item : val.toArray()) {
                list.append(item.toString());
            }
        }
        return list;
    };
    
    pkg.depends = parseArray(obj["Depends"]);
    pkg.makeDepends = parseArray(obj["MakeDepends"]);
    pkg.optDepends = parseArray(obj["OptDepends"]);
    pkg.conflicts = parseArray(obj["Conflicts"]);
    pkg.provides = parseArray(obj["Provides"]);
    pkg.replaces = parseArray(obj["Replaces"]);
    pkg.keywords = parseArray(obj["Keywords"]);
    pkg.license = parseArray(obj["License"]);
    
    return pkg;
}

void AURClient::setLoading(bool loading) {
    if (m_loading != loading) {
        m_loading = loading;
        emit loadingChanged(loading);
    }
}

void AURClient::setError(const QString& error) {
    m_lastError = error;
    qWarning() << "AURClient error:" << error;
    emit this->error(error);
}

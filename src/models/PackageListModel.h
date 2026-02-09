#ifndef PACKAGELISTMODEL_H
#define PACKAGELISTMODEL_H

#include <QAbstractTableModel>
#include <QList>
#include <QSortFilterProxyModel>
#include "Package.h"

class PackageListModel : public QAbstractTableModel {
    Q_OBJECT
    
public:
    enum Column {
        NameColumn = 0,
        VersionColumn,
        SizeColumn,
        InstallDateColumn,
        ReasonColumn,
        DescriptionColumn,
        ColumnCount
    };
    
    enum Role {
        PackageRole = Qt::UserRole + 1,
        SortRole
    };
    
    explicit PackageListModel(QObject* parent = nullptr);
    
    // QAbstractTableModel interface
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    
    // Data management
    void setPackages(const QList<Package>& packages);
    void addPackage(const Package& package);
    void updatePackage(const Package& package);
    void removePackage(const QString& name);
    void clear();
    
    Package getPackage(int row) const;
    Package getPackageByName(const QString& name) const;
    int findPackageRow(const QString& name) const;
    
    QList<Package> getAllPackages() const { return m_packages; }
    
signals:
    void packagesChanged();
    
private:
    QList<Package> m_packages;
};

// Proxy model for filtering and sorting
class PackageFilterProxyModel : public QSortFilterProxyModel {
    Q_OBJECT
    
public:
    enum FilterType {
        FilterAll,
        FilterExplicit,
        FilterDependency,
        FilterOrphan,
        FilterKeep,
        FilterReview,
        FilterLarge    // > 100MB
    };
    
    explicit PackageFilterProxyModel(QObject* parent = nullptr);
    
    void setFilterType(FilterType type);
    FilterType filterType() const { return m_filterType; }
    
    void setSearchText(const QString& text);
    QString searchText() const { return m_searchText; }
    
    void setTagFilter(const QString& tag);
    QString tagFilter() const { return m_tagFilter; }
    
    void setMinSize(qint64 size);
    void setMaxSize(qint64 size);
    
protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
    bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;
    
private:
    FilterType m_filterType = FilterAll;
    QString m_searchText;
    QString m_tagFilter;
    qint64 m_minSize = 0;
    qint64 m_maxSize = -1;  // -1 means no limit
};

#endif // PACKAGELISTMODEL_H

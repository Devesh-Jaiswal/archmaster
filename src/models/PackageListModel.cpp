#include "PackageListModel.h"
#include <QFont>
#include <QColor>

PackageListModel::PackageListModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

int PackageListModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return m_packages.size();
}

int PackageListModel::columnCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return ColumnCount;
}

QVariant PackageListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_packages.size()) {
        return QVariant();
    }
    
    const Package& pkg = m_packages[index.row()];
    
    if (role == Qt::DisplayRole) {
        switch (index.column()) {
            case NameColumn:
                return pkg.name;
            case VersionColumn:
                return pkg.version;
            case SizeColumn:
                return pkg.formattedSize();
            case InstallDateColumn:
                return pkg.installDate.toString("yyyy-MM-dd");
            case ReasonColumn:
                return pkg.installReason;
            case DescriptionColumn:
                return pkg.description;
        }
    }
    else if (role == SortRole) {
        switch (index.column()) {
            case NameColumn:
                return pkg.name.toLower();
            case VersionColumn:
                return pkg.version;
            case SizeColumn:
                return pkg.installedSize;
            case InstallDateColumn:
                return pkg.installDate;
            case ReasonColumn:
                return pkg.installReason;
            case DescriptionColumn:
                return pkg.description.toLower();
        }
    }
    else if (role == PackageRole) {
        return QVariant::fromValue(pkg);
    }
    else if (role == Qt::ForegroundRole) {
        if (pkg.isOrphan()) {
            return QColor(255, 165, 0);  // Orange for orphans
        }
    }
    else if (role == Qt::FontRole) {
        if (pkg.isExplicit()) {
            QFont font;
            font.setBold(true);
            return font;
        }
    }
    else if (role == Qt::ToolTipRole) {
        return QString("<b>%1</b> %2<br><br>%3<br><br>Installed: %4<br>Size: %5")
            .arg(pkg.name)
            .arg(pkg.version)
            .arg(pkg.description)
            .arg(pkg.installDate.toString("yyyy-MM-dd hh:mm"))
            .arg(pkg.formattedSize());
    }
    
    return QVariant();
}

QVariant PackageListModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return QVariant();
    }
    
    switch (section) {
        case NameColumn: return tr("Name");
        case VersionColumn: return tr("Version");
        case SizeColumn: return tr("Size");
        case InstallDateColumn: return tr("Installed");
        case ReasonColumn: return tr("Reason");
        case DescriptionColumn: return tr("Description");
    }
    
    return QVariant();
}

void PackageListModel::setPackages(const QList<Package>& packages) {
    beginResetModel();
    m_packages = packages;
    endResetModel();
    emit packagesChanged();
}

void PackageListModel::addPackage(const Package& package) {
    beginInsertRows(QModelIndex(), m_packages.size(), m_packages.size());
    m_packages.append(package);
    endInsertRows();
    emit packagesChanged();
}

void PackageListModel::updatePackage(const Package& package) {
    int row = findPackageRow(package.name);
    if (row >= 0) {
        m_packages[row] = package;
        emit dataChanged(index(row, 0), index(row, ColumnCount - 1));
    }
}

void PackageListModel::removePackage(const QString& name) {
    int row = findPackageRow(name);
    if (row >= 0) {
        beginRemoveRows(QModelIndex(), row, row);
        m_packages.removeAt(row);
        endRemoveRows();
        emit packagesChanged();
    }
}

void PackageListModel::clear() {
    beginResetModel();
    m_packages.clear();
    endResetModel();
    emit packagesChanged();
}

Package PackageListModel::getPackage(int row) const {
    if (row >= 0 && row < m_packages.size()) {
        return m_packages[row];
    }
    return Package();
}

Package PackageListModel::getPackageByName(const QString& name) const {
    for (const Package& pkg : m_packages) {
        if (pkg.name == name) {
            return pkg;
        }
    }
    return Package();
}

int PackageListModel::findPackageRow(const QString& name) const {
    for (int i = 0; i < m_packages.size(); ++i) {
        if (m_packages[i].name == name) {
            return i;
        }
    }
    return -1;
}

// PackageFilterProxyModel implementation

PackageFilterProxyModel::PackageFilterProxyModel(QObject* parent)
    : QSortFilterProxyModel(parent)
{
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setSortRole(PackageListModel::SortRole);
}

void PackageFilterProxyModel::setFilterType(FilterType type) {
    if (m_filterType != type) {
        m_filterType = type;
        invalidateRowsFilter();
    }
}

void PackageFilterProxyModel::setSearchText(const QString& text) {
    if (m_searchText != text) {
        m_searchText = text;
        invalidateRowsFilter();
    }
}

void PackageFilterProxyModel::setTagFilter(const QString& tag) {
    if (m_tagFilter != tag) {
        m_tagFilter = tag;
        invalidateRowsFilter();
    }
}

void PackageFilterProxyModel::setMinSize(qint64 size) {
    if (m_minSize != size) {
        m_minSize = size;
        invalidateRowsFilter();
    }
}

void PackageFilterProxyModel::setMaxSize(qint64 size) {
    if (m_maxSize != size) {
        m_maxSize = size;
        invalidateRowsFilter();
    }
}

bool PackageFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const {
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
    Package pkg = index.data(PackageListModel::PackageRole).value<Package>();
    
    // Search text filter
    if (!m_searchText.isEmpty()) {
        bool matchesSearch = pkg.name.contains(m_searchText, Qt::CaseInsensitive) ||
                            pkg.description.contains(m_searchText, Qt::CaseInsensitive);
        if (!matchesSearch) return false;
    }
    
    // Tag filter
    if (!m_tagFilter.isEmpty()) {
        if (!pkg.userTags.contains(m_tagFilter)) return false;
    }
    
    // Size filter
    if (m_minSize > 0 && pkg.installedSize < m_minSize) return false;
    if (m_maxSize >= 0 && pkg.installedSize > m_maxSize) return false;
    
    // Type filter
    switch (m_filterType) {
        case FilterAll:
            return true;
        case FilterExplicit:
            return pkg.isExplicit();
        case FilterDependency:
            return !pkg.isExplicit();
        case FilterOrphan:
            return pkg.isOrphan();
        case FilterKeep:
            return pkg.isMarkedKeep;
        case FilterReview:
            return pkg.isMarkedReview;
        case FilterLarge:
            return pkg.installedSize > 100 * 1024 * 1024;  // > 100MB
    }
    
    return true;
}

bool PackageFilterProxyModel::lessThan(const QModelIndex& left, const QModelIndex& right) const {
    QVariant leftData = sourceModel()->data(left, PackageListModel::SortRole);
    QVariant rightData = sourceModel()->data(right, PackageListModel::SortRole);
    
    if (leftData.typeId() == QMetaType::QString) {
        return leftData.toString() < rightData.toString();
    }
    else if (leftData.typeId() == QMetaType::LongLong) {
        return leftData.toLongLong() < rightData.toLongLong();
    }
    else if (leftData.typeId() == QMetaType::QDateTime) {
        return leftData.toDateTime() < rightData.toDateTime();
    }
    
    return QSortFilterProxyModel::lessThan(left, right);
}

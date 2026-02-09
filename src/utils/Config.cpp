#include "Config.h"
#include <QCoreApplication>
#include <QDir>

Config* Config::s_instance = nullptr;

Config* Config::instance() {
    if (!s_instance) {
        s_instance = new Config(qApp);
    }
    return s_instance;
}

Config::Config(QObject* parent)
    : QObject(parent)
    , m_settings("ArchMaster", "ArchMaster")
{
}

QSize Config::windowSize() const {
    return m_settings.value("window/size", QSize(1200, 800)).toSize();
}

void Config::setWindowSize(const QSize& size) {
    m_settings.setValue("window/size", size);
}

QPoint Config::windowPosition() const {
    return m_settings.value("window/position", QPoint(100, 100)).toPoint();
}

void Config::setWindowPosition(const QPoint& pos) {
    m_settings.setValue("window/position", pos);
}

bool Config::isMaximized() const {
    return m_settings.value("window/maximized", false).toBool();
}

void Config::setMaximized(bool maximized) {
    m_settings.setValue("window/maximized", maximized);
}

bool Config::darkMode() const {
    return m_settings.value("theme/darkMode", true).toBool();  // Default to dark
}

void Config::setDarkMode(bool dark) {
    if (darkMode() != dark) {
        m_settings.setValue("theme/darkMode", dark);
        emit darkModeChanged(dark);
        emit settingsChanged();
    }
}

int Config::defaultSortColumn() const {
    return m_settings.value("packages/sortColumn", 0).toInt();
}

void Config::setDefaultSortColumn(int column) {
    m_settings.setValue("packages/sortColumn", column);
}

Qt::SortOrder Config::defaultSortOrder() const {
    return static_cast<Qt::SortOrder>(m_settings.value("packages/sortOrder", Qt::AscendingOrder).toInt());
}

void Config::setDefaultSortOrder(Qt::SortOrder order) {
    m_settings.setValue("packages/sortOrder", static_cast<int>(order));
}

QList<int> Config::columnWidths() const {
    QList<int> widths;
    QVariantList list = m_settings.value("packages/columnWidths").toList();
    for (const QVariant& v : list) {
        widths.append(v.toInt());
    }
    return widths;
}

void Config::setColumnWidths(const QList<int>& widths) {
    QVariantList list;
    for (int w : widths) {
        list.append(w);
    }
    m_settings.setValue("packages/columnWidths", list);
}

bool Config::confirmBeforeRemove() const {
    return m_settings.value("behavior/confirmRemove", true).toBool();
}

void Config::setConfirmBeforeRemove(bool confirm) {
    m_settings.setValue("behavior/confirmRemove", confirm);
}

bool Config::showOrphanWarning() const {
    return m_settings.value("behavior/orphanWarning", true).toBool();
}

void Config::setShowOrphanWarning(bool show) {
    m_settings.setValue("behavior/orphanWarning", show);
}

int Config::refreshInterval() const {
    return m_settings.value("behavior/refreshInterval", 0).toInt();
}

void Config::setRefreshInterval(int seconds) {
    m_settings.setValue("behavior/refreshInterval", seconds);
}

QString Config::exportPath() const {
    return m_settings.value("paths/export", QDir::homePath()).toString();
}

void Config::setExportPath(const QString& path) {
    m_settings.setValue("paths/export", path);
}

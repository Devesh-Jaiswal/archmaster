#ifndef CONFIG_H
#define CONFIG_H

#include <QObject>
#include <QSettings>
#include <QString>
#include <QSize>
#include <QPoint>

class Config : public QObject {
    Q_OBJECT
    
public:
    static Config* instance();
    
    // Window settings
    QSize windowSize() const;
    void setWindowSize(const QSize& size);
    
    QPoint windowPosition() const;
    void setWindowPosition(const QPoint& pos);
    
    bool isMaximized() const;
    void setMaximized(bool maximized);
    
    // Theme
    bool darkMode() const;
    void setDarkMode(bool dark);
    
    // Package view settings
    int defaultSortColumn() const;
    void setDefaultSortColumn(int column);
    
    Qt::SortOrder defaultSortOrder() const;
    void setDefaultSortOrder(Qt::SortOrder order);
    
    QList<int> columnWidths() const;
    void setColumnWidths(const QList<int>& widths);
    
    // Behavior
    bool confirmBeforeRemove() const;
    void setConfirmBeforeRemove(bool confirm);
    
    bool showOrphanWarning() const;
    void setShowOrphanWarning(bool show);
    
    int refreshInterval() const;  // in seconds, 0 = manual only
    void setRefreshInterval(int seconds);
    
    // Paths
    QString exportPath() const;
    void setExportPath(const QString& path);
    
signals:
    void darkModeChanged(bool dark);
    void settingsChanged();
    
private:
    explicit Config(QObject* parent = nullptr);
    static Config* s_instance;
    
    QSettings m_settings;
};

#endif // CONFIG_H

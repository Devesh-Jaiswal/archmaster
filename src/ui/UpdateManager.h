#ifndef UPDATEMANAGER_H
#define UPDATEMANAGER_H

#include <QWidget>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QProgressBar>
#include <QCheckBox>

class PackageManager;

struct UpdateInfo {
    QString name;
    QString currentVersion;
    QString newVersion;
    QString description;
    QString changelog;
    bool isSecurityUpdate;
    bool isMajorUpdate;
    bool isAUR;
    bool selected;
};

class UpdateManager : public QWidget {
    Q_OBJECT
    
public:
    explicit UpdateManager(PackageManager* pm, QWidget* parent = nullptr);
    
    void applyTheme(bool isDark);
    void checkForUpdates();
    
private slots:
    void onRefresh();
    void onUpdateSelected();
    void onUpdateAll();
    void onUpdateClicked(int row, int column);
    void onSelectAll();
    void onSelectNone();
    void onShowHistory();
    
private:
    void setupUI();
    void showUpdateDetails(int row);
    void parseChangelog(const QString& packageName);
    void parseUpdatesOutput(const QString& output, bool isAUR);
    void checkAURUpdates();
    void finalizeUpdatesList();
    
    PackageManager* m_packageManager;
    
    // UI Elements
    QLabel* m_statusLabel;
    QProgressBar* m_progressBar;
    QTableWidget* m_updatesTable;
    QTextEdit* m_changelogText;
    QPushButton* m_refreshBtn;
    QPushButton* m_updateSelectedBtn;
    QPushButton* m_updateAllBtn;
    QPushButton* m_selectAllBtn;
    QPushButton* m_selectNoneBtn;
    QPushButton* m_historyBtn;
    
    // Update data
    QList<UpdateInfo> m_updates;
};

#endif // UPDATEMANAGER_H

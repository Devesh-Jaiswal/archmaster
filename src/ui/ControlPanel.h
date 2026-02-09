#ifndef CONTROLPANEL_H
#define CONTROLPANEL_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QTextEdit>
#include <QListWidget>
#include <QGroupBox>

class PackageManager;
class Database;

class ControlPanel : public QWidget {
    Q_OBJECT
    
public:
    explicit ControlPanel(PackageManager* pm, Database* db, QWidget* parent = nullptr);
    
private slots:
    void onCleanCache();
    void onRemoveOrphans();
    void onRemoveSelected();
    void onRefreshOrphans();
    void onSyncDatabase();
    void onRefreshKeyrings();
    void onFixDbLock();
    
private:
    void setupUI();
    void runCommand(const QString& title, const QString& command);
    
    PackageManager* m_packageManager;
    Database* m_database;
    
    // System operations
    QPushButton* m_cleanCacheBtn;
    QPushButton* m_removeOrphansBtn;
    QPushButton* m_syncDbBtn;
    QPushButton* m_refreshKeysBtn;
    QPushButton* m_fixDbLockBtn;
    
    // Orphan removal
    QListWidget* m_orphansList;
    QPushButton* m_removeSelectedBtn;
    QPushButton* m_refreshOrphansBtn;
    QLabel* m_orphanSizeLabel;
    
    // Output
    QTextEdit* m_outputText;
    QProgressBar* m_progressBar;
    QLabel* m_statusLabel;
    
    // Cache info
    QLabel* m_cacheSizeLabel;
};

#endif // CONTROLPANEL_H

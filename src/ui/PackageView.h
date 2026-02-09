#ifndef PACKAGEVIEW_H
#define PACKAGEVIEW_H

#include <QWidget>
#include <QTableView>
#include <QLineEdit>
#include <QComboBox>
#include <QSplitter>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>
#include <QListWidget>

class PackageManager;
class Database;
class AURClient;
class PackageListModel;
class PackageFilterProxyModel;
struct Package;

class PackageView : public QWidget {
    Q_OBJECT
    
public:
    explicit PackageView(PackageManager* pm, Database* db, AURClient* aur, QWidget* parent = nullptr);
    
    void loadPackages();
    
signals:
    void packageSelected(const QString& packageName);
    
private slots:
    void onSearchTextChanged(const QString& text);
    void onFilterChanged(int index);
    void onPackageClicked(const QModelIndex& index);
    void onSaveNotes();
    void onAddTag();
    void onRemoveTag();
    void onToggleKeep();
    void onToggleReview();
    void onRemovePackage();
    void onChangeVersion();
    void onTogglePin();
    void onExportClicked();
    
private:
    void setupUI();
    void showPackageDetails(const Package& pkg);
    void updateTagsList();
    void refreshCurrentPackage();
    
    PackageManager* m_packageManager;
    Database* m_database;
    AURClient* m_aurClient;
    
    // Models
    PackageListModel* m_model;
    PackageFilterProxyModel* m_proxyModel;
    
    // Left panel
    QLineEdit* m_searchEdit;
    QComboBox* m_filterCombo;
    QTableView* m_tableView;
    
    // Right panel - details
    QLabel* m_packageNameLabel;
    QLabel* m_packageVersionLabel;
    QLabel* m_packageSizeLabel;
    QLabel* m_packageDateLabel;
    QLabel* m_packageReasonLabel;
    QLabel* m_packageDescLabel;
    
    // Dependencies
    QLabel* m_dependsLabel;
    QLabel* m_requiredByLabel;
    
    // User data
    QTextEdit* m_notesEdit;
    QPushButton* m_saveNotesBtn;
    QLineEdit* m_tagInput;
    QListWidget* m_tagsList;
    QPushButton* m_addTagBtn;
    QPushButton* m_removeTagBtn;
    QPushButton* m_keepBtn;
    QPushButton* m_reviewBtn;
    
    // Package actions
    QPushButton* m_removeBtn;
    QPushButton* m_versionBtn;
    QPushButton* m_pinBtn;
    QPushButton* m_exportBtn;
    
    QString m_currentPackage;
};

#endif // PACKAGEVIEW_H

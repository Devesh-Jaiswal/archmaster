#ifndef PROFILEVIEW_H
#define PROFILEVIEW_H

#include <QWidget>
#include <QListWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>

class ProfileManager;
class PackageManager;

class ProfileView : public QWidget {
    Q_OBJECT
    
public:
    explicit ProfileView(ProfileManager* pm, PackageManager* pkgMgr, QWidget* parent = nullptr);
    
    void applyTheme(bool isDark);
    
private slots:
    void onProfileSelected(int index);
    void onInstallClicked();
    void onCreateClicked();
    void onCreateCustomClicked();
    void onEditClicked();
    void onDeleteClicked();
    void onImportClicked();
    void onExportClicked();
    void onPackageDoubleClicked(QListWidgetItem* item);
    
private:
    void setupUI();
    void refreshProfiles();
    
    ProfileManager* m_profileManager;
    PackageManager* m_packageManager;
    
    QListWidget* m_profileList;
    QLabel* m_profileNameLabel;
    QLabel* m_profileDescLabel;
    QListWidget* m_packageList;
    QPushButton* m_installBtn;
    QPushButton* m_createBtn;
    QPushButton* m_createCustomBtn;
    QPushButton* m_editBtn;
    QPushButton* m_deleteBtn;
    QPushButton* m_importBtn;
    QPushButton* m_exportBtn;
    
    QString m_selectedProfile;
    
    void showPackageDetails(const QString& pkgName);
};

#endif // PROFILEVIEW_H

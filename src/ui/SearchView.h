#ifndef SEARCHVIEW_H
#define SEARCHVIEW_H

#include <QWidget>
#include <QLineEdit>
#include <QTableWidget>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>

class AURClient;
class PackageManager;

class SearchView : public QWidget {
    Q_OBJECT
    
public:
    explicit SearchView(PackageManager* pm, AURClient* aur, QWidget* parent = nullptr);
    
private slots:
    void performSearch();
    void onResultClicked(int row, int column);
    void onInstallClicked();
    
private:
    void setupUI();
    void searchAUR(const QString& query);
    void searchRepo(const QString& query);
    void searchByFile(const QString& filename);
    void showPackageInfo(int row);
    
    PackageManager* m_packageManager;
    AURClient* m_aurClient;
    
    // Search controls
    QLineEdit* m_searchEdit;
    QComboBox* m_sourceCombo;  // AUR or Repo
    QPushButton* m_searchBtn;
    
    // Results
    QTableWidget* m_resultsTable;
    QLabel* m_statusLabel;
    
    // Selected package info
    QTextEdit* m_infoText;
    QPushButton* m_installBtn;
    
    // Current search results metadata
    struct PackageResult {
        QString name;
        QString version;
        QString description;
        QString source;  // "aur" or "repo"
        bool installed;
    };
    QList<PackageResult> m_searchResults;
};

#endif // SEARCHVIEW_H

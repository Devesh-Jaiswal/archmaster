#ifndef PRIVILEGEDRUNNER_H
#define PRIVILEGEDRUNNER_H

#include <QDialog>
#include <QProcess>
#include <QLineEdit>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>

class PrivilegedRunner : public QDialog {
    Q_OBJECT
    
public:
    explicit PrivilegedRunner(const QString& command, 
                               const QString& description,
                               QWidget* parent = nullptr);
    ~PrivilegedRunner();
    
    // Static helper to run a command with GUI
    static bool runCommand(const QString& command,
                          const QString& description,
                          QWidget* parent = nullptr);
                          
    void applyTheme(bool isDark);
    
    bool wasSuccessful() const { return m_success; }
    QString output() const { return m_output; }
    QString errorOutput() const { return m_errorOutput; }
    
signals:
    void commandStarted();
    void commandFinished(bool success);
    void outputReceived(const QString& text);
    
private slots:
    void onAuthenticate();
    void onSendInput();
    void onProcessOutput();
    void onProcessError();
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    
private:
    void setupUI();
    
    QString m_command;
    QString m_description;
    
    QLabel* m_statusLabel;
    QLineEdit* m_passwordEdit;
    QTextEdit* m_outputEdit;
    QPushButton* m_authBtn;
    QPushButton* m_cancelBtn;
    QProgressBar* m_progressBar;
    
    QProcess* m_process;
    bool m_success;
    QString m_output;
    QString m_errorOutput;
    
    // Interactive input
    QLineEdit* m_inputEdit;
    QPushButton* m_sendInputBtn;
    QWidget* m_inputBar;
};

#endif // PRIVILEGEDRUNNER_H

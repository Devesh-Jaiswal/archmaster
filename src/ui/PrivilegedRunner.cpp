#include "PrivilegedRunner.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QApplication>
#include <QRegularExpression>

PrivilegedRunner::PrivilegedRunner(const QString& command, 
                                   const QString& description,
                                   QWidget* parent)
    : QDialog(parent)
    , m_command(command)
    , m_description(description)
    , m_process(nullptr)
    , m_success(false)
{
    setWindowTitle("🔐 Authentication Required");
    setMinimumSize(500, 400);
    setModal(true);
    setupUI();
}

PrivilegedRunner::~PrivilegedRunner() {
    // Ensure process is properly terminated before destruction
    if (m_process) {
        if (m_process->state() != QProcess::NotRunning) {
            m_process->terminate();
            if (!m_process->waitForFinished(2000)) {
                m_process->kill();
                m_process->waitForFinished(1000);
            }
        }
        delete m_process;
        m_process = nullptr;
    }
}

void PrivilegedRunner::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    
    // Description
    QLabel* descLabel = new QLabel(m_description);
    descLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #cdd6f4;");
    descLabel->setWordWrap(true);
    mainLayout->addWidget(descLabel);
    
    // Command preview
    QGroupBox* cmdGroup = new QGroupBox("Command to run:");
    QVBoxLayout* cmdLayout = new QVBoxLayout(cmdGroup);
    QLabel* cmdLabel = new QLabel(m_command);
    cmdLabel->setStyleSheet("font-family: monospace; color: #89b4fa; padding: 10px;");
    cmdLabel->setWordWrap(true);
    cmdLayout->addWidget(cmdLabel);
    mainLayout->addWidget(cmdGroup);
    
    // Password input
    QGroupBox* authGroup = new QGroupBox("Authentication:");
    QHBoxLayout* authLayout = new QHBoxLayout(authGroup);
    
    QLabel* passLabel = new QLabel("Password:");
    m_passwordEdit = new QLineEdit();
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setPlaceholderText("Enter your password...");
    connect(m_passwordEdit, &QLineEdit::returnPressed, this, &PrivilegedRunner::onAuthenticate);
    
    authLayout->addWidget(passLabel);
    authLayout->addWidget(m_passwordEdit);
    mainLayout->addWidget(authGroup);
    
    // Status
    m_statusLabel = new QLabel("Ready to authenticate");
    m_statusLabel->setStyleSheet("color: #a6adc8;");
    mainLayout->addWidget(m_statusLabel);
    
    // Progress bar
    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 0);
    m_progressBar->setVisible(false);
    mainLayout->addWidget(m_progressBar);
    
    // Output display
    QGroupBox* outputGroup = new QGroupBox("Output:");
    QVBoxLayout* outputLayout = new QVBoxLayout(outputGroup);
    
    m_outputEdit = new QTextEdit();
    m_outputEdit->setReadOnly(true);
    m_outputEdit->setStyleSheet(R"(
        QTextEdit {
            background-color: #1e1e2e;
            color: #cdd6f4;
            font-family: monospace;
            font-size: 12px;
            border: 1px solid #45475a;
            border-radius: 6px;
        }
    )");
    m_outputEdit->setMinimumHeight(120);
    outputLayout->addWidget(m_outputEdit);
    mainLayout->addWidget(outputGroup);
    
    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    m_cancelBtn = new QPushButton("Cancel");
    m_cancelBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #45475a;
            color: #cdd6f4;
            border: none;
            border-radius: 6px;
            padding: 10px 20px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #585b70;
        }
    )");
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    
    m_authBtn = new QPushButton("🔓 Authenticate & Run");
    m_authBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #89b4fa;
            color: #1e1e2e;
            border: none;
            border-radius: 6px;
            padding: 10px 20px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #b4befe;
        }
    )");
    connect(m_authBtn, &QPushButton::clicked, this, &PrivilegedRunner::onAuthenticate);
    
    buttonLayout->addWidget(m_cancelBtn);
    buttonLayout->addWidget(m_authBtn);
    mainLayout->addLayout(buttonLayout);
    
    // Style the dialog
    setStyleSheet(R"(
        QDialog {
            background-color: #313244;
        }
        QGroupBox {
            font-weight: bold;
            color: #89b4fa;
            border: 1px solid #45475a;
            border-radius: 8px;
            margin-top: 10px;
            padding-top: 10px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 5px;
        }
        QLineEdit {
            background-color: #1e1e2e;
            color: #cdd6f4;
            border: 1px solid #45475a;
            border-radius: 6px;
            padding: 8px;
        }
        QLabel {
            color: #cdd6f4;
        }
    )");
}

void PrivilegedRunner::onAuthenticate() {
    QString password = m_passwordEdit->text();
    if (password.isEmpty()) {
        m_statusLabel->setText("⚠️ Please enter your password");
        m_statusLabel->setStyleSheet("color: #f38ba8;");
        return;
    }
    
    m_statusLabel->setText("⏳ Running command...");
    m_statusLabel->setStyleSheet("color: #f9e2af;");
    m_progressBar->setVisible(true);
    m_authBtn->setEnabled(false);
    m_passwordEdit->setEnabled(false);
    m_outputEdit->clear();
    
    // Create process with sudo
    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::MergedChannels);
    
    connect(m_process, &QProcess::readyReadStandardOutput, this, &PrivilegedRunner::onProcessOutput);
    connect(m_process, &QProcess::readyReadStandardError, this, &PrivilegedRunner::onProcessError);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &PrivilegedRunner::onProcessFinished);
    
    // Use sudo with -S flag to read password from stdin
    m_process->start("sudo", {"-S", "bash", "-c", m_command});
    
    if (!m_process->waitForStarted(3000)) {
        m_statusLabel->setText("❌ Failed to start process");
        m_statusLabel->setStyleSheet("color: #f38ba8;");
        m_progressBar->setVisible(false);
        m_authBtn->setEnabled(true);
        m_passwordEdit->setEnabled(true);
        return;
    }
    
    // Send password to sudo via stdin
    m_process->write((password + "\n").toUtf8());
    m_process->closeWriteChannel();
    
    emit commandStarted();
}

void PrivilegedRunner::onProcessOutput() {
    if (m_process) {
        QString output = QString::fromUtf8(m_process->readAllStandardOutput());
        // Filter out password prompt
        output.remove(QRegularExpression("\\[sudo\\].*:"));
        m_output += output;
        m_outputEdit->append(output);
        emit outputReceived(output);
    }
}

void PrivilegedRunner::onProcessError() {
    if (m_process) {
        QString error = QString::fromUtf8(m_process->readAllStandardError());
        m_errorOutput += error;
        m_outputEdit->append("<span style='color: #f38ba8;'>" + error + "</span>");
    }
}

void PrivilegedRunner::onProcessFinished(int exitCode, QProcess::ExitStatus status) {
    m_progressBar->setVisible(false);
    
    if (status == QProcess::NormalExit && exitCode == 0) {
        m_success = true;
        m_statusLabel->setText("✅ Command completed successfully!");
        m_statusLabel->setStyleSheet("color: #a6e3a1;");
        
        // Auto-close after success
        m_authBtn->setText("Done");
        m_authBtn->setEnabled(true);
        disconnect(m_authBtn, &QPushButton::clicked, this, &PrivilegedRunner::onAuthenticate);
        connect(m_authBtn, &QPushButton::clicked, this, &QDialog::accept);
    } else {
        m_success = false;
        if (m_errorOutput.contains("incorrect password", Qt::CaseInsensitive) ||
            m_errorOutput.contains("sorry", Qt::CaseInsensitive)) {
            m_statusLabel->setText("❌ Incorrect password. Please try again.");
        } else {
            m_statusLabel->setText(QString("❌ Command failed (exit code: %1)").arg(exitCode));
        }
        m_statusLabel->setStyleSheet("color: #f38ba8;");
        m_authBtn->setEnabled(true);
        m_passwordEdit->setEnabled(true);
        m_passwordEdit->clear();
        m_passwordEdit->setFocus();
    }
    
    emit commandFinished(m_success);
}

bool PrivilegedRunner::runCommand(const QString& command,
                                  const QString& description,
                                  QWidget* parent) {
    PrivilegedRunner dialog(command, description, parent);
    return dialog.exec() == QDialog::Accepted && dialog.wasSuccessful();
}

#include "PrivilegedRunner.h"
#include "utils/Config.h"
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
    setMinimumSize(600, 550);
    resize(650, 600);
    setModal(true);
    setupUI();
    // Apply initial theme
    applyTheme(Config::instance()->darkMode());
}

void PrivilegedRunner::applyTheme(bool isDark) {
    QString bgColor = isDark ? "#313244" : "#eff1f5"; // Dialog (Surface0 in Latte/Mocha? Check Palette)
    // Actually for Dialog main bg, Mocha Base is #1e1e2e, Surface0 is #313244. 
    // Latte Base is #eff1f5, Surface0 is #ccd0da. Let's use Base for dialogs usually or Surface. 
    // The previous hardcoded was #313244 (Surface0). Let's stick to Surface0/Base logic.
    QString dialogBg = isDark ? "#313244" : "#e6e9ef"; 
    QString groupBg = isDark ? "#1e1e2e" : "#eff1f5";
    QString textColor = isDark ? "#cdd6f4" : "#4c4f69";
    QString borderColor = isDark ? "#45475a" : "#ccd0da";
    QString headerColor = isDark ? "#89b4fa" : "#1e66f5"; // Blue
    QString subTextColor = isDark ? "#a6adc8" : "#6c6f85";
    QString inputBg = isDark ? "#1e1e2e" : "#eff1f5";
    
    // Dialog
    setStyleSheet(QString("QDialog { background-color: %1; }").arg(dialogBg));
    
    // Labels
    QList<QLabel*> labels = this->findChildren<QLabel*>();
    for(auto label : labels) {
         if (label->text() == m_description) {
              label->setStyleSheet(QString("font-size: 14px; font-weight: bold; color: %1;").arg(textColor));
         } else if (label->text() == m_command) {
              label->setStyleSheet(QString("font-family: monospace; color: %1; padding: 10px;").arg(headerColor));
         } else if (label->text().contains("Input:")) {
              label->setStyleSheet(QString("color: %1; font-weight: bold; font-size: 13px;").arg(isDark ? "#f9e2af" : "#df8e1d"));
         } else if (label == m_statusLabel) {
              // Status label color changes dynamically, just set base font if needed?
              // The setupUI sets color: #a6adc8. We can leave it or set a base.
         } else {
              // General labels
              label->setStyleSheet(QString("color: %1;").arg(textColor));
         }
    }
    
    // GroupBoxes
    QString groupStyle = QString(R"(
        QGroupBox {
             font-weight: bold;
             color: %1;
             border: 1px solid %2;
             border-radius: 8px;
             margin-top: 10px;
             padding-top: 10px;
        }
        QGroupBox::title {
             subcontrol-origin: margin;
             left: 10px;
             padding: 0 5px;
        }
    )").arg(headerColor, borderColor);
    
    QList<QGroupBox*> groups = this->findChildren<QGroupBox*>();
    for (QGroupBox* group : groups) {
        group->setStyleSheet(groupStyle);
    }
    
    // LineEdits
    QString inputStyle = QString(R"(
        QLineEdit {
            background-color: %1;
            color: %2;
            border: 1px solid %3;
            border-radius: 6px;
            padding: 8px;
        }
    )").arg(inputBg, textColor, borderColor);
    
    if(m_passwordEdit) m_passwordEdit->setStyleSheet(inputStyle);
    
    // Input Bar Edit (Special)
    if(m_inputEdit) {
        m_inputEdit->setStyleSheet(QString(R"(
            QLineEdit {
                background-color: %1;
                color: %2;
                border: 2px solid %3;
                border-radius: 6px;
                padding: 8px;
                font-family: monospace; 
                font-size: 13px;
            }
        )").arg(inputBg, isDark ? "#a6e3a1" : "#40a02b", isDark ? "#f9e2af" : "#df8e1d"));
    }
    
    // Output Text
    if(m_outputEdit) {
        m_outputEdit->setStyleSheet(QString(R"(
            QTextEdit {
                background-color: %1;
                color: %2;
                font-family: monospace;
                font-size: 12px;
                border: 1px solid %3;
                border-radius: 6px;
            }
        )").arg(inputBg, textColor, borderColor));
    }
    
    // Buttons
    // Cancel
    if(m_cancelBtn) {
        m_cancelBtn->setStyleSheet(QString(R"(
            QPushButton {
                background-color: %1;
                color: %2;
                border: none;
                border-radius: 6px;
                padding: 10px 20px;
                font-weight: bold;
            }
            QPushButton:hover {
                background-color: %3;
            }
        )").arg(isDark ? "#45475a" : "#ccd0da", textColor, isDark ? "#585b70" : "#bcc0cc"));
    }
    
    // Auth / Main Action
    QString mainBtnStyle = QString(R"(
        QPushButton {
            background-color: %1;
            color: %2;
            border: none;
            border-radius: 6px;
            padding: 10px 20px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: %3;
        }
    )").arg(headerColor, isDark ? "#1e1e2e" : "#ffffff", isDark ? "#b4befe" : "#7287fd");
    
    if(m_authBtn) m_authBtn->setStyleSheet(mainBtnStyle);
    
    // Input Send Button
    if(m_sendInputBtn) {
        m_sendInputBtn->setStyleSheet(QString(R"(
            QPushButton {
                background-color: %1;
                color: %2;
                border: none;
                border-radius: 6px;
                padding: 8px 20px;
                font-weight: bold;
                font-size: 13px;
            }
            QPushButton:hover {
                background-color: %3;
            }
        )").arg(isDark ? "#a6e3a1" : "#40a02b", isDark ? "#1e1e2e" : "#ffffff", isDark ? "#94e2d5" : "#179299"));
    }
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
    // Style applied by applyTheme
    descLabel->setWordWrap(true);
    mainLayout->addWidget(descLabel);
    
    // Command preview
    QGroupBox* cmdGroup = new QGroupBox("Command to run:");
    QVBoxLayout* cmdLayout = new QVBoxLayout(cmdGroup);
    QLabel* cmdLabel = new QLabel(m_command);
    // Style applied by applyTheme
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
    // Style applied by applyTheme
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
    // Style applied by applyTheme
    m_outputEdit->setMinimumHeight(150);
    outputLayout->addWidget(m_outputEdit);
    
    // Interactive input bar INSIDE the output group for visibility
    QFrame* inputSeparator = new QFrame();
    inputSeparator->setFrameShape(QFrame::HLine);
    inputSeparator->setStyleSheet("color: #45475a;");
    
    m_inputBar = new QWidget();
    QHBoxLayout* inputBarLayout = new QHBoxLayout(m_inputBar);
    inputBarLayout->setContentsMargins(0, 5, 0, 0);
    
    QLabel* inputLabel = new QLabel("⌨ Input:");
    // Style applied by applyTheme
    m_inputEdit = new QLineEdit();
    m_inputEdit->setPlaceholderText("Type response here (e.g. Y/n, v/m/s/r/o/q) and press Enter...");
    // Style applied by applyTheme
    m_inputEdit->setMinimumHeight(36);
    // Connection moved to after button creation to avoid null pointer access
    // and to serialize input via button click
    
    m_sendInputBtn = new QPushButton("Send ↵");
    m_sendInputBtn->setMinimumHeight(36);
    m_sendInputBtn->setAutoDefault(false);
    m_sendInputBtn->setDefault(false);
    // Style applied by applyTheme
    connect(m_sendInputBtn, &QPushButton::clicked, this, &PrivilegedRunner::onSendInput);
    
    // Trigger onSendInput on Enter (autoDefault is false on button, so this is safe)
    connect(m_inputEdit, &QLineEdit::returnPressed, this, &PrivilegedRunner::onSendInput);
    
    inputBarLayout->addWidget(inputLabel);
    inputBarLayout->addWidget(m_inputEdit, 1);
    inputBarLayout->addWidget(m_sendInputBtn);
    m_inputBar->setVisible(false);
    
    outputLayout->addWidget(inputSeparator);
    outputLayout->addWidget(m_inputBar);
    mainLayout->addWidget(outputGroup);
    
    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    m_cancelBtn = new QPushButton("Cancel");
    // Style applied by applyTheme
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    
    m_authBtn = new QPushButton("🔓 Authenticate & Run");
    // Style applied by applyTheme
    // Make Auth button default so Enter triggers it
    m_authBtn->setAutoDefault(true);
    m_authBtn->setDefault(true);
    
    // Explicit connection removed to prevent double-trigger (setDefault handles it)
    // connect(m_passwordEdit, &QLineEdit::returnPressed, this, &PrivilegedRunner::onAuthenticate);
    
    connect(m_authBtn, &QPushButton::clicked, this, &PrivilegedRunner::onAuthenticate);
    
    buttonLayout->addWidget(m_cancelBtn);
    buttonLayout->addWidget(m_authBtn);
    mainLayout->addLayout(buttonLayout);
    
    // Style the dialog
}

void PrivilegedRunner::onAuthenticate() {
    // Prevent re-entrancy or double-clicks
    if (!m_authBtn->isEnabled()) return;
    
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
    
    // Clean up any previous process (e.g. from a failed auth attempt)
    if (m_process) {
        if (m_process->state() != QProcess::NotRunning) {
            m_process->terminate();
            m_process->waitForFinished(1000);
        }
        delete m_process;
        m_process = nullptr;
    }
    
    // Create process with sudo
    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::MergedChannels);
    
    connect(m_process, &QProcess::readyReadStandardOutput, this, &PrivilegedRunner::onProcessOutput);
    connect(m_process, &QProcess::readyReadStandardError, this, &PrivilegedRunner::onProcessError);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &PrivilegedRunner::onProcessFinished);
    
    // Use sudo with -S flag to read password from stdin
    // Wrap with 'script' to provide a PTY for interactive commands (e.g. pacdiff)
    // Create a temp wrapper script for diff -u to avoid shell quoting issues with export
    QString setupWrapper = "echo -e '#!/bin/sh\\ndiff -u \"$@\"' > /tmp/pacdiff_diff && chmod +x /tmp/pacdiff_diff";
    
    QString escapedCmd = m_command;
    escapedCmd.replace("'", "'\\''");
    
    QString wrappedCmd = QString("%1 && script -qec 'export DIFFPROG=/tmp/pacdiff_diff; %2' /dev/null").arg(setupWrapper, escapedCmd);
    m_process->start("sudo", {"-S", "bash", "-c", wrappedCmd});
    
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
    // Don't close write channel — keep stdin open for interactive input
    
    // Show the interactive input bar prominently
    m_inputBar->setVisible(true);
    m_inputEdit->setFocus();
    m_statusLabel->setText("⏳ Running... Type input below if the command asks for it.");
    m_statusLabel->setStyleSheet("color: #f9e2af; font-weight: bold;");
    
    emit commandStarted();
}

void PrivilegedRunner::onProcessOutput() {
    if (m_process) {
        QString output = QString::fromUtf8(m_process->readAllStandardOutput());
        // Filter out password prompt
        output.remove(QRegularExpression("\\[sudo\\].*:"));
        // Filter out ANSI escape codes (CSI sequences)
        static const QRegularExpression ansiRegex("\x1b\\[[0-9;?]*[a-zA-Z]");
        output.remove(ansiRegex);
        
        m_output += output;
        m_outputEdit->moveCursor(QTextCursor::End);
        m_outputEdit->insertPlainText(output);
        m_outputEdit->moveCursor(QTextCursor::End);
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
    m_inputBar->setVisible(false);  // Hide input once process is done
    
    if (status == QProcess::NormalExit && exitCode == 0) {
        m_success = true;
        m_statusLabel->setText("✅ Command completed successfully!");
        m_statusLabel->setStyleSheet("color: #a6e3a1;");
        
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

void PrivilegedRunner::onSendInput() {
    if (!m_process) return;
    
    if (m_process->state() == QProcess::Running) {
        QString input = m_inputEdit->text() + "\n";
        m_process->write(input.toUtf8());
        m_inputEdit->clear();
        
        // No manual echo needed - the PTY/script command will echo the input back
        // m_outputEdit->insertPlainText(input);
    }
}

bool PrivilegedRunner::runCommand(const QString& command,
                                  const QString& description,
                                  QWidget* parent) {
    PrivilegedRunner dialog(command, description, parent);
    return dialog.exec() == QDialog::Accepted && dialog.wasSuccessful();
}

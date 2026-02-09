#include "PacmanConfig.h"
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QProcess>

QString PacmanConfig::configPath() {
    return "/etc/pacman.conf";
}

QString PacmanConfig::readConfig() {
    QFile file(configPath());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }
    QString content = QString::fromUtf8(file.readAll());
    file.close();
    return content;
}

bool PacmanConfig::writeConfig(const QString& content) {
    // Writing to /etc/pacman.conf requires sudo
    // We use a temp file and then copy with sudo
    QString tempPath = "/tmp/pacman.conf.tmp";
    
    QFile tempFile(tempPath);
    if (!tempFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    
    QTextStream out(&tempFile);
    out << content;
    tempFile.close();
    
    // Copy with sudo
    QProcess proc;
    proc.start("pkexec", {"cp", tempPath, configPath()});
    proc.waitForFinished(10000);
    
    // Clean up temp file
    QFile::remove(tempPath);
    
    return proc.exitCode() == 0;
}

QStringList PacmanConfig::getIgnoredPackages() {
    QStringList ignored;
    QString content = readConfig();
    
    if (content.isEmpty()) return ignored;
    
    // Find IgnorePkg line(s)
    QRegularExpression re(R"(^\s*IgnorePkg\s*=\s*(.+)$)", QRegularExpression::MultilineOption);
    QRegularExpressionMatchIterator iter = re.globalMatch(content);
    
    while (iter.hasNext()) {
        QRegularExpressionMatch match = iter.next();
        QString pkgList = match.captured(1).trimmed();
        // Packages are space-separated
        QStringList pkgs = pkgList.split(QRegularExpression(R"(\s+)"), Qt::SkipEmptyParts);
        ignored.append(pkgs);
    }
    
    ignored.removeDuplicates();
    return ignored;
}

bool PacmanConfig::isPackagePinned(const QString& packageName) {
    return getIgnoredPackages().contains(packageName);
}

bool PacmanConfig::addIgnoredPackage(const QString& packageName) {
    if (isPackagePinned(packageName)) {
        return true; // Already pinned
    }
    
    QString content = readConfig();
    if (content.isEmpty()) return false;
    
    // Check if IgnorePkg line exists
    QRegularExpression re(R"(^(\s*IgnorePkg\s*=\s*)(.*)$)", QRegularExpression::MultilineOption);
    QRegularExpressionMatch match = re.match(content);
    
    if (match.hasMatch()) {
        // Add to existing line
        QString prefix = match.captured(1);
        QString existing = match.captured(2).trimmed();
        QString newValue;
        if (existing.isEmpty()) {
            newValue = packageName;
        } else {
            newValue = existing + " " + packageName;
        }
        content.replace(match.capturedStart(), match.capturedLength(), prefix + newValue);
    } else {
        // Add new IgnorePkg line in [options] section
        QRegularExpression optionsRe(R"((\[options\][^\[]*))");
        QRegularExpressionMatch optMatch = optionsRe.match(content);
        
        if (optMatch.hasMatch()) {
            QString optionsSection = optMatch.captured(1);
            QString newSection = optionsSection.trimmed() + "\nIgnorePkg = " + packageName + "\n";
            content.replace(optMatch.capturedStart(), optMatch.capturedLength(), newSection);
        } else {
            return false; // Can't find [options] section
        }
    }
    
    return writeConfig(content);
}

bool PacmanConfig::removeIgnoredPackage(const QString& packageName) {
    if (!isPackagePinned(packageName)) {
        return true; // Not pinned anyway
    }
    
    QString content = readConfig();
    if (content.isEmpty()) return false;
    
    // Find and modify IgnorePkg line
    QRegularExpression re(R"(^(\s*IgnorePkg\s*=\s*)(.*)$)", QRegularExpression::MultilineOption);
    QRegularExpressionMatch match = re.match(content);
    
    if (match.hasMatch()) {
        QString prefix = match.captured(1);
        QString existing = match.captured(2).trimmed();
        
        QStringList pkgs = existing.split(QRegularExpression(R"(\s+)"), Qt::SkipEmptyParts);
        pkgs.removeAll(packageName);
        
        QString newValue = pkgs.join(" ");
        content.replace(match.capturedStart(), match.capturedLength(), prefix + newValue);
        
        return writeConfig(content);
    }
    
    return false;
}

#include <QApplication>
#include <QIcon>
#include "ui/MainWindow.h"
#include "models/Package.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    
    // Application metadata
    app.setApplicationName("ArchMaster");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("ArchMaster");
    app.setOrganizationDomain("archmaster.local");
    
    // Register metatypes
    qRegisterMetaType<Package>("Package");
    
    // Set application icon
    app.setWindowIcon(QIcon(":/icons/archmaster.svg"));
    
    // Create and show main window
    MainWindow mainWindow;
    mainWindow.show();
    
    return app.exec();
}

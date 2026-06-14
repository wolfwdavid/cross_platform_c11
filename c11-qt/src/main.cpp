#include "app/C11Application.h"
#include "ghostty/GhosttyRuntime.h"
#include "ui/MainWindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication qtApp(argc, argv);
    QApplication::setApplicationName("c11");
    QApplication::setApplicationVersion(C11_VERSION);
    QApplication::setOrganizationName("Stage 11");
    QApplication::setOrganizationDomain("stage11.com");

    c11::C11Application app;
    if (!app.initialize()) {
        return 1;
    }

    c11::MainWindow mainWindow(app);
    mainWindow.show();
    mainWindow.raise();
    mainWindow.activateWindow();

    return qtApp.exec();
}

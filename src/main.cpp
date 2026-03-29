#include "ui/MainWindow.h"
#include <QApplication>
#include <QSettings>
#include <QStyleFactory>

int main(int argc, char* argv[])
{
    // Let Qt inherit the OS color scheme (light/dark) automatically.
    // Fusion style provides consistent cross-platform rendering and
    // correctly propagates QGuiApplication::styleHints()->colorScheme().
    QApplication::setStyle(QStyleFactory::create("Fusion"));

    QApplication app(argc, argv);
    app.setApplicationName("SPE 1K-FA Control");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("SPE");
    app.setOrganizationDomain("com.spe");

    // Ensure QSettings uses a consistent location
    QSettings::setDefaultFormat(QSettings::IniFormat);

    MainWindow window;
    window.show();

    return app.exec();
}

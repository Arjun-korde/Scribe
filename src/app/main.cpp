#include "MainWindow.h"

#include <QApplication>

#include <memory>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("Scribe");
    QApplication::setApplicationVersion(PROJECT_VERSION);
    QApplication::setOrganizationName("Scribe");

    auto window = std::make_unique<MainWindow>();
    window->show();

    return app.exec();
}

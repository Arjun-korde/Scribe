#include "MainWindow.h"
#include "SettingsManager.h"

#include <QApplication>
#include <QIcon>

#include <memory>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("Scribe");
    QApplication::setApplicationVersion(PROJECT_VERSION);
    QApplication::setOrganizationName("Scribe");
    QApplication::setWindowIcon(QIcon(":/icons/scribe.ico"));

    // Declared before the window, so it is destroyed after it: the window saves on its way out.
    scribe::core::SettingsManager settings(scribe::core::SettingsManager::defaultFilePath());

    auto window = std::make_unique<MainWindow>(defaultFileDialogs(), &settings);
    window->show();

    return app.exec();
}

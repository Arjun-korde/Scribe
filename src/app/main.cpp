#include "MainWindow.h"
#include "SettingsManager.h"
#include "ThemeManager.h"

#include <QApplication>
#include <QIcon>

#include <memory>

int main(int argc, char* argv[])
{
    // Per-monitor DPI awareness (v2) is Qt 6's default on Windows. Fractional scales such as 125%
    // and 150% must be used as they are: rounding them to 100% or 200% makes text blurry or too
    // big. Set before the application exists, as Qt requires.
    QApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QApplication app(argc, argv);
    QApplication::setApplicationName("Scribe");
    QApplication::setApplicationVersion(PROJECT_VERSION);
    QApplication::setOrganizationName("Scribe");
    QApplication::setWindowIcon(QIcon(":/icons/scribe.ico"));

    scribe::app::ThemeManager themes(app); // before the window, so it is first painted themed

    // Declared before the window, so it is destroyed after it: the window saves on its way out.
    scribe::core::SettingsManager settings(scribe::core::SettingsManager::defaultFilePath());

    auto window = std::make_unique<MainWindow>(defaultFileDialogs(), &settings);
    window->show();

    return app.exec();
}

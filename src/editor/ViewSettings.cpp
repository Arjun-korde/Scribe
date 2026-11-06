#include "ViewSettings.h"

#include <QFontDatabase>
#include <QStringList>

namespace scribe::editor
{

QFont ViewSettings::defaultFont()
{
    const QStringList installed = QFontDatabase::families();
    for (const QString& family : {QStringLiteral("Cascadia Mono"), QStringLiteral("Consolas")}) {
        if (installed.contains(family)) {
            QFont font(family);
            font.setPointSize(11);
            return font;
        }
    }

    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setPointSize(11);
    return font;
}

QFont ViewSettings::zoomedFont() const
{
    QFont scaled = font;
    if (font.pointSizeF() > 0) {
        scaled.setPointSizeF(font.pointSizeF() * zoomPercent / 100.0);
    } else if (font.pixelSize() > 0) {
        scaled.setPixelSize(std::max(1, font.pixelSize() * zoomPercent / 100));
    }
    return scaled;
}

} // namespace scribe::editor

#include "ViewSettingsStore.h"

#include "SettingsManager.h"

#include <QFontDatabase>
#include <QStringList>

namespace scribe::editor
{

namespace
{

constexpr double kMinFontSize = 1.0;
constexpr double kMaxFontSize = 200.0;
constexpr int kMinWeight = 1;
constexpr int kMaxWeight = 1000;

} // namespace

ViewSettings ViewSettingsStore::load(const core::SettingsManager& settings)
{
    ViewSettings view; // starts with every default

    view.wordWrap = settings.boolValue(kWordWrapKey, view.wordWrap);
    view.statusBarVisible = settings.boolValue(kStatusBarKey, view.statusBarVisible);
    view.zoomPercent = settings.intValue(kZoomKey, view.zoomPercent, ViewSettings::kMinZoomPercent,
                                         ViewSettings::kMaxZoomPercent);

    // A family that is no longer installed would silently be drawn in some other font while the
    // font dialog still claimed otherwise, so ignore it and keep the default family.
    const QString family = settings.stringValue(kFontFamilyKey, QString());
    if (!family.isEmpty() && QFontDatabase::families().contains(family)) {
        view.font.setFamily(family);
    }

    bool sizeOk = false;
    const double size = settings.stringValue(kFontSizeKey, QString()).trimmed().toDouble(&sizeOk);
    if (sizeOk && size >= kMinFontSize && size <= kMaxFontSize) { // also rejects nan and inf
        view.font.setPointSizeF(size);
    }

    const int weight = settings.intValue(kFontWeightKey, static_cast<int>(view.font.weight()),
                                         kMinWeight, kMaxWeight);
    view.font.setWeight(static_cast<QFont::Weight>(weight));
    view.font.setItalic(settings.boolValue(kFontItalicKey, view.font.italic()));

    return view;
}

void ViewSettingsStore::save(core::SettingsManager& settings, const ViewSettings& view)
{
    settings.setBool(kWordWrapKey, view.wordWrap);
    settings.setBool(kStatusBarKey, view.statusBarVisible);
    settings.setInt(kZoomKey, view.zoomPercent);

    settings.setString(kFontFamilyKey, view.font.family());
    if (view.font.pointSizeF() > 0) {
        settings.setString(kFontSizeKey, QString::number(view.font.pointSizeF(), 'g', 6));
    } else {
        settings.remove(kFontSizeKey); // sized in pixels: nothing sensible to store
    }
    settings.setInt(kFontWeightKey, static_cast<int>(view.font.weight()));
    settings.setBool(kFontItalicKey, view.font.italic());
}

} // namespace scribe::editor

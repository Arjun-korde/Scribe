#include "SettingsManager.h"

#include <QDir>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QMetaType>
#include <QVariant>

#include <algorithm>
#include <limits>

namespace scribe::core
{

namespace
{

Q_LOGGING_CATEGORY(lcSettings, "scribe.settings")

constexpr auto kVersionKey = "settings/version";

// QSettings reads an unquoted value that contains commas as a list. Such a value is usually text
// that was typed into the file by hand ("Courier New, Bold"), so put the commas back.
QString textOfVariant(const QVariant& value)
{
    if (value.metaType() == QMetaType::fromType<QStringList>()) {
        return value.toStringList().join(QLatin1Char(','));
    }
    return value.toString();
}

} // namespace

SettingsManager::SettingsManager(const QString& filePath)
    : m_filePath(filePath)
    , m_settings(filePath, QSettings::IniFormat) // Qt 6 reads and writes INI files as UTF-8
{}

QString SettingsManager::defaultFilePath()
{
    QString base = qEnvironmentVariable("APPDATA");
    if (base.isEmpty()) {
        base = QDir::homePath() + QStringLiteral("/AppData/Roaming");
    }
    return QDir::cleanPath(base + QStringLiteral("/Scribe/scribe.ini"));
}

// ---- Reading --------------------------------------------------------------------------------

QString SettingsManager::textOf(const QString& key) const
{
    return textOfVariant(m_settings.value(key));
}

bool SettingsManager::contains(const QString& key) const
{
    return !key.isEmpty() && m_settings.contains(key);
}

bool SettingsManager::boolValue(const QString& key, bool defaultValue) const
{
    if (!contains(key)) {
        return defaultValue;
    }

    const QString text = textOf(key).trimmed();
    if (text.compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0 ||
        text == QLatin1String("1")) {
        return true;
    }
    if (text.compare(QStringLiteral("false"), Qt::CaseInsensitive) == 0 ||
        text == QLatin1String("0")) {
        return false;
    }
    return defaultValue;
}

int SettingsManager::intValue(const QString& key, int defaultValue, int minimum, int maximum) const
{
    if (!contains(key)) {
        return defaultValue;
    }

    bool ok = false;
    const int value = textOf(key).trimmed().toInt(&ok);
    if (!ok || value < minimum || value > maximum) {
        return defaultValue;
    }
    return value;
}

QString SettingsManager::stringValue(const QString& key, const QString& defaultValue) const
{
    return contains(key) ? textOf(key) : defaultValue;
}

QStringList SettingsManager::stringList(const QString& key, int maxItems) const
{
    QStringList items;
    if (key.isEmpty() || maxItems <= 0) {
        return items;
    }

    const int stored = m_settings.beginReadArray(key);
    const int count = std::min(stored, maxItems); // "size" in a damaged file can be enormous
    for (int i = 0; i < count; ++i) {
        m_settings.setArrayIndex(i);
        items.append(textOfVariant(m_settings.value(QStringLiteral("value"))));
    }
    m_settings.endArray();
    return items;
}

int SettingsManager::version() const
{
    return intValue(QString::fromLatin1(kVersionKey), 0, 0, std::numeric_limits<int>::max());
}

bool SettingsManager::isHealthy() const
{
    return !m_couldNotCreateFolder && m_settings.status() == QSettings::NoError;
}

// ---- Writing --------------------------------------------------------------------------------

void SettingsManager::writeVersionIfMissing()
{
    const QString key = QString::fromLatin1(kVersionKey);
    if (!m_settings.contains(key)) {
        m_settings.setValue(key, QString::number(kCurrentVersion));
    }
}

void SettingsManager::commit()
{
    // QSettings does not promise to create missing folders, so do it here.
    const QString folder = QFileInfo(m_filePath).absolutePath();
    if (!QDir().mkpath(folder)) {
        m_couldNotCreateFolder = true;
    } else {
        m_couldNotCreateFolder = false;
        m_settings.sync();
    }

    if (!isHealthy() && !m_warned) {
        m_warned = true; // once is enough; the program carries on without saving
        qCWarning(lcSettings) << "Settings cannot be saved to" << m_filePath;
    }
}

void SettingsManager::setText(const QString& key, const QString& text)
{
    if (key.isEmpty()) {
        return;
    }
    if (m_settings.contains(key) && textOf(key) == text) {
        return; // already stored: do not touch the disk
    }

    writeVersionIfMissing();
    m_settings.setValue(key, text);
    commit();
}

void SettingsManager::setBool(const QString& key, bool value)
{
    setText(key, value ? QStringLiteral("true") : QStringLiteral("false"));
}

void SettingsManager::setInt(const QString& key, int value)
{
    setText(key, QString::number(value));
}

void SettingsManager::setString(const QString& key, const QString& value)
{
    setText(key, value);
}

void SettingsManager::setStringList(const QString& key, const QStringList& values)
{
    if (key.isEmpty()) {
        return;
    }
    if (stringList(key, std::numeric_limits<int>::max()) == values) {
        return; // already stored
    }

    writeVersionIfMissing();
    // Removing first drops the old "size" and every old entry, so a shorter list leaves nothing
    // from a longer one behind.
    m_settings.remove(key);
    if (!values.isEmpty()) {
        m_settings.beginWriteArray(key, static_cast<int>(values.size()));
        for (int i = 0; i < values.size(); ++i) {
            m_settings.setArrayIndex(i);
            m_settings.setValue(QStringLiteral("value"), values.at(i));
        }
        m_settings.endArray();
    }
    commit();
}

void SettingsManager::remove(const QString& key)
{
    if (key.isEmpty()) {
        return;
    }
    // QSettings removes a single value, or a whole group such as a list, with the same call.
    m_settings.remove(key);
    commit();
}

} // namespace scribe::core

#include "InMemoryBackend.h"

namespace scribe::core
{

qsizetype InMemoryBackend::length() const
{
    return m_text.size();
}

qsizetype InMemoryBackend::lineCount() const
{
    return m_text.count(u'\n') + 1;
}

QString InMemoryBackend::text() const
{
    return m_text;
}

QString InMemoryBackend::text(qsizetype pos, qsizetype count) const
{
    if (pos < 0 || count <= 0 || pos >= m_text.size()) {
        return {};
    }
    return m_text.mid(pos, count);
}

QString InMemoryBackend::line(qsizetype index) const
{
    if (index < 0) {
        return {};
    }

    qsizetype start = 0;
    for (qsizetype i = 0; i < index; ++i) {
        start = m_text.indexOf(u'\n', start);
        if (start < 0) {
            return {};
        }
        ++start; // first character after the line break
    }

    const qsizetype end = m_text.indexOf(u'\n', start);
    return m_text.mid(start, end < 0 ? -1 : end - start);
}

void InMemoryBackend::setText(const QString& text)
{
    m_text = text;
}

bool InMemoryBackend::insert(qsizetype pos, const QString& text)
{
    if (pos < 0 || pos > m_text.size()) {
        return false;
    }
    m_text.insert(pos, text);
    return true;
}

bool InMemoryBackend::remove(qsizetype pos, qsizetype count)
{
    if (pos < 0 || count < 0 || pos > m_text.size() || count > m_text.size() - pos) {
        return false;
    }
    m_text.remove(pos, count);
    return true;
}

} // namespace scribe::core

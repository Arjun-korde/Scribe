#include "Document.h"

#include "InMemoryBackend.h"

#include <QFile>
#include <QSaveFile>

namespace scribe::core
{

Document::Document()
    : Document(std::make_unique<InMemoryBackend>())
{}

Document::Document(std::unique_ptr<IDocumentBackend> backend)
    : m_backend(std::move(backend))
{}

void Document::setEncoding(const EncodingInfo& encoding)
{
    if (m_encoding != encoding) {
        m_encoding = encoding;
        m_modified = true;
    }
}

void Document::setLineEnding(LineEnding lineEnding)
{
    if (m_lineEnding != lineEnding) {
        m_lineEnding = lineEnding;
        m_modified = true;
    }
}

void Document::setText(const QString& text)
{
    m_backend->setText(convertLineEndings(text, LineEnding::Lf));
    m_modified = true;
}

bool Document::insert(qsizetype pos, const QString& text)
{
    if (!m_backend->insert(pos, convertLineEndings(text, LineEnding::Lf))) {
        return false;
    }
    m_modified = true;
    return true;
}

bool Document::remove(qsizetype pos, qsizetype count)
{
    if (!m_backend->remove(pos, count)) {
        return false;
    }
    m_modified = true;
    return true;
}

IoResult Document::load(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {DocumentError::CannotRead, file.errorString()};
    }
    const QByteArray bytes = file.readAll();
    if (file.error() != QFileDevice::NoError) {
        return {DocumentError::CannotRead, file.errorString()};
    }

    const EncodingInfo encoding = detectEncoding(bytes);
    const QString text = decode(bytes, encoding);

    // Decoding replaces bad input with U+FFFD, so saving would silently change the file. Refuse
    // to open anything that does not survive a decode/encode round trip byte for byte.
    if (encode(text, encoding) != bytes) {
        return {DocumentError::InvalidEncoding, {}};
    }

    m_lineEnding = detectLineEnding(text);
    m_backend->setText(convertLineEndings(text, LineEnding::Lf));
    m_encoding = encoding;
    m_filePath = path;
    m_modified = false;
    return {};
}

IoResult Document::save()
{
    if (m_filePath.isEmpty()) {
        return {DocumentError::CannotWrite, QStringLiteral("no file path")};
    }
    return saveAs(m_filePath);
}

IoResult Document::saveAs(const QString& path)
{
    const QString text = convertLineEndings(m_backend->text(), m_lineEnding);
    const std::optional<QByteArray> bytes = encode(text, m_encoding);
    if (!bytes) {
        return {DocumentError::CannotEncode, {}};
    }

    // QSaveFile writes a temporary file and renames it over the target on commit(), so an
    // interrupted or failed save never leaves a half-written file behind.
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return {DocumentError::CannotWrite, file.errorString()};
    }
    if (file.write(*bytes) != bytes->size() || !file.commit()) {
        return {DocumentError::CannotWrite, file.errorString()};
    }

    m_filePath = path;
    m_modified = false;
    return {};
}

} // namespace scribe::core

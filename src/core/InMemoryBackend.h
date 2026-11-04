#pragma once

#include "IDocumentBackend.h"

namespace scribe::core
{

/// Keeps the whole document in one QString. Simple and fast for normal files; very large files
/// need a different backend.
class InMemoryBackend final : public IDocumentBackend
{
public:
    qsizetype length() const override;
    qsizetype lineCount() const override;
    QString text() const override;
    QString text(qsizetype pos, qsizetype count) const override;
    QString line(qsizetype index) const override;
    void setText(const QString& text) override;
    bool insert(qsizetype pos, const QString& text) override;
    bool remove(qsizetype pos, qsizetype count) override;

private:
    QString m_text;
};

} // namespace scribe::core

#include "qhexmetadata.h"
#include "qhexcursor.h"

QHexMetadata::QHexMetadata(const QHexOptions* options, QObject *parent) : QObject(parent), m_options(options) { }

const QHexMetadataLine* QHexMetadata::find(qint64 line) const
{
    auto it = m_metadata.find(line);
    return it != m_metadata.end() ? std::addressof(it.value()) : nullptr;
}

QString QHexMetadata::getComment(qint64 line, qint64 column) const
{
    auto* metadataline = this->find(line);
    if(!metadataline) return QString();

    auto offset = QHexUtils::positionToOffset(m_options, {line, column});
    QStringList comments;

    for(auto& mi : *metadataline)
    {
        if((offset < mi.begin || offset > mi.end) || mi.comment.isEmpty()) continue;
        comments.push_back(mi.comment);
    }

    return comments.join("\n");
}

void QHexMetadata::removeMetadata(qint64 line)
{
    auto it = m_metadata.find(line);
    if(it == m_metadata.end()) return;

    m_metadata.erase(it);
    Q_EMIT changed();
}

void QHexMetadata::removeBackground(qint64 line)
{
    this->clearMetadata(line, [](QHexMetadataItem& mi) -> bool {
        if(!mi.background.isValid()) return false;

        if(mi.foreground.isValid() || !mi.comment.isEmpty()) {
            mi.background = QColor();
            return false;
        }

        return true;
    });
}

void QHexMetadata::removeForeground(qint64 line)
{
    this->clearMetadata(line, [](QHexMetadataItem& mi) -> bool {
        if(!mi.foreground.isValid()) return false;

        if(mi.background.isValid() || !mi.comment.isEmpty()) {
            mi.foreground = QColor();
            return false;
        }

        return true;
    });
}

void QHexMetadata::removeComments(qint64 line)
{
    this->clearMetadata(line, [](QHexMetadataItem& mi) -> bool {
        if(mi.comment.isEmpty()) return false;

        if(mi.foreground.isValid() || mi.background.isValid()) {
            mi.comment.clear();
            return false;
        }

        return true;
    });
}

void QHexMetadata::unhighlight(qint64 line)
{
    this->clearMetadata(line, [](QHexMetadataItem& mi) -> bool {
        if(!mi.foreground.isValid() && !mi.background.isValid()) return false;

        if(!mi.comment.isEmpty()) {
            mi.foreground = QColor();
            mi.background = QColor();
            return false;
        }

        return true;
    });
}

void QHexMetadata::clear() { m_metadata.clear(); Q_EMIT changed(); }
void QHexMetadata::copy(const QHexMetadata* metadata) { m_metadata = metadata->m_metadata; }

void QHexMetadata::clearMetadata(qint64 line, ClearMetadataCallback&& cb)
{
    auto iit = m_metadata.find(line);
    if(iit == m_metadata.end()) return;

    auto oldsize = iit->size();

    for(auto it = iit->begin(); it != iit->end(); )
    {
        if(cb(*it)) it = iit->erase(it);
        else it++;
    }

    if(iit->empty())
    {
        this->removeMetadata(line);
        return;
    }

    if(oldsize != iit->size()) Q_EMIT changed();
}

void QHexMetadata::setMetadata(const QHexMetadataItem& mi)
{
    if(!m_options->linelength) return;

    const qint64 firstline = mi.begin / m_options->linelength;
    const qint64 lastline = mi.end / m_options->linelength;
    bool notify = false;

    for(auto line = firstline; line <= lastline; line++)
    {
        auto start = line == firstline ? mi.begin % m_options->linelength : 0;
        auto length = line == lastline ? (mi.end % m_options->linelength) - start : m_options->linelength;
        if(length <= 0) continue;

        notify = true;
        m_metadata[line].push_back(mi);
    }

    if(notify) Q_EMIT changed();
}

void QHexMetadata::invalidate()
{
    auto oldmetadata = m_metadata;
    m_metadata.clear();

    for(const QHexMetadataLine& line : oldmetadata)
        for(const QHexMetadataItem& mi : line)
            this->setMetadata(mi);
}

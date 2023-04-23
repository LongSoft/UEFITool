#include "qhexdocument.h"
#include "buffer/qmemorybuffer.h"
#include "commands/insertcommand.h"
#include "commands/replacecommand.h"
#include "commands/removecommand.h"
#include "qhexutils.h"
#include <QBuffer>
#include <QFile>
#include <cmath>

QHexDocument::QHexDocument(QHexBuffer *buffer, QObject* parent): QObject(parent)
{
    m_buffer = buffer;
    m_buffer->setParent(this); // Take Ownership

    connect(&m_undostack, &QUndoStack::canUndoChanged, this, &QHexDocument::canUndoChanged);
    connect(&m_undostack, &QUndoStack::canRedoChanged, this, &QHexDocument::canRedoChanged);
}

qint64 QHexDocument::indexOf(const QByteArray& ba, qint64 from) { return m_buffer->indexOf(ba, from); }
qint64 QHexDocument::lastIndexOf(const QByteArray& ba, qint64 from) { return m_buffer->lastIndexOf(ba, from); }
bool QHexDocument::isEmpty() const { return m_buffer->isEmpty(); }
bool QHexDocument::canUndo() const { return m_undostack.canUndo(); }
bool QHexDocument::canRedo() const { return m_undostack.canRedo(); }

void QHexDocument::setData(const QByteArray& ba)
{
    QHexBuffer* mb = new QMemoryBuffer();
    mb->read(ba);
    this->setData(mb);
}

void QHexDocument::setData(QHexBuffer* buffer)
{
    if(!buffer) return;

    m_undostack.clear();
    buffer->setParent(this);

    auto* oldbuffer = m_buffer;
    m_buffer = buffer;
    if(oldbuffer) oldbuffer->deleteLater();

    Q_EMIT canUndoChanged(false);
    Q_EMIT canRedoChanged(false);
    Q_EMIT changed();
    Q_EMIT reset();
}

qint64 QHexDocument::length() const { return m_buffer ? m_buffer->length() : 0; }
uchar QHexDocument::at(int offset) const { return m_buffer->at(offset); }

void QHexDocument::undo() { m_undostack.undo(); Q_EMIT changed(); }
void QHexDocument::redo() { m_undostack.redo(); Q_EMIT changed(); }
void QHexDocument::insert(qint64 offset, uchar b) { this->insert(offset, QByteArray(1, b)); }
void QHexDocument::replace(qint64 offset, uchar b) { this->replace(offset, QByteArray(1, b)); }

void QHexDocument::insert(qint64 offset, const QByteArray &data)
{
    m_undostack.push(new InsertCommand(m_buffer, this, offset, data));

    Q_EMIT changed();
    Q_EMIT dataChanged(data, offset, ChangeReason::Insert);
}

void QHexDocument::replace(qint64 offset, const QByteArray &data)
{
    m_undostack.push(new ReplaceCommand(m_buffer, this, offset, data));
    Q_EMIT changed();
    Q_EMIT dataChanged(data, offset, ChangeReason::Replace);
}

void QHexDocument::remove(qint64 offset, int len)
{
    QByteArray data = m_buffer->read(offset, len);

    m_undostack.push(new RemoveCommand(m_buffer, this, offset, len));
    Q_EMIT changed();
    Q_EMIT dataChanged(data, offset, ChangeReason::Remove);
}

QByteArray QHexDocument::read(qint64 offset, int len) const { return m_buffer->read(offset, len); }

bool QHexDocument::saveTo(QIODevice *device)
{
    if(!device->isWritable()) return false;
    m_buffer->write(device);
    return true;
}

QHexDocument* QHexDocument::fromBuffer(QHexBuffer* buffer, QObject* parent) { return new QHexDocument(buffer, parent); }
QHexDocument* QHexDocument::create(QObject* parent) { return QHexDocument::fromMemory<QMemoryBuffer>({}, parent); }

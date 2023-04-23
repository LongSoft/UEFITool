#include "insertcommand.h"
#include "../qhexdocument.h"

InsertCommand::InsertCommand(QHexBuffer *buffer, QHexDocument* document, qint64 offset, const QByteArray &data, QUndoCommand *parent): HexCommand(buffer, document, parent)
{
    m_offset = offset;
    m_data = data;
}

void InsertCommand::undo()
{
    m_buffer->remove(m_offset, m_data.length());
    Q_EMIT m_hexdocument->dataChanged(m_data, m_offset, QHexDocument::ChangeReason::Remove);
}

void InsertCommand::redo() { m_buffer->insert(m_offset, m_data); }

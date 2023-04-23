#include "removecommand.h"
#include "../qhexdocument.h"

RemoveCommand::RemoveCommand(QHexBuffer *buffer, QHexDocument* document, qint64 offset, int length, QUndoCommand *parent): HexCommand(buffer, document, parent)
{
    m_offset = offset;
    m_length = length;
}

void RemoveCommand::undo()
{
    m_buffer->insert(m_offset, m_data);
    Q_EMIT m_hexdocument->dataChanged(m_data, m_offset, QHexDocument::ChangeReason::Insert);
}

void RemoveCommand::redo()
{
    m_data = m_buffer->read(m_offset, m_length); // Backup data
    m_buffer->remove(m_offset, m_length);
}

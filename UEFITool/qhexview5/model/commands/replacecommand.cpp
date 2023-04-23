#include "replacecommand.h"
#include "../qhexdocument.h"

ReplaceCommand::ReplaceCommand(QHexBuffer *buffer, QHexDocument* document, qint64 offset, const QByteArray &data, QUndoCommand *parent): HexCommand(buffer, document, parent)
{
    m_offset = offset;
    m_data = data;
}

void ReplaceCommand::undo()
{
    m_buffer->replace(m_offset, m_olddata);
    Q_EMIT m_hexdocument->dataChanged(m_olddata, m_offset, QHexDocument::ChangeReason::Replace);
}

void ReplaceCommand::redo()
{
    m_olddata = m_buffer->read(m_offset, m_data.length());
    m_buffer->replace(m_offset, m_data);
}

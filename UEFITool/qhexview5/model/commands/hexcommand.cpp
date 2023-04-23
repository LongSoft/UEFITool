#include "hexcommand.h"

HexCommand::HexCommand(QHexBuffer *buffer, QHexDocument* document, QUndoCommand *parent): QUndoCommand(parent), m_hexdocument(document), m_buffer(buffer), m_offset(0), m_length(0) { }

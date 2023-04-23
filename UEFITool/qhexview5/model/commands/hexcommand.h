#pragma once

#include <QUndoCommand>
#include "../buffer/qhexbuffer.h"

class QHexDocument;

class HexCommand: public QUndoCommand
{
    public:
        HexCommand(QHexBuffer* buffer, QHexDocument* document, QUndoCommand* parent = nullptr);

    protected:
        QHexDocument* m_hexdocument;
        QHexBuffer* m_buffer;
        qint64 m_offset;
        int m_length;
        QByteArray m_data;
};

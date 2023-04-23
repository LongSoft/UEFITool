#pragma once

#include "hexcommand.h"

class InsertCommand: public HexCommand
{
    public:
        InsertCommand(QHexBuffer* buffer, QHexDocument* document, qint64 offset, const QByteArray& data, QUndoCommand* parent = nullptr);
        void undo() override;
        void redo() override;
};

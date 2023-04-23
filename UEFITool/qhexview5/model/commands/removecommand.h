#pragma once

#include "hexcommand.h"

class RemoveCommand: public HexCommand
{
    public:
        RemoveCommand(QHexBuffer* buffer, QHexDocument* document, qint64 offset, int length, QUndoCommand* parent = nullptr);
        void undo() override;
        void redo() override;
};

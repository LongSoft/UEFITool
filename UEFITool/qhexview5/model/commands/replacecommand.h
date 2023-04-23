#pragma once

#include "hexcommand.h"

class ReplaceCommand: public HexCommand
{
    public:
        ReplaceCommand(QHexBuffer* buffer, QHexDocument* document, qint64 offset, const QByteArray& data, QUndoCommand* parent = nullptr);
        void undo() override;
        void redo() override;

    private:
        QByteArray m_olddata;
};

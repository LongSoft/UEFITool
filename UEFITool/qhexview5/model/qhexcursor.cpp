#include "../qhexview.h"
#include "qhexcursor.h"
#include "qhexdocument.h"

/*
 * https://stackoverflow.com/questions/10803043/inverse-column-row-major-order-transformation
 *
 *  If the index is calculated as:
 *      offset = row + column*NUMROWS
 *  then the inverse would be:
 *      row = offset % NUMROWS
 *      column = offset / NUMROWS
 *  where % is modulus, and / is integer division.
 */

QHexCursor::QHexCursor(const QHexOptions* options, QHexView* parent) : QObject(parent), m_options(options) { }
QHexView* QHexCursor::hexView() const { return qobject_cast<QHexView*>(this->parent()); }
QHexCursor::Mode QHexCursor::mode() const { return m_mode; }
qint64 QHexCursor::offset() const { return this->positionToOffset(m_position); }
qint64 QHexCursor::address() const { return m_options->baseaddress + this->offset(); }
quint64 QHexCursor::lineAddress() const { return m_options->baseaddress + (m_position.line * m_options->linelength); }
qint64 QHexCursor::selectionStartOffset() const { return this->positionToOffset(this->selectionStart()); }
qint64 QHexCursor::selectionEndOffset() const { return this->positionToOffset(this->selectionEnd()); }
qint64 QHexCursor::line() const { return m_position.line; }
qint64 QHexCursor::column() const { return m_position.column; }

QHexPosition QHexCursor::selectionStart() const
{
    if(m_position.line < m_selection.line)
        return m_position;

    if(m_position.line == m_selection.line)
    {
        if(m_position.column < m_selection.column)
            return m_position;
    }

    return m_selection;
}

QHexPosition QHexCursor::selectionEnd() const
{
    if(m_position.line > m_selection.line)
        return m_position;

    if(m_position.line == m_selection.line)
    {
        if(m_position.column > m_selection.column)
            return m_position;
    }

    return m_selection;
}

qint64 QHexCursor::selectionLength() const
{
    auto selstart = this->selectionStartOffset(), selend = this->selectionEndOffset();
    return selstart == selend ? 0 : selend - selstart + 1;
}

QHexPosition QHexCursor::position() const { return m_position; }
QByteArray QHexCursor::selectedBytes() const { return this->hexView()->selectedBytes(); }
bool QHexCursor::hasSelection() const { return m_position != m_selection; }

bool QHexCursor::isSelected(qint64 line, qint64 column) const
{
    if(!this->hasSelection()) return false;

    auto selstart = this->selectionStart(), selend = this->selectionEnd();
    if(line > selstart.line && line < selend.line) return true;
    if(line == selstart.line && line == selend.line) return column >= selstart.column && column <= selend.column;
    if(line == selstart.line) return column >= selstart.column;
    if(line == selend.line) return column <= selend.column;
    return false;
}

void QHexCursor::setMode(Mode m)
{
    if(m_mode == m) return;
    m_mode = m;
    Q_EMIT modeChanged();
}

void QHexCursor::switchMode()
{
    switch(m_mode)
    {
        case Mode::Insert: this->setMode(Mode::Overwrite); break;
        case Mode::Overwrite: this->setMode(Mode::Insert); break;
    }
}

void QHexCursor::move(qint64 offset) { this->move(this->offsetToPosition(offset)); }
void QHexCursor::move(qint64 line, qint64 column) { return this->move({line, column}); }

void QHexCursor::move(QHexPosition pos)
{
    if(pos.line >= 0) m_selection.line = pos.line;
    if(pos.column >= 0) m_selection.column = pos.column;
    this->select(pos);
}

void QHexCursor::select(qint64 offset) { this->select(this->offsetToPosition(offset)); }
void QHexCursor::select(qint64 line, qint64 column) { this->select({line, column}); }

void QHexCursor::select(QHexPosition pos)
{
    if(pos.line >= 0) m_position.line = pos.line;
    if(pos.column >= 0) m_position.column = pos.column;
    Q_EMIT positionChanged();
}

void QHexCursor::selectSize(qint64 length)
{
    if(length > 0) length--;
    else if(length < 0) length++;
    if(length) this->select(this->offset() + length);
}

qint64 QHexCursor::replace(const QVariant& oldvalue, const QVariant& newvalue, qint64 offset, QHexFindMode mode, unsigned int options, QHexFindDirection fd) const { return this->hexView()->replace(oldvalue, newvalue, offset, mode, options, fd); }
qint64 QHexCursor::find(const QVariant& value, qint64 offset, QHexFindMode mode, unsigned int options, QHexFindDirection fd) const { return this->hexView()->find(value, offset, mode, options, fd); }
void QHexCursor::cut(bool hex) { this->hexView()->cut(hex); }
void QHexCursor::copy(bool hex) const { this->hexView()->copy(hex); }
void QHexCursor::paste(bool hex) { this->hexView()->paste(hex); }
void QHexCursor::selectAll() { this->hexView()->selectAll(); }
void QHexCursor::removeSelection() { this->hexView()->removeSelection(); }
void QHexCursor::clearSelection() { m_position = m_selection; Q_EMIT positionChanged(); }
qint64 QHexCursor::positionToOffset(QHexPosition pos) const { return QHexUtils::positionToOffset(m_options, pos); }
QHexPosition QHexCursor::offsetToPosition(qint64 offset) const { return QHexUtils::offsetToPosition(m_options, offset); }

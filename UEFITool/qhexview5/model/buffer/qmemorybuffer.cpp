#include "qmemorybuffer.h"
#include <QIODevice>

QMemoryBuffer::QMemoryBuffer(QObject *parent) : QHexBuffer{parent} { }
uchar QMemoryBuffer::at(qint64 idx) { return static_cast<uchar>(m_buffer.at(idx)); }
qint64 QMemoryBuffer::length() const { return static_cast<qint64>(m_buffer.length()); }
void QMemoryBuffer::insert(qint64 offset, const QByteArray &data) { m_buffer.insert(static_cast<int>(offset), data); }
void QMemoryBuffer::remove(qint64 offset, int length) { m_buffer.remove(static_cast<int>(offset), length); }
QByteArray QMemoryBuffer::read(qint64 offset, int length) { return m_buffer.mid(static_cast<int>(offset), length); }

bool QMemoryBuffer::read(QIODevice *device)
{
    m_buffer = device->readAll();
    return true;
}

void QMemoryBuffer::write(QIODevice *device) { device->write(m_buffer); }
qint64 QMemoryBuffer::indexOf(const QByteArray& ba, qint64 from) { return m_buffer.indexOf(ba, static_cast<int>(from)); }
qint64 QMemoryBuffer::lastIndexOf(const QByteArray& ba, qint64 from) { return m_buffer.lastIndexOf(ba, static_cast<int>(from)); }

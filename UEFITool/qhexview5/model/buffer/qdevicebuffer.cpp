#include "qdevicebuffer.h"
#include <QIODevice>
#include <limits>

QDeviceBuffer::QDeviceBuffer(QObject *parent) : QHexBuffer{parent} { }

QDeviceBuffer::~QDeviceBuffer()
{
    if(!m_device) return;

    if(m_device->parent() == this)
    {
        if(m_device->isOpen()) m_device->close();
        m_device->deleteLater();
    }

    m_device = nullptr;
}

uchar QDeviceBuffer::at(qint64 idx)
{
    m_device->seek(idx);

    char c = '\0';
    m_device->getChar(&c);
    return static_cast<uchar>(c);
}

qint64 QDeviceBuffer::length() const { return m_device->size(); }

void QDeviceBuffer::insert(qint64 offset, const QByteArray &data)
{
    Q_UNUSED(offset)
    Q_UNUSED(data)
    // Not implemented
}

void QDeviceBuffer::replace(qint64 offset, const QByteArray& data)
{
    m_device->seek(offset);
    m_device->write(data);
}

void QDeviceBuffer::remove(qint64 offset, int length)
{
    Q_UNUSED(offset)
    Q_UNUSED(length)
    // Not implemented
}

QByteArray QDeviceBuffer::read(qint64 offset, int length)
{
    m_device->seek(offset);
    return m_device->read(length);
}

bool QDeviceBuffer::read(QIODevice *device)
{
    m_device = device;
    if(!m_device) return false;
    if(!m_device->isOpen()) m_device->open(QIODevice::ReadWrite);
    return m_device->isOpen();
}

void QDeviceBuffer::write(QIODevice *device)
{
    Q_UNUSED(device)
    // Not implemented
}

qint64 QDeviceBuffer::indexOf(const QByteArray& ba, qint64 from)
{
    const auto MAX = std::numeric_limits<int>::max();
    qint64 idx = -1;

    if(from < m_device->size())
    {
        idx = from;
        m_device->seek(from);

        while(idx < m_device->size())
        {
            QByteArray data = m_device->read(MAX);
            int sidx = data.indexOf(ba);

            if(sidx >= 0)
            {
                idx += sidx;
                break;
            }

            if(idx + data.size() >= m_device->size()) return -1;
            m_device->seek(m_device->pos() + data.size() - ba.size());
        }
    }

    return idx;
}

qint64 QDeviceBuffer::lastIndexOf(const QByteArray& ba, qint64 from)
{
    const auto MAX = std::numeric_limits<int>::max();
    qint64 idx = -1;

    if(from >= 0 && ba.size() < MAX)
    {
        qint64 currpos = from;

        while(currpos >= 0)
        {
            qint64 readpos = (currpos < MAX) ? 0 : currpos - MAX;
            m_device->seek(readpos);

            QByteArray data = m_device->read(currpos - readpos);
            int lidx = data.lastIndexOf(ba, from);

            if(lidx >= 0)
            {
                idx = readpos + lidx;
                break;
            }

            if(readpos <= 0) break;
            currpos = readpos + ba.size();
        }

    }

    return idx;
}

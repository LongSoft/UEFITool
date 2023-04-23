#include "qhexbuffer.h"
#include <QBuffer>

QHexBuffer::QHexBuffer(QObject *parent) : QObject{parent} { }
uchar QHexBuffer::at(qint64 idx) { return this->read(idx, 1).at(0); }
bool QHexBuffer::isEmpty() const { return this->length() <= 0; }

void QHexBuffer::replace(qint64 offset, const QByteArray &data)
{
    this->remove(offset, data.length());
    this->insert(offset, data);
}

void QHexBuffer::read(char *data, int size)
{
    QBuffer* buffer = new QBuffer(this);
    buffer->setData(data, size);

    if(!buffer->isOpen())
        buffer->open(QBuffer::ReadWrite);

    this->read(buffer);
}

void QHexBuffer::read(const QByteArray &ba)
{
    QBuffer* buffer = new QBuffer(this);

    buffer->setData(ba);
    if(!buffer->isOpen())
        buffer->open(QBuffer::ReadWrite);

    this->read(buffer);
}


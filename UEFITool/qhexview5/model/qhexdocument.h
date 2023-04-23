#pragma once

#include <QUndoStack>
#include "buffer/qhexbuffer.h"
#include "qhexmetadata.h"
#include "qhexoptions.h"

class QHexCursor;

class QHexDocument: public QObject
{
    Q_OBJECT

    public:
        enum class ChangeReason { Insert, Remove, Replace };
        enum class FindDirection { Forward, Backward };
        Q_ENUM(ChangeReason);
        Q_ENUM(FindDirection);

    private:
        explicit QHexDocument(QHexBuffer* buffer, QObject *parent = nullptr);

    public:
        bool isEmpty() const;
        bool canUndo() const;
        bool canRedo() const;
        void setData(const QByteArray& ba);
        void setData(QHexBuffer* buffer);
        qint64 length() const;
        qint64 indexOf(const QByteArray& ba, qint64 from = 0);
        qint64 lastIndexOf(const QByteArray& ba, qint64 from = 0);
        QByteArray read(qint64 offset, int len = 0) const;
        uchar at(int offset) const;

    public Q_SLOTS:
        void undo();
        void redo();
        void insert(qint64 offset, uchar b);
        void replace(qint64 offset, uchar b);
        void insert(qint64 offset, const QByteArray& data);
        void replace(qint64 offset, const QByteArray& data);
        void remove(qint64 offset, int len);
        bool saveTo(QIODevice* device);

    public:
        template<typename T, bool Owned = true> static QHexDocument* fromDevice(QIODevice* iodevice, QObject* parent = nullptr);
        template<typename T> static QHexDocument* fromMemory(char *data, int size, QObject* parent = nullptr);
        template<typename T> static QHexDocument* fromMemory(const QByteArray& ba, QObject* parent = nullptr);
        static QHexDocument* fromBuffer(QHexBuffer* buffer, QObject* parent = nullptr);
        static QHexDocument* create(QObject* parent = nullptr);

    Q_SIGNALS:
        void canUndoChanged(bool canundo);
        void canRedoChanged(bool canredo);
        void dataChanged(const QByteArray& data, quint64 offset, QHexDocument::ChangeReason reason);
        void changed();
        void reset();

    private:
        QHexBuffer* m_buffer;
        QUndoStack m_undostack;

    friend class QHexView;
};

template<typename T, bool Owned>
QHexDocument* QHexDocument::fromDevice(QIODevice* iodevice, QObject *parent)
{
    QHexBuffer* hexbuffer = new T(parent);
    if(Owned) iodevice->setParent(hexbuffer);
    return hexbuffer->read(iodevice) ? new QHexDocument(hexbuffer, parent) : nullptr;
}

template<typename T>
QHexDocument* QHexDocument::fromMemory(char *data, int size, QObject *parent)
{
    QHexBuffer* hexbuffer = new T();
    hexbuffer->read(data, size);
    return new QHexDocument(hexbuffer, parent);
}

template<typename T>
QHexDocument* QHexDocument::fromMemory(const QByteArray& ba, QObject *parent)
{
    QHexBuffer* hexbuffer = new T();
    hexbuffer->read(ba);
    return new QHexDocument(hexbuffer, parent);
}

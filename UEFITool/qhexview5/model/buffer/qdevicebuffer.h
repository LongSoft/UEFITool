#pragma once

#include "qhexbuffer.h"

class QDeviceBuffer : public QHexBuffer
{
    Q_OBJECT

    public:
        explicit QDeviceBuffer(QObject *parent = nullptr);
        virtual ~QDeviceBuffer();
        uchar at(qint64 idx) override;
        qint64 length() const override;
        void insert(qint64 offset, const QByteArray& data) override;
        void replace(qint64 offset, const QByteArray& data) override;
        void remove(qint64 offset, int length) override;
        QByteArray read(qint64 offset, int length) override;
        bool read(QIODevice* device) override;
        void write(QIODevice* device) override;
        qint64 indexOf(const QByteArray& ba, qint64 from) override;
        qint64 lastIndexOf(const QByteArray& ba, qint64 from) override;

    protected:
        QIODevice* m_device{nullptr};
};

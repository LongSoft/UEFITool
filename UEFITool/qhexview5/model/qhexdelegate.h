#pragma once

#include <QTextCharFormat>
#include <QObject>
#include "qhexutils.h"

class QHexView;

class QHexDelegate: public QObject
{
    Q_OBJECT

    public:
        explicit QHexDelegate(QObject* parent = nullptr);
        virtual ~QHexDelegate() = default;
        virtual QString addressHeader(const QHexView* hexview) const;
        virtual QString hexHeader(const QHexView* hexview) const;
        virtual QString asciiHeader(const QHexView* hexview) const;
        virtual void renderAddress(quint64 address, QTextCharFormat& cf, const QHexView* hexview) const;
        virtual void renderHeader(QTextBlockFormat& bf, const QHexView* hexview) const;
        virtual void renderHeaderPart(const QString& s, QHexArea area, QTextCharFormat& cf, const QHexView* hexview) const;
        virtual bool render(quint64 offset, quint8 b, QTextCharFormat& outcf, const QHexView* hexview) const;
        virtual bool paintSeparator(QPainter* painter, QLineF line, const QHexView* hexview) const;
        virtual void paint(QPainter* painter, const QHexView* hexview) const;
};

#include "../qhexview.h"
#include "qhexdelegate.h"

QHexDelegate::QHexDelegate(QObject* parent): QObject{parent} { }

QString QHexDelegate::addressHeader(const QHexView* hexview) const
{
    Q_UNUSED(hexview);
    return QString();
}

QString QHexDelegate::hexHeader(const QHexView* hexview) const
{
    Q_UNUSED(hexview);
    return QString();
}

QString QHexDelegate::asciiHeader(const QHexView* hexview) const
{
    Q_UNUSED(hexview);
    return QString();
}

void QHexDelegate::renderAddress(quint64 address, QTextCharFormat& cf, const QHexView* hexview) const
{
    Q_UNUSED(address);
    Q_UNUSED(hexview);
    Q_UNUSED(cf);
    Q_UNUSED(hexview);
}

void QHexDelegate::renderHeader(QTextBlockFormat& bf, const QHexView* hexview) const
{
    Q_UNUSED(bf);
    Q_UNUSED(hexview);
}

void QHexDelegate::renderHeaderPart(const QString& s, QHexArea area, QTextCharFormat& cf, const QHexView* hexview) const
{
    Q_UNUSED(s);
    Q_UNUSED(area);
    Q_UNUSED(cf);
    Q_UNUSED(hexview);
}

bool QHexDelegate::render(quint64 offset, quint8 b, QTextCharFormat& outcf, const QHexView* hexview) const
{
    Q_UNUSED(offset);
    Q_UNUSED(b);
    Q_UNUSED(outcf);
    Q_UNUSED(hexview);

    return false;
}

bool QHexDelegate::paintSeparator(QPainter* painter, QLineF line, const QHexView* hexview) const
{
    Q_UNUSED(painter);
    Q_UNUSED(line);
    Q_UNUSED(hexview);
    return false;
}

void QHexDelegate::paint(QPainter* painter, const QHexView* hexview) const
{
    Q_UNUSED(hexview);
    hexview->paint(painter);
}

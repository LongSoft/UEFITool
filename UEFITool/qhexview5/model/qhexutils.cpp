#include "qhexutils.h"
#include "qhexoptions.h"
#include "../qhexview.h"
#include <QGlobalStatic>
#include <QDataStream>
#include <QtEndian>
#include <QList>
#include <QHash>
#include <limits>
#include <cctype>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    #define QHEXVIEW_VARIANT_EQ(x, t) ((x).metaType().id() == QMetaType::Q##t)
#else
    #define QHEXVIEW_VARIANT_EQ(x, t) ((x).type() == QVariant::t)
#endif

namespace QHexUtils {

Q_GLOBAL_STATIC_WITH_ARGS(QList<char>, HEXMAP, ({
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
}));

namespace PatternUtils {

Q_GLOBAL_STATIC_WITH_ARGS(QString, WILDCARD_BYTE, ("??"))

bool check(QString& p, qint64& len)
{
    static QHash<QString, QPair<QString, size_t>> processed; // Cache processed patterns

    auto it = processed.find(p);

    if(it != processed.end())
    {
        p = it.value().first;
        len = it.value().second;
        return true;
    }

    QString op = p; // Store unprocessed pattern
    p = p.simplified().replace(" ", "");
    if(p.isEmpty() || (p.size() % 2)) return false;

    int wccount = 0;

    for(auto i = 0; i < p.size() - 2; i += 2)
    {
        const auto& hexb = p.mid(i, 2);

        if(hexb == *WILDCARD_BYTE)
        {
            wccount++;
            continue;
        }

        if(!isxdigit(hexb.at(0).toLatin1()) || !isxdigit(hexb.at(1).toLatin1()))
            return false;
    }

    if(wccount >= p.size()) return false;
    len = p.size() / 2;
    processed[op] = qMakePair(p, len); // Cache processed pattern
    return true;
}

bool match(const QByteArray& data, const QString& pattern)
{
    for(qint64 i = 0, idx = 0; (i <= (pattern.size() - 2)); i += 2, idx++)
    {
        if(idx >= data.size()) return false;

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        QStringView hexb = QStringView{pattern}.mid(i, 2);
#else
        const QStringRef& hexb = pattern.midRef(i, 2);
#endif

        if(hexb == *WILDCARD_BYTE) continue;

        bool ok = false;
        auto b = static_cast<char>(hexb.toUInt(&ok, 16));
        if(!ok || (b != data.at(idx))) return false;
    }

    return true;
}

}

namespace {

unsigned int countBits(quint64 val)
{
    if(val <= std::numeric_limits<quint8>::max()) return QHexFindOptions::Int8;
    if(val <= std::numeric_limits<quint16>::max()) return QHexFindOptions::Int16;
    if(val <= std::numeric_limits<quint32>::max()) return QHexFindOptions::Int32;

    return QHexFindOptions::Int64;
}

template<typename Function>
qint64 findIter(qint64 startoffset, QHexFindDirection fd, const QHexView* hexview, Function&& f)
{
    QHexDocument* hexdocument = hexview->hexDocument();
    qint64 offset = -1;

    QHexFindDirection cfd = fd;
    if(cfd == QHexFindDirection::All) cfd = QHexFindDirection::Forward;

    qint64 i = startoffset;

    while(offset == -1 && (cfd == QHexFindDirection::Backward ? (i >= 0) : (i < hexdocument->length())))
    {
        if(!f(i, offset)) break;

        if(cfd == QHexFindDirection::Backward) i--;
        else i++;

        if(fd == QHexFindDirection::All && i >= hexdocument->length()) i = 0;
    }

    return offset;
}

qint64 findDefault(const QByteArray& value, qint64 startoffset, const QHexView* hexview, unsigned int options, QHexFindDirection fd)
{
    QHexDocument* hexdocument = hexview->hexDocument();
    if(value.size() > hexdocument->length()) return -1;

    return findIter(startoffset, fd, hexview, [options, value, hexdocument](qint64 idx, qint64& offset) -> bool {
        for(auto i = 0; i < value.size(); i++) {
            qint64 curroffset = idx + i;

            if(curroffset >= hexdocument->length()) {
                offset = -1;
                return false;
            }

            uchar ch1 = hexdocument->at(curroffset);
            uchar ch2 = value.at(i);

            if(!(options & QHexFindOptions::CaseSensitive)) {
                ch1 = std::tolower(ch1);
                ch2 = std::tolower(ch2);
            }

            if(ch1 != ch2) break;
            if(i == value.size() - 1) offset = idx;
        }

        return true;
    });
}

qint64 findWildcard(QString pattern, qint64 startoffset, const QHexView* hexview, QHexFindDirection fd, qint64& patternlen)
{
    QHexDocument* hexdocument = hexview->hexDocument();
    if(!PatternUtils::check(pattern, patternlen) || (patternlen >= hexdocument->length())) return -1;

    return findIter(startoffset, fd, hexview, [hexdocument, pattern, patternlen](qint64 idx, qint64& offset) -> bool {
        if(PatternUtils::match(hexdocument->read(idx, patternlen), pattern)) offset = idx;
        return true;
    });
}

QByteArray variantToByteArray(QVariant value, QHexFindMode mode, unsigned int options)
{
    QByteArray v;

    switch(mode)
    {
        case QHexFindMode::Text:
            if(QHEXVIEW_VARIANT_EQ(value, String)) v = value.toString().toUtf8();
            else if(QHEXVIEW_VARIANT_EQ(value, ByteArray)) v = value.toByteArray();
            break;

        case QHexFindMode::Hex: {
            if(QHEXVIEW_VARIANT_EQ(value, String)) {
                qint64 len = 0;
                auto s = value.toString();
                if(!PatternUtils::check(s, len)) return { };

                bool ok = true;

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                for(auto i = 0; ok && i < s.size(); i += 2) v.push_back(static_cast<char>(QStringView{s}.mid(i, 2).toUInt(&ok, 16)));
#else
                for(auto i = 0; ok && i < s.size(); i += 2) v.push_back(static_cast<char>(s.midRef(i, 2).toUInt(&ok, 16)));
#endif

                if(!ok) return { };
            }
            else if(QHEXVIEW_VARIANT_EQ(value, ByteArray)) v = value.toByteArray();
            break;
        }

        case QHexFindMode::Int: {
            bool ok = false;
            uint val = value.toUInt(&ok);
            if(!ok) return QByteArray{ };

            QDataStream ds(&v, QIODevice::WriteOnly);

            if(options & QHexFindOptions::BigEndian) {
                if(options & QHexFindOptions::Int8) ds << qToBigEndian<quint8>(val);
                else if(options & QHexFindOptions::Int16) ds << qToBigEndian<quint16>(val);
                else if(options & QHexFindOptions::Int32) ds << qToBigEndian<quint32>(val);
                else if(options & QHexFindOptions::Int64) ds << qToBigEndian<quint64>(val);
                else return variantToByteArray(value, mode, options | countBits(val));
            }
            else {
                if(options & QHexFindOptions::Int8) ds << static_cast<quint8>(val);
                else if(options & QHexFindOptions::Int16) ds << static_cast<quint16>(val);
                else if(options & QHexFindOptions::Int32) ds << static_cast<quint32>(val);
                else if(options & QHexFindOptions::Int64) ds << static_cast<quint64>(val);
                else return variantToByteArray(value, mode, options | countBits(val));
            }

            break;
        }

        case QHexFindMode::Float: {
            bool ok = false;
            QDataStream ds(&v, QIODevice::WriteOnly);
            if(options & QHexFindOptions::Float) ds << value.toFloat(&ok);
            else if(options & QHexFindOptions::Double) ds << value.toDouble(&ok);
            if(!ok) return { };
        }

        default: break;
    }

    return v;
}

} // namespace

QByteArray toHex(const QByteArray& ba, char sep)
{
    QByteArray hex(sep ? (ba.size() * 3 - 1) : (ba.size() * 2), Qt::Uninitialized);

    for(auto i = 0, o = 0; i < ba.size(); i++)
    {
        if(sep && i) hex[o++] = static_cast<uchar>(sep);
        hex[o++] = HEXMAP->at((ba.at(i) & 0xf0) >> 4);
        hex[o++] = HEXMAP->at(ba.at(i) & 0x0f);
    }

    return hex;
}

QByteArray toHex(const QByteArray& ba) { return QHexUtils::toHex(ba, '\0'); }
qint64 positionToOffset(const QHexOptions* options, QHexPosition pos) { return options->linelength * pos.line + pos.column; }
QHexPosition offsetToPosition(const QHexOptions* options, qint64 offset) { return { offset / options->linelength, offset % options->linelength }; }

QPair<qint64, qint64> find(const QHexView* hexview, QVariant value, qint64 startoffset, QHexFindMode mode, unsigned int options, QHexFindDirection fd)
{
    qint64 offset, size = 0;
    if(startoffset == -1) startoffset = static_cast<qint64>(hexview->offset());

    if(mode == QHexFindMode::Hex && QHEXVIEW_VARIANT_EQ(value, String))
    {
        offset = QHexUtils::findWildcard(value.toString(), startoffset, hexview, fd, size);
    }
    else
    {
        auto ba = variantToByteArray(value, mode, options);

        if(!ba.isEmpty())
        {
            offset = QHexUtils::findDefault(ba, startoffset, hexview, options, fd);
            size = ba.size();
        }
        else
            offset = -1;
    }

    return {offset, offset > -1 ? size : 0};
}

bool checkPattern(QString pattern)
{
    qint64 len = 0;
    return PatternUtils::check(pattern, len);
}

QPair<qint64, qint64> replace(const QHexView* hexview, QVariant oldvalue, QVariant newvalue, qint64 startoffset, QHexFindMode mode, unsigned int options, QHexFindDirection fd)
{
    auto res = QHexUtils::find(hexview, oldvalue, startoffset, mode, options, fd);

    if(res.first != -1 && res.second > 0)
    {
        QHexDocument* hexdocument = hexview->hexDocument();
        auto ba = variantToByteArray(newvalue, mode, options);

        if(!ba.isEmpty())
        {
            hexdocument->remove(res.first, res.second);
            hexdocument->insert(res.first, ba);
            res.second = ba.size();
        }
        else
        {
            res.first = -1;
            res.second = 0;
        }
    }

    return res;
}

}

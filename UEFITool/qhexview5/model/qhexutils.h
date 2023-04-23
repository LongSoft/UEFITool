#pragma once

#include <QByteArray>
#include <QVariant>
#include <QString>
#include <QPair>

struct QHexOptions;
class QHexView;

namespace QHexFindOptions {
    enum: unsigned int {
        None          = (1 << 0),
        CaseSensitive = (1 << 1),
        Int8          = (1 << 2),
        Int16         = (1 << 3),
        Int32         = (1 << 4),
        Int64         = (1 << 5),
        Float         = (1 << 6),
        Double        = (1 << 7),

        BigEndian     = (1 << 11),
    };
}

enum class QHexFindMode { Text, Hex, Int, Float };
enum class QHexFindDirection { All, Forward, Backward };
enum class QHexArea { Header, Address, Hex, Ascii, Extra };

struct QHexPosition {
    qint64 line; qint64 column;
    static inline QHexPosition invalid() { return {-1, -1}; }
    inline bool isValid() const { return line >= 0 && column >= 0; }
    inline bool operator==(const QHexPosition& rhs) const { return (line == rhs.line) && (column == rhs.column); }
    inline bool operator!=(const QHexPosition& rhs) const { return (line != rhs.line) || (column != rhs.column); }
};

namespace QHexUtils {

QByteArray toHex(const QByteArray& ba, char sep);
QByteArray toHex(const QByteArray& ba);
qint64 positionToOffset(const QHexOptions* options, QHexPosition pos);
QPair<qint64, qint64> find(const QHexView* hexview, QVariant value, qint64 startoffset = 0, QHexFindMode mode = QHexFindMode::Text, unsigned int options = QHexFindOptions::None, QHexFindDirection fd = QHexFindDirection::Forward);
QPair<qint64, qint64> replace(const QHexView* hexview, QVariant oldvalue, QVariant newvalue, qint64 startoffset = 0, QHexFindMode mode = QHexFindMode::Text, unsigned int options = QHexFindOptions::None, QHexFindDirection fd = QHexFindDirection::Forward);
QHexPosition offsetToPosition(const QHexOptions* options, qint64 offset);
bool checkPattern(QString pattern);

}

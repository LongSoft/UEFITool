#pragma once

#include <QHash>
#include <QColor>
#include <QChar>

namespace QHexFlags {
    enum: unsigned int {
        None             = (1 << 0),
        HSeparator       = (1 << 1),
        VSeparator       = (1 << 2),
        StyledHeader     = (1 << 3),
        StyledAddress    = (1 << 4),
        NoHeader         = (1 << 5),
        HighlightAddress = (1 << 6),
        HighlightColumn  = (1 << 7),

        Separators = HSeparator | VSeparator,
        Styled     = StyledHeader | StyledAddress,
    };
}

struct QHexColor
{
    QColor foreground;
    QColor background;
};

struct QHexOptions
{
    // Appearance
    QChar unprintablechar{'.'};
    QString addresslabel{""};
    QString hexlabel;
    QString asciilabel;
    quint64 baseaddress{0};
    unsigned int flags{QHexFlags::None};
    unsigned int linelength{0x10};
    unsigned int addresswidth{0};
    unsigned int grouplength{1};
    unsigned int scrollsteps{1};

    // Colors & Styles
    QHash<quint8, QHexColor> bytecolors;
    QColor linealternatebackground;
    QColor linebackground;
    QColor headercolor;
    QColor commentcolor;
    QColor separatorcolor;

    // Misc
    bool copybreak{true};

    inline bool hasFlag(unsigned int flag) const { return flags & flag; }
};

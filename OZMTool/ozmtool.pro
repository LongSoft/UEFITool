QMAKE_CFLAGS_RELEASE *= -O1

QT       += core
QT       += xml
QT       -= gui

TARGET    = OZMTool
TEMPLATE  = app
CONFIG   += console
CONFIG += static
CONFIG -= app_bundle
DEFINES  += _CONSOLE

INCLUDEPATH += distorm/include

SOURCES  += ozmtool_main.cpp \
 ozmtool.cpp \
 qtplist/PListParser.cpp \
 distorm/decoder.c \
 distorm/distorm.c \
 distorm/instructions.c \
 distorm/insts.c \
 distorm/mnemonics.c \
 distorm/operands.c \
 distorm/prefix.c \
 distorm/textdefs.c \
 distorm/wstring.c \
 ../types.cpp \
 ../descriptor.cpp \
 ../ffs.cpp \
 ../ffsengine.cpp \
 ../treeitem.cpp \
 ../treemodel.cpp \
 ../LZMA/LzmaCompress.c \
 ../LZMA/LzmaDecompress.c \
 ../LZMA/SDK/C/LzFind.c \
 ../LZMA/SDK/C/LzmaDec.c \
 ../LZMA/SDK/C/LzmaEnc.c \
 ../Tiano/EfiTianoDecompress.c \
 ../Tiano/EfiTianoCompress.c \
    util.cpp \
    ffsutil.cpp


HEADERS  += ozmtool.h \
   qtplist/PListParser.h \
   distorm/decoder.h \
   distorm/config.h \
 ../basetypes.h \
 ../descriptor.h \
 ../gbe.h \
 ../me.h \
 ../ffs.h \
 ../peimage.h \
 ../types.h \
 ../ffsengine.h \
 ../treeitem.h \
 ../treemodel.h \
 ../peimage.h \
 ../LZMA/LzmaCompress.h \
 ../LZMA/LzmaDecompress.h \
 ../Tiano/EfiTianoDecompress.h \
 ../Tiano/EfiTianoCompress.h \
    util.h \
    ffsutil.h \
    common.h \
    version.h

OTHER_FILES += \
    README \
    build_macos.sh

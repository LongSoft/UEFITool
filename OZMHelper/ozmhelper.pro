QT       += core
QT       -= gui

TARGET    = OZMHelper
TEMPLATE  = app
CONFIG   += console
DEFINES  += _CONSOLE
CONFIG   += c++11
# QMAKE_CXXFLAGS += -std=c++11 -stdlib=libc++ -mmacosx-version-min=10.7
# LIBS += -stdlib=libc++ -mmacosx-version-min=10.7 #For MAC


INCLUDEPATH += ffs/Common
INCLUDEPATH += ffs/Include
INCLUDEPATH += ffs/Include/X64

SOURCES  += ozmhelper_main.cpp \
 ozmhelper.cpp \
 ffs/Common/EfiUtilityMsgs.c \
 ffs/Common/ParseInf.c \
 ffs/Common/CommonLib.c \
 ffs/Common/Crc32.c \
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
    wrapper.cpp \
    ffs/kextconvert.cpp

HEADERS  += ozmhelper.h \
   ffs/Common/CommonLib.h \
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
 ../LZMA/LzmaCompress.h \
 ../LZMA/LzmaDecompress.h \
 ../Tiano/EfiTianoDecompress.h \
 ../Tiano/EfiTianoCompress.h \
    wrapper.h \
    ffs/kextconvert.h
 

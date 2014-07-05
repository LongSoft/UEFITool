QMAKE_CFLAGS_RELEASE *= -O1

QT       += core
QT       += xml
QT       -= gui

TARGET    = OZMTool
TEMPLATE  = app
CONFIG   += console
CONFIG -= app_bundle
DEFINES  += _CONSOLE

INCLUDEPATH += include
INCLUDEPATH += dsdt2bios/capstone/include

SOURCES  += ozmtool_main.cpp \
 ozmtool.cpp \
 qtplist/PListParser.cpp \
 qtplist/PListSerializer.cpp \
 dsdt2bios/capstone/cs.c \
 dsdt2bios/capstone/MCInst.c \
 dsdt2bios/capstone/SStream.c \
 dsdt2bios/capstone/arch/X86/X86Module.c \
 dsdt2bios/capstone/arch/X86/X86IntelInstPrinter.c \
 dsdt2bios/capstone/arch/X86/X86ATTInstPrinter.c \
 dsdt2bios/capstone/arch/X86/X86Disassembler.c \
 dsdt2bios/capstone/arch/X86/X86DisassemblerDecoder.c \
 dsdt2bios/capstone/arch/X86/X86Mapping.c \
 dsdt2bios/capstone/utils.c \
 dsdt2bios/Dsdt2Bios.cpp \
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
   qtplist/PListSerializer.h \
   dsdt2bios/Dsdt2Bios.h \
   dsdt2bios/PeImage.h \
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
    util.h \
    ffsutil.h \
    common.h

OTHER_FILES += \
    README

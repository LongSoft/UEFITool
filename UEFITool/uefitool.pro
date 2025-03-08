QT += core gui widgets

TARGET = UEFITool
TEMPLATE = app

CONFIG += c++11

DEFINES += "U_ENABLE_FIT_PARSING_SUPPORT"
DEFINES += "U_ENABLE_NVRAM_PARSING_SUPPORT"
DEFINES += "U_ENABLE_ME_PARSING_SUPPORT"
DEFINES += "U_ENABLE_GUID_DATABASE_SUPPORT"

HEADERS += uefitool.h \
 searchdialog.h \
 hexviewdialog.h \
 gotobasedialog.h \
 gotoaddressdialog.h \
 hexlineedit.h \
 ffsfinder.h \
 hexspinbox.h \
 ../common/fitparser.h \
 ../common/guiddatabase.h \
 ../common/nvram.h \
 ../common/nvramparser.h \
 ../common/meparser.h \
 ../common/ffsops.h \
 ../common/basetypes.h \
 ../common/descriptor.h \
 ../common/gbe.h \
 ../common/me.h \
 ../common/ffs.h \
 ../common/peimage.h \
 ../common/types.h \
 ../common/utility.h \
 ../common/parsingdata.h \
 ../common/ffsbuilder.h \
 ../common/ffsparser.h \
 ../common/ffsreport.h \
 ../common/treeitem.h \
 ../common/intel_fit.h \
 ../common/intel_microcode.h \
 ../common/treemodel.h \
 ../common/LZMA/LzmaCompress.h \
 ../common/LZMA/LzmaDecompress.h \
 ../common/Tiano/EfiTianoDecompress.h \
 ../common/Tiano/EfiTianoCompress.h \
 ../common/ustring.h \
 ../common/ubytearray.h \
 ../common/umemstream.h \
 ../common/digest/sha1.h \
 ../common/digest/sha2.h \
 ../common/digest/sm3.h \
 ../common/generated/ami_nvar.h \
 ../common/generated/apple_sysf.h \
 ../common/generated/edk2_vss.h \
 ../common/generated/edk2_vss2.h \
 ../common/generated/edk2_ftw.h \
 ../common/generated/insyde_fdc.h \
 ../common/generated/insyde_fdm.h \
 ../common/generated/ms_slic_marker.h \
 ../common/generated/ms_slic_pubkey.h \
 ../common/generated/phoenix_flm.h \
 ../common/generated/phoenix_evsa.h \
 ../common/generated/intel_acbp_v1.h \
 ../common/generated/intel_acbp_v2.h \
 ../common/generated/intel_keym_v1.h \
 ../common/generated/intel_keym_v2.h \
 ../common/generated/intel_acm.h \
 ../common/kaitai/kaitaistream.h \
 ../common/kaitai/kaitaistruct.h \
 ../common/kaitai/exceptions.h \
 ../common/zlib/zlib.h \
 ../common/zlib/crc32.h \
 ../version.h \
 QHexView/include/QHexView/model/buffer/qhexbuffer.h \
 QHexView/include/QHexView/model/buffer/qdevicebuffer.h \
 QHexView/include/QHexView/model/buffer/qmemorybuffer.h \
 QHexView/include/QHexView/model/buffer/qmappedfilebuffer.h \
 QHexView/include/QHexView/model/commands/hexcommand.h \
 QHexView/include/QHexView/model/commands/insertcommand.h \
 QHexView/include/QHexView/model/commands/removecommand.h \
 QHexView/include/QHexView/model/commands/replacecommand.h \
 QHexView/include/QHexView/model/qhexcursor.h \
 QHexView/include/QHexView/model/qhexdelegate.h \
 QHexView/include/QHexView/model/qhexdocument.h \
 QHexView/include/QHexView/model/qhexmetadata.h \
 QHexView/include/QHexView/model/qhexoptions.h \
 QHexView/include/QHexView/model/qhexutils.h \
 QHexView/include/QHexView/qhexview.h

SOURCES += uefitool_main.cpp \
 uefitool.cpp \
 searchdialog.cpp \
 hexviewdialog.cpp \
 hexlineedit.cpp \
 ffsfinder.cpp \
 hexspinbox.cpp \
 ../common/fitparser.cpp \
 ../common/guiddatabase.cpp \
 ../common/nvram.cpp \
 ../common/nvramparser.cpp \
 ../common/meparser.cpp \
 ../common/ffsops.cpp \
 ../common/types.cpp \
 ../common/descriptor.cpp \
 ../common/ffs.cpp \
 ../common/peimage.cpp \
 ../common/utility.cpp \
 ../common/ffsbuilder.cpp \
 ../common/ffsparser.cpp \
 ../common/ffsreport.cpp \
 ../common/treeitem.cpp \
 ../common/treemodel.cpp \
 ../common/LZMA/LzmaCompress.c \
 ../common/LZMA/LzmaDecompress.c \
 ../common/LZMA/SDK/C/CpuArch.c \
 ../common/LZMA/SDK/C/Bra.c \
 ../common/LZMA/SDK/C/Bra86.c \
 ../common/LZMA/SDK/C/LzFind.c \
 ../common/LZMA/SDK/C/LzmaDec.c \
 ../common/LZMA/SDK/C/LzmaEnc.c \
 ../common/Tiano/EfiTianoDecompress.c \
 ../common/Tiano/EfiTianoCompress.c \
 ../common/Tiano/EfiTianoCompressLegacy.c \
 ../common/ustring.cpp \
 ../common/digest/sha1.c \
 ../common/digest/sha256.c \
 ../common/digest/sha512.c \
 ../common/digest/sm3.c \
 ../common/generated/ami_nvar.cpp \
 ../common/generated/apple_sysf.cpp \
 ../common/generated/edk2_vss.cpp \
 ../common/generated/edk2_vss2.cpp \
 ../common/generated/edk2_ftw.cpp \
 ../common/generated/insyde_fdc.cpp \
 ../common/generated/insyde_fdm.cpp \
 ../common/generated/ms_slic_marker.cpp \
 ../common/generated/ms_slic_pubkey.cpp \
 ../common/generated/phoenix_flm.cpp \
 ../common/generated/phoenix_evsa.cpp \
 ../common/generated/intel_acbp_v1.cpp \
 ../common/generated/intel_acbp_v2.cpp \
 ../common/generated/intel_keym_v1.cpp \
 ../common/generated/intel_keym_v2.cpp \
 ../common/generated/intel_acm.cpp \
 ../common/kaitai/kaitaistream.cpp \
 ../common/zlib/adler32.c \
 ../common/zlib/compress.c \
 ../common/zlib/crc32.c \
 ../common/zlib/deflate.c \
 ../common/zlib/gzclose.c \
 ../common/zlib/gzlib.c \
 ../common/zlib/gzread.c \
 ../common/zlib/gzwrite.c \
 ../common/zlib/inflate.c \
 ../common/zlib/infback.c \
 ../common/zlib/inftrees.c \
 ../common/zlib/inffast.c \
 ../common/zlib/trees.c \
 ../common/zlib/uncompr.c \
 ../common/zlib/zutil.c \
 QHexView/src/model/buffer/qhexbuffer.cpp \
 QHexView/src/model/buffer/qdevicebuffer.cpp \
 QHexView/src/model/buffer/qmemorybuffer.cpp \
 QHexView/src/model/buffer/qmappedfilebuffer.cpp \
 QHexView/src/model/commands/hexcommand.cpp \
 QHexView/src/model/commands/insertcommand.cpp \
 QHexView/src/model/commands/removecommand.cpp \
 QHexView/src/model/commands/replacecommand.cpp \
 QHexView/src/model/qhexcursor.cpp \
 QHexView/src/model/qhexdelegate.cpp \
 QHexView/src/model/qhexdocument.cpp \
 QHexView/src/model/qhexmetadata.cpp \
 QHexView/src/model/qhexutils.cpp \
 QHexView/src/qhexview.cpp

INCLUDEPATH += QHexView/include/

FORMS += uefitool.ui \
 searchdialog.ui \
 hexviewdialog.ui \
 gotobasedialog.ui \
 gotoaddressdialog.ui

RESOURCES += uefitool.qrc
RC_FILE = uefitool.rc
ICON = icons/uefitool.icns
QMAKE_BUNDLE_DATA += ICONFILE
QMAKE_INFO_PLIST = Info.plist

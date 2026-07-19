# UEFIEdit is a console-only utility that does not use Qt.
# It uses the bstrlib-based UString/UByteArray implementations.
QT -= core gui
QT =
CONFIG -= qt

CONFIG += c++11 console
CONFIG -= app_bundle

DEFINES += "U_ENABLE_FIT_PARSING_SUPPORT"
DEFINES += "U_ENABLE_NVRAM_PARSING_SUPPORT"
DEFINES += "U_ENABLE_ME_PARSING_SUPPORT"
DEFINES += "U_ENABLE_GUID_DATABASE_SUPPORT"

TARGET = UEFIEdit
TEMPLATE = app

INCLUDEPATH += ..

HEADERS += uefiedit.h \
  ../common/basetypes.h \
  ../common/ustring.h \
  ../common/ubytearray.h \
  ../common/umemstream.h \
  ../common/filesystem.h \
  ../common/treemodel.h \
  ../common/treeitem.h \
  ../common/types.h \
  ../common/parsingdata.h \
  ../common/ffs.h \
  ../common/ffsparser.h \
  ../common/ffsops.h \
  ../common/ffsbuilder.h \
  ../common/ffsreport.h \
  ../common/utility.h \
  ../common/guiddatabase.h \
  ../common/descriptor.h \
  ../common/gbe.h \
  ../common/me.h \
  ../common/nvram.h \
  ../common/nvramparser.h \
  ../common/meparser.h \
  ../common/fitparser.h \
  ../common/peimage.h \
  ../common/intel_fit.h \
  ../common/intel_microcode.h \
  ../common/amd_microcode.h \
  ../common/LZMA/LzmaDecompress.h \
  ../common/Tiano/EfiTianoDecompress.h \
  ../common/digest/sha1.h \
  ../common/digest/sha2.h \
  ../common/digest/sm3.h \
  ../common/generated/ami_nvar.h \
  ../common/generated/apple_sysf.h \
  ../common/generated/dell_dvar.h \
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
  ../version.h

SOURCES += uefiedit_main.cpp \
  uefiedit.cpp \
  ../common/guiddatabase.cpp \
  ../common/types.cpp \
  ../common/descriptor.cpp \
  ../common/filesystem.cpp \
  ../common/ffs.cpp \
  ../common/nvram.cpp \
  ../common/nvramparser.cpp \
  ../common/meparser.cpp \
  ../common/fitparser.cpp \
  ../common/ffsparser.cpp \
  ../common/amd_microcode.cpp \
  ../common/ffsreport.cpp \
  ../common/peimage.cpp \
  ../common/treeitem.cpp \
  ../common/treemodel.cpp \
  ../common/utility.cpp \
  ../common/ffsops.cpp \
  ../common/ffsbuilder.cpp \
  ../common/brotli/common/constants.c \
  ../common/brotli/common/context.c \
  ../common/brotli/common/dictionary.c \
  ../common/brotli/common/platform.c \
  ../common/brotli/common/shared_dictionary.c \
  ../common/brotli/common/transform.c \
  ../common/brotli/dec/bit_reader.c \
  ../common/brotli/dec/decode.c \
  ../common/brotli/dec/huffman.c \
  ../common/brotli/dec/prefix.c \
  ../common/brotli/dec/state.c \
  ../common/brotli/dec/static_init.c \
  ../common/LZMA/LzmaDecompress.c \
  ../common/LZMA/SDK/C/Bra.c \
  ../common/LZMA/SDK/C/Bra86.c \
  ../common/LZMA/SDK/C/CpuArch.c \
  ../common/LZMA/SDK/C/LzmaDec.c \
  ../common/LZMA/SDK/C/LzFind.c \
  ../common/LZMA/SDK/C/LzmaEnc.c \
  ../common/LZMA/LzmaCompress.c \
  ../common/Tiano/EfiTianoDecompress.c \
  ../common/Tiano/EfiTianoCompress.c \
  ../common/Tiano/EfiTianoCompressLegacy.c \
  ../common/ustring.cpp \
  ../common/bstrlib/bstrlib.c \
  ../common/bstrlib/bstrwrap.cpp \
  ../common/digest/sha1.c \
  ../common/digest/sha256.c \
  ../common/digest/sha512.c \
  ../common/digest/sm3.c \
  ../common/generated/ami_nvar.cpp \
  ../common/generated/apple_sysf.cpp \
  ../common/generated/dell_dvar.cpp \
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
  ../common/zlib/zutil.c
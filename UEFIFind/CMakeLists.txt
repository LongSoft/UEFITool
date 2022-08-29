CMAKE_MINIMUM_REQUIRED(VERSION 3.1.0 FATAL_ERROR)

PROJECT(UEFIFind)

SET(CMAKE_CXX_STANDARD 11)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_EXTENSIONS OFF)

SET(PROJECT_SOURCES
 uefifind_main.cpp
 uefifind.cpp
 ../common/guiddatabase.cpp
 ../common/types.cpp
 ../common/descriptor.cpp
 ../common/ffs.cpp
 ../common/nvram.cpp
 ../common/nvramparser.cpp
 ../common/ffsparser.cpp
 ../common/fitparser.cpp
 ../common/ffsreport.cpp
 ../common/ffsutils.cpp
 ../common/peimage.cpp
 ../common/treeitem.cpp
 ../common/treemodel.cpp
 ../common/utility.cpp
 ../common/LZMA/LzmaDecompress.c
 ../common/LZMA/SDK/C/Bra.c
 ../common/LZMA/SDK/C/Bra86.c
 ../common/LZMA/SDK/C/CpuArch.c
 ../common/LZMA/SDK/C/LzmaDec.c
 ../common/Tiano/EfiTianoDecompress.c
 ../common/ustring.cpp
 ../common/bstrlib/bstrlib.c
 ../common/bstrlib/bstrwrap.cpp
 ../common/generated/intel_acbp_v1.cpp
 ../common/generated/intel_acbp_v2.cpp
 ../common/generated/intel_keym_v1.cpp
 ../common/generated/intel_keym_v2.cpp
 ../common/generated/intel_acm.cpp
 ../common/kaitai/kaitaistream.cpp
 ../common/digest/sha1.c
 ../common/digest/sha256.c
 ../common/digest/sha512.c
 ../common/digest/sm3.c
 ../common/zlib/adler32.c
 ../common/zlib/compress.c
 ../common/zlib/crc32.c
 ../common/zlib/deflate.c
 ../common/zlib/gzclose.c
 ../common/zlib/gzlib.c
 ../common/zlib/gzread.c
 ../common/zlib/gzwrite.c
 ../common/zlib/inflate.c
 ../common/zlib/infback.c
 ../common/zlib/inftrees.c
 ../common/zlib/inffast.c
 ../common/zlib/trees.c
 ../common/zlib/uncompr.c
 ../common/zlib/zutil.c
)

ADD_DEFINITIONS(-DU_ENABLE_NVRAM_PARSING_SUPPORT -DU_ENABLE_FIT_PARSING_SUPPORT -DU_ENABLE_GUID_DATABASE_SUPPORT)

ADD_EXECUTABLE(UEFIFind ${PROJECT_SOURCES})


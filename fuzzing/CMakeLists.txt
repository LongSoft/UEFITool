CMAKE_MINIMUM_REQUIRED(VERSION 3.1.0 FATAL_ERROR)

PROJECT(ffsparser_fuzzer LANGUAGES C CXX)

OPTION(USE_QT "Link against Qt" OFF)
OPTION(USE_AFL "Build in AFL-compatible mode" OFF)

SET(CMAKE_CXX_STANDARD 11)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_EXTENSIONS OFF)

SET(PROJECT_SOURCES
 ffsparser_fuzzer.cpp
 ../common/types.cpp
 ../common/descriptor.cpp
 ../common/guiddatabase.cpp
 ../common/ffs.cpp
 ../common/nvram.cpp
 ../common/nvramparser.cpp
 ../common/meparser.cpp
 ../common/ffsparser.cpp
 ../common/fitparser.cpp
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
 ../common/generated/ami_nvar.cpp
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

IF(USE_AFL)
  SET(PROJECT_SOURCES ${PROJECT_SOURCES} afl_driver.cpp)
  MESSAGE("-- Building in AFL-compatible mode")
ELSE()
  MESSAGE("-- Building in libFuzzer mode")
ENDIF()

IF(NOT USE_QT)
  SET(PROJECT_SOURCES ${PROJECT_SOURCES}
    ../common/bstrlib/bstrlib.c
    ../common/bstrlib/bstrwrap.cpp
  )
  MESSAGE("-- Using non-Qt implementations")
ELSE()
  FIND_PACKAGE(Qt6 REQUIRED COMPONENTS Core)
  MESSAGE("-- Using Qt version: ${Qt6_VERSION}")
ENDIF()

ADD_DEFINITIONS(
 -DU_ENABLE_NVRAM_PARSING_SUPPORT
 -DU_ENABLE_ME_PARSING_SUPPORT
 -DU_ENABLE_FIT_PARSING_SUPPORT
 -DU_ENABLE_GUID_DATABASE_SUPPORT
)

ADD_EXECUTABLE(ffsparser_fuzzer ${PROJECT_SOURCES})


IF(NOT USE_AFL_DRIVER)
TARGET_COMPILE_OPTIONS(ffsparser_fuzzer PRIVATE -O1 -fno-omit-frame-pointer -g -ggdb3 -fsanitize=fuzzer,address,undefined -fsanitize-address-use-after-scope -fno-sanitize-recover=undefined)
TARGET_LINK_LIBRARIES(ffsparser_fuzzer PRIVATE -fsanitize=fuzzer,address,undefined)
ELSE()
TARGET_COMPILE_OPTIONS(ffsparser_fuzzer PRIVATE -O1 -fno-omit-frame-pointer -g -ggdb3 -fsanitize=address,undefined -fsanitize-coverage=trace-pc-guard -fsanitize-address-use-after-scope -fno-sanitize-recover=undefined)
TARGET_LINK_LIBRARIES(ffsparser_fuzzer PRIVATE -fsanitize=address,undefined)
ENDIF()

IF(USE_QT)
  TARGET_LINK_LIBRARIES(ffsparser_fuzzer PRIVATE Qt6::Core)
ENDIF()

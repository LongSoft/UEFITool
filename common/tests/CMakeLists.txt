CMAKE_MINIMUM_REQUIRED(VERSION 3.22)

PROJECT(UEFIToolTests)

# Tests compile as C++14 for Catch2 v3 compatibility.
# Library code remains C++11-compatible.
SET(CMAKE_CXX_STANDARD 14)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_EXTENSIONS OFF)

# Catch2 amalgamated source (provides main)
SET(CATCH2_SOURCE
  catch2/catch_amalgamated.cpp
)

# Common library sources (same set used by UEFIExtract)
SET(COMMON_SOURCES
  ../guiddatabase.cpp
  ../types.cpp
  ../filesystem.cpp
  ../descriptor.cpp
  ../ffs.cpp
  ../nvram.cpp
  ../nvramparser.cpp
  ../meparser.cpp
  ../ffsparser.cpp
  ../amd_microcode.cpp
  ../fitparser.cpp
  ../ffsreport.cpp
  ../peimage.cpp
  ../treeitem.cpp
  ../treemodel.cpp
  ../utility.cpp
  ../ustring.cpp
  ../bstrlib/bstrlib.c
  ../bstrlib/bstrwrap.cpp
  ../brotli/common/constants.c
  ../brotli/common/context.c
  ../brotli/common/dictionary.c
  ../brotli/common/platform.c
  ../brotli/common/shared_dictionary.c
  ../brotli/common/transform.c
  ../brotli/dec/bit_reader.c
  ../brotli/dec/decode.c
  ../brotli/dec/huffman.c
  ../brotli/dec/prefix.c
  ../brotli/dec/state.c
  ../brotli/dec/static_init.c
  ../LZMA/LzmaDecompress.c
  ../LZMA/SDK/C/Bra.c
  ../LZMA/SDK/C/Bra86.c
  ../LZMA/SDK/C/CpuArch.c
  ../LZMA/SDK/C/LzmaDec.c
  ../Tiano/EfiTianoDecompress.c
  ../zlib/adler32.c
  ../zlib/compress.c
  ../zlib/crc32.c
  ../zlib/deflate.c
  ../zlib/gzclose.c
  ../zlib/gzlib.c
  ../zlib/gzread.c
  ../zlib/gzwrite.c
  ../zlib/inflate.c
  ../zlib/infback.c
  ../zlib/inftrees.c
  ../zlib/inffast.c
  ../zlib/trees.c
  ../zlib/uncompr.c
  ../zlib/zutil.c
  ../generated/ami_nvar.cpp
  ../generated/apple_sysf.cpp
  ../generated/dell_dvar.cpp
  ../generated/edk2_vss.cpp
  ../generated/edk2_vss2.cpp
  ../generated/edk2_ftw.cpp
  ../generated/insyde_fdc.cpp
  ../generated/insyde_fdm.cpp
  ../generated/ms_slic_marker.cpp
  ../generated/ms_slic_pubkey.cpp
  ../generated/phoenix_flm.cpp
  ../generated/phoenix_evsa.cpp
  ../generated/intel_acbp_v1.cpp
  ../generated/intel_acbp_v2.cpp
  ../generated/intel_keym_v1.cpp
  ../generated/intel_keym_v2.cpp
  ../generated/intel_acm.cpp
  ../kaitai/kaitaistream.cpp
  ../digest/sha1.c
  ../digest/sha256.c
  ../digest/sha512.c
  ../digest/sm3.c
)

# Test sources -- one file per tested module
SET(TEST_SOURCES
  test_utility.cpp
  test_ffs.cpp
  test_types.cpp
  test_descriptor.cpp
)

ADD_DEFINITIONS(
  -DU_ENABLE_NVRAM_PARSING_SUPPORT
  -DU_ENABLE_ME_PARSING_SUPPORT
  -DU_ENABLE_FIT_PARSING_SUPPORT
  -DU_ENABLE_GUID_DATABASE_SUPPORT
)

ADD_EXECUTABLE(uefitool_tests ${CATCH2_SOURCE} ${COMMON_SOURCES} ${TEST_SOURCES})

TARGET_INCLUDE_DIRECTORIES(uefitool_tests PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/catch2
  ${CMAKE_CURRENT_SOURCE_DIR}/..
)

ENABLE_TESTING()
ADD_TEST(NAME uefitool_tests COMMAND uefitool_tests)

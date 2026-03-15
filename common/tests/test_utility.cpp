#include <vector>

#include <catch_amalgamated.hpp>
#include "utility.h"

TEST_CASE("Smoke test: utility compiles and links", "[utility][smoke]") {
    REQUIRE(calculateSum8(nullptr, 0) == 0);
}

TEST_CASE("calculateSum8", "[utility][checksum]") {
    SECTION("null buffer returns 0") {
        REQUIRE(calculateSum8(nullptr, 0) == 0);
        REQUIRE(calculateSum8(nullptr, 100) == 0);
    }
    SECTION("empty buffer returns 0") {
        UINT8 buf[] = {0x42};
        REQUIRE(calculateSum8(buf, 0) == 0);
    }
    SECTION("single byte") {
        UINT8 buf[] = {0x42};
        REQUIRE(calculateSum8(buf, 1) == 0x42);
    }
    SECTION("overflow wraps") {
        UINT8 buf[] = {0x80, 0x80};
        REQUIRE(calculateSum8(buf, 2) == 0x00);
    }
    SECTION("known sum") {
        UINT8 buf[] = {0x01, 0x02, 0x03, 0x04};
        REQUIRE(calculateSum8(buf, 4) == 0x0A);
    }
    SECTION("all 0xFF") {
        UINT8 buf[] = {0xFF, 0xFF, 0xFF};
        // 0xFF * 3 = 0x2FD, truncated to 0xFD
        REQUIRE(calculateSum8(buf, 3) == 0xFD);
    }
}

TEST_CASE("calculateChecksum8", "[utility][checksum]") {
    SECTION("null buffer returns 0") {
        REQUIRE(calculateChecksum8(nullptr, 0) == 0);
    }
    SECTION("all zeros") {
        UINT8 buf[] = {0x00, 0x00};
        REQUIRE(calculateChecksum8(buf, 2) == 0x00);
    }
    SECTION("complementary property") {
        // sum8 + checksum8 should equal 0 mod 256
        UINT8 buf[] = {0x01, 0x02, 0x03, 0x04};
        UINT8 sum = calculateSum8(buf, 4);
        UINT8 chk = calculateChecksum8(buf, 4);
        REQUIRE((UINT8)(sum + chk) == 0);
    }
    SECTION("single byte") {
        UINT8 buf[] = {0x01};
        REQUIRE(calculateChecksum8(buf, 1) == 0xFF);
    }
}

TEST_CASE("calculateChecksum16", "[utility][checksum]") {
    SECTION("null buffer returns 0") {
        REQUIRE(calculateChecksum16(nullptr, 0) == 0);
    }
    SECTION("single word") {
        UINT16 buf[] = {0x0001};
        REQUIRE(calculateChecksum16(buf, sizeof(buf)) == 0xFFFF);
    }
    SECTION("complementary property") {
        UINT16 buf[] = {0x1234, 0x5678};
        UINT16 sum = (UINT16)(buf[0] + buf[1]);
        UINT16 chk = calculateChecksum16(buf, sizeof(buf));
        REQUIRE((UINT16)(sum + chk) == 0);
    }
}

TEST_CASE("calculateChecksum32", "[utility][checksum]") {
    SECTION("null buffer returns 0") {
        REQUIRE(calculateChecksum32(nullptr, 0) == 0);
    }
    SECTION("single dword") {
        UINT32 buf[] = {0x00000001};
        REQUIRE(calculateChecksum32(buf, sizeof(buf)) == 0xFFFFFFFF);
    }
    SECTION("complementary property") {
        UINT32 buf[] = {0xDEADBEEF, 0xCAFEBABE};
        UINT32 sum = buf[0] + buf[1];
        UINT32 chk = calculateChecksum32(buf, sizeof(buf));
        REQUIRE(sum + chk == 0);
    }
}

TEST_CASE("fletcher32", "[utility][checksum]") {
    SECTION("empty array") {
        UByteArray empty;
        // c0 = 0xFFFF, c1 = 0xFFFF, no iterations
        REQUIRE(fletcher32(empty) == 0xFFFFFFFF);
    }
    SECTION("deterministic for same input") {
        UByteArray data("\x01\x02\x03\x04", 4);
        UINT32 first = fletcher32(data);
        UINT32 second = fletcher32(data);
        REQUIRE(first == second);
    }
    SECTION("different data produces different checksums") {
        UByteArray a("\x01\x02\x03\x04", 4);
        UByteArray b("\x04\x03\x02\x01", 4);
        REQUIRE(fletcher32(a) != fletcher32(b));
    }
}

TEST_CASE("uniformByte", "[utility][uniform]") {
    SECTION("empty array returns default") {
        UByteArray empty;
        REQUIRE(uniformByte(empty) == UINT32_MAX);
    }
    SECTION("empty array with custom default") {
        UByteArray empty;
        REQUIRE(uniformByte(empty, 0x42) == 0x42);
    }
    SECTION("single byte") {
        UByteArray single("\x00", 1);
        REQUIRE(uniformByte(single) == 0x00);
    }
    SECTION("single 0xFF") {
        UByteArray single("\xFF", 1);
        REQUIRE(uniformByte(single) == 0xFF);
    }
    SECTION("all same") {
        UByteArray data(4, '\xAB');
        REQUIRE(uniformByte(data) == 0xAB);
    }
    SECTION("not uniform") {
        UByteArray data("\x00\x01", 2);
        REQUIRE(uniformByte(data) == UINT32_MAX);
    }
    SECTION("non-uniform at end") {
        UByteArray data("\xFF\xFF\xFF\x00", 4);
        REQUIRE(uniformByte(data) == UINT32_MAX);
    }
}

TEST_CASE("isUniformByte", "[utility][uniform]") {
    SECTION("empty array matches any value") {
        UByteArray empty;
        REQUIRE(isUniformByte(empty, 0x00));
        REQUIRE(isUniformByte(empty, 0xFF));
    }
    SECTION("matching uniform") {
        UByteArray zeros(4, '\x00');
        REQUIRE(isUniformByte(zeros, 0x00));
    }
    SECTION("non-matching uniform") {
        UByteArray zeros(4, '\x00');
        REQUIRE_FALSE(isUniformByte(zeros, 0xFF));
    }
}

TEST_CASE("getPaddingType", "[utility][padding]") {
    SECTION("all zeros") {
        UByteArray zeros(4, '\x00');
        REQUIRE(getPaddingType(zeros) == Subtypes::ZeroPadding);
    }
    SECTION("all 0xFF") {
        UByteArray ones(4, '\xFF');
        REQUIRE(getPaddingType(ones) == Subtypes::OnePadding);
    }
    SECTION("mixed data") {
        UByteArray mixed("\x00\xFF", 2);
        REQUIRE(getPaddingType(mixed) == Subtypes::DataPadding);
    }
    SECTION("single 0x42") {
        UByteArray data("\x42", 1);
        REQUIRE(getPaddingType(data) == Subtypes::DataPadding);
    }
    SECTION("empty") {
        UByteArray empty;
        // uniformByte returns UINT32_MAX, no case matches
        REQUIRE(getPaddingType(empty) == Subtypes::DataPadding);
    }
}

TEST_CASE("makePattern", "[utility][pattern]") {
    std::vector<UINT8> pattern, mask;

    SECTION("valid hex pair") {
        REQUIRE(makePattern("AABB", pattern, mask));
        REQUIRE(pattern.size() == 2);
        REQUIRE(pattern[0] == 0xAA);
        REQUIRE(pattern[1] == 0xBB);
        REQUIRE(mask[0] == 0xFF);
        REQUIRE(mask[1] == 0xFF);
    }
    SECTION("case insensitive") {
        REQUIRE(makePattern("aabb", pattern, mask));
        REQUIRE(pattern[0] == 0xAA);
        REQUIRE(pattern[1] == 0xBB);
    }
    SECTION("high nibble wildcard") {
        REQUIRE(makePattern(".B", pattern, mask));
        REQUIRE(pattern.size() == 1);
        REQUIRE(pattern[0] == 0x0B);
        REQUIRE(mask[0] == 0x0F);
    }
    SECTION("low nibble wildcard") {
        REQUIRE(makePattern("A.", pattern, mask));
        REQUIRE(pattern.size() == 1);
        REQUIRE(pattern[0] == 0xA0);
        REQUIRE(mask[0] == 0xF0);
    }
    SECTION("full wildcard byte") {
        REQUIRE(makePattern("..", pattern, mask));
        REQUIRE(pattern.size() == 1);
        REQUIRE(pattern[0] == 0x00);
        REQUIRE(mask[0] == 0x00);
    }
    SECTION("empty string fails") {
        REQUIRE_FALSE(makePattern("", pattern, mask));
    }
    SECTION("odd length fails") {
        REQUIRE_FALSE(makePattern("A", pattern, mask));
    }
    SECTION("invalid hex fails") {
        REQUIRE_FALSE(makePattern("GG", pattern, mask));
    }
}

TEST_CASE("findPattern", "[utility][pattern]") {
    UINT8 data[] = {0x00, 0x11, 0x22, 0x33, 0x44};

    SECTION("exact match at start") {
        UINT8 pat[] = {0x00, 0x11};
        UINT8 msk[] = {0xFF, 0xFF};
        REQUIRE(findPattern(pat, msk, 2, data, 5, 0) == 0);
    }
    SECTION("match in middle") {
        UINT8 pat[] = {0x22, 0x33};
        UINT8 msk[] = {0xFF, 0xFF};
        REQUIRE(findPattern(pat, msk, 2, data, 5, 0) == 2);
    }
    SECTION("no match") {
        UINT8 pat[] = {0xFF};
        UINT8 msk[] = {0xFF};
        REQUIRE(findPattern(pat, msk, 1, data, 5, 0) == -1);
    }
    SECTION("wildcard matches") {
        UINT8 pat[] = {0x00, 0x10};  // Low nibble 0, high nibble 1
        UINT8 msk[] = {0x0F, 0xF0}; // Mask keeps low nibble of first, high nibble of second
        REQUIRE(findPattern(pat, msk, 2, data, 5, 0) == 0);
    }
    SECTION("offset skips early match") {
        UINT8 pat[] = {0x22};
        UINT8 msk[] = {0xFF};
        REQUIRE(findPattern(pat, msk, 1, data, 5, 3) == -1);
    }
    SECTION("empty pattern") {
        REQUIRE(findPattern(nullptr, nullptr, 0, data, 5, 0) == -1);
    }
    SECTION("empty data") {
        UINT8 pat[] = {0x00};
        UINT8 msk[] = {0xFF};
        REQUIRE(findPattern(pat, msk, 1, nullptr, 0, 0) == -1);
    }
    SECTION("pattern at exact end") {
        UINT8 pat[] = {0x44};
        UINT8 msk[] = {0xFF};
        REQUIRE(findPattern(pat, msk, 1, data, 5, 0) == 4);
    }
}

TEST_CASE("fourCC", "[utility][string]") {
    SECTION("FV signature") {
        // 0x4856465F = '_' 'F' 'V' 'H' in LE
        UString result = fourCC(0x4856465F);
        REQUIRE(result == UString("_FVH"));
    }
    SECTION("CPD signature") {
        UString result = fourCC(0x44504324);
        REQUIRE(result == UString("$CPD"));
    }
}

TEST_CASE("visibleAsciiOrHex", "[utility][string]") {
    SECTION("all printable returns ASCII") {
        UINT8 buf[] = {'H', 'e', 'l', 'l', 'o'};
        REQUIRE(visibleAsciiOrHex(buf, 5) == UString("Hello"));
    }
    SECTION("printable with trailing zeros returns ASCII") {
        UINT8 buf[] = {'H', 'i', 0x00, 0x00};
        REQUIRE(visibleAsciiOrHex(buf, 4) == UString("Hi"));
    }
    SECTION("control char returns hex") {
        UINT8 buf[] = {0x01};
        REQUIRE(visibleAsciiOrHex(buf, 1) == UString("01"));
    }
    SECTION("high ASCII returns hex") {
        UINT8 buf[] = {0x80};
        REQUIRE(visibleAsciiOrHex(buf, 1) == UString("80"));
    }
    SECTION("null then non-null returns hex") {
        UINT8 buf[] = {0x00, 'A'};
        // 0x00 < 0x20, not printable -> hex
        REQUIRE(visibleAsciiOrHex(buf, 2) == UString("0041"));
    }
}

TEST_CASE("fixFileName", "[utility][string]") {
    SECTION("normal name unchanged") {
        UString name("firmware_v1.2");
        fixFileName(name, false);
        REQUIRE(name == UString("firmware_v1.2"));
    }
    SECTION("spaces replaced when flag set") {
        UString name("my file");
        fixFileName(name, true);
        REQUIRE(name == UString("my_file"));
    }
    SECTION("spaces kept when flag clear") {
        UString name("my file");
        fixFileName(name, false);
        REQUIRE(name == UString("my file"));
    }
    SECTION("slashes replaced") {
        UString name("path/name");
        fixFileName(name, false);
        REQUIRE(name == UString("path_name"));
    }
    SECTION("windows banned chars replaced") {
        UString name("a<b>c:d");
        fixFileName(name, false);
        REQUIRE(name == UString("a_b_c_d"));
    }
    SECTION("empty becomes underscore") {
        UString name("");
        fixFileName(name, false);
        REQUIRE(name == UString("_"));
    }
}

TEST_CASE("errorCodeToUString", "[utility][string]") {
    SECTION("success") {
        REQUIRE(errorCodeToUString(U_SUCCESS) == UString("Success"));
    }
    SECTION("common errors") {
        REQUIRE(errorCodeToUString(U_INVALID_PARAMETER) == UString("Function called with invalid parameter"));
        REQUIRE(errorCodeToUString(U_ITEM_NOT_FOUND) == UString("Item not found"));
        REQUIRE(errorCodeToUString(U_NOT_IMPLEMENTED) == UString("Not implemented"));
    }
    SECTION("unknown code") {
        UString result = errorCodeToUString(100);
        // 100 = 0x64
        REQUIRE(result == usprintf("Unknown error %02lX", (USTATUS)100));
    }
}

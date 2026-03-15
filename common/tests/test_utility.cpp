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

#include <cstring>
#include <catch_amalgamated.hpp>
#include "ffs.h"

TEST_CASE("Smoke test: ffs compiles and links", "[ffs][smoke]") {
    UINT8 buf[3] = {0, 0, 0};
    REQUIRE(uint24ToUint32(buf) == 0);
}

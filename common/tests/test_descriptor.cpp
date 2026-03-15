#include <catch_amalgamated.hpp>
#include "descriptor.h"

TEST_CASE("Smoke test: descriptor compiles and links", "[descriptor][smoke]") {
    REQUIRE(calculateRegionOffset(0) == 0);
}

#include <catch_amalgamated.hpp>
#include "utility.h"

TEST_CASE("Smoke test: utility compiles and links", "[utility][smoke]") {
    REQUIRE(calculateSum8(nullptr, 0) == 0);
}

#include <catch_amalgamated.hpp>
#include "ustring.h"
#include "types.h"
#include "ffs.h"

TEST_CASE("Smoke test: types compiles and links", "[types][smoke]") {
    REQUIRE(itemTypeToUString(Types::Root) == UString("Root"));
}

#include <catch_amalgamated.hpp>
#include "ustring.h"
#include "types.h"
#include "ffs.h"

TEST_CASE("Smoke test: types compiles and links", "[types][smoke]") {
    REQUIRE(itemTypeToUString(Types::Root) == UString("Root"));
}

TEST_CASE("itemTypeToUString", "[types]") {
    REQUIRE(itemTypeToUString(Types::Root) == UString("Root"));
    REQUIRE(itemTypeToUString(Types::Capsule) == UString("Capsule"));
    REQUIRE(itemTypeToUString(Types::Image) == UString("Image"));
    REQUIRE(itemTypeToUString(Types::Region) == UString("Region"));
    REQUIRE(itemTypeToUString(Types::Volume) == UString("Volume"));
    REQUIRE(itemTypeToUString(Types::File) == UString("File"));
    REQUIRE(itemTypeToUString(Types::Section) == UString("Section"));
    REQUIRE(itemTypeToUString(Types::Padding) == UString("Padding"));
    REQUIRE(itemTypeToUString(Types::FreeSpace) == UString("Free space"));
    REQUIRE(itemTypeToUString(Types::Microcode) == UString("Microcode"));
    // Unknown
    REQUIRE(itemTypeToUString(0) == usprintf("Unknown %02Xh", 0));
}

TEST_CASE("itemSubtypeToUString", "[types]") {
    SECTION("image subtypes") {
        REQUIRE(itemSubtypeToUString(Types::Image, Subtypes::IntelImage) == UString("Intel"));
        REQUIRE(itemSubtypeToUString(Types::Image, Subtypes::UefiImage) == UString("UEFI"));
        REQUIRE(itemSubtypeToUString(Types::Image, Subtypes::AmdImage) == UString("AMD"));
    }
    SECTION("padding subtypes") {
        REQUIRE(itemSubtypeToUString(Types::Padding, Subtypes::ZeroPadding) == UString("Empty (00h)"));
        REQUIRE(itemSubtypeToUString(Types::Padding, Subtypes::OnePadding) == UString("Empty (FFh)"));
        REQUIRE(itemSubtypeToUString(Types::Padding, Subtypes::DataPadding) == UString("Non-empty"));
    }
    SECTION("volume subtypes") {
        REQUIRE(itemSubtypeToUString(Types::Volume, Subtypes::Ffs2Volume) == UString("FFSv2"));
        REQUIRE(itemSubtypeToUString(Types::Volume, Subtypes::Ffs3Volume) == UString("FFSv3"));
    }
    SECTION("capsule subtypes") {
        REQUIRE(itemSubtypeToUString(Types::Capsule, Subtypes::UefiCapsule) == UString("UEFI 2.0"));
        REQUIRE(itemSubtypeToUString(Types::Capsule, Subtypes::ToshibaCapsule) == UString("Toshiba"));
    }
    SECTION("region delegates to regionTypeToUString") {
        REQUIRE(itemSubtypeToUString(Types::Region, Subtypes::BiosRegion) == UString("BIOS"));
        REQUIRE(itemSubtypeToUString(Types::Region, Subtypes::MeRegion) == UString("ME"));
    }
    SECTION("unknown type/subtype returns empty") {
        REQUIRE(itemSubtypeToUString(Types::Root, 0) == UString());
    }
}

TEST_CASE("compressionTypeToUString", "[types]") {
    REQUIRE(compressionTypeToUString(COMPRESSION_ALGORITHM_NONE) == UString("None"));
    REQUIRE(compressionTypeToUString(COMPRESSION_ALGORITHM_LZMA) == UString("LZMA"));
    REQUIRE(compressionTypeToUString(COMPRESSION_ALGORITHM_GZIP) == UString("GZip"));
    REQUIRE(compressionTypeToUString(COMPRESSION_ALGORITHM_BROTLI) == UString("Brotli"));
    REQUIRE(compressionTypeToUString(99) == usprintf("Unknown %02Xh", 99));
}

TEST_CASE("actionTypeToUString", "[types]") {
    REQUIRE(actionTypeToUString(Actions::NoAction) == UString());
    REQUIRE(actionTypeToUString(Actions::Remove) == UString("Remove"));
    REQUIRE(actionTypeToUString(Actions::Rebuild) == UString("Rebuild"));
    REQUIRE(actionTypeToUString(Actions::Replace) == UString("Replace"));
    REQUIRE(actionTypeToUString(Actions::Create) == UString("Create"));
    REQUIRE(actionTypeToUString(Actions::Insert) == UString("Insert"));
    REQUIRE(actionTypeToUString(Actions::Rebase) == UString("Rebase"));
    // Note: Actions::Erase (51) exists in the enum but is not handled
    // in the switch -- it falls through to the unknown default.
    REQUIRE(actionTypeToUString(Actions::Erase) == usprintf("Unknown %02Xh", Actions::Erase));
    REQUIRE(actionTypeToUString(0) == usprintf("Unknown %02Xh", 0));
}

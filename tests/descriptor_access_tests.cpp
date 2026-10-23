#include "../common/descriptor.h"

#include <cstring>
#include <iostream>
#include <string>

static unsigned failures = 0;

static void expect(bool condition, const char* name)
{
    if (!condition) {
        std::cerr << "FAIL: " << name << '\n';
        failures++;
    }
}

static UByteArray fixture(UINT32 version, UINT8 clock, UINT32 biosMaster)
{
    UByteArray descriptor(FLASH_DESCRIPTOR_SIZE, '\0');
    FLASH_DESCRIPTOR_HEADER header = {};
    header.Signature = FLASH_DESCRIPTOR_SIGNATURE;
    FLASH_DESCRIPTOR_MAP map = {};
    map.ComponentBase = 3;
    map.RegionBase = 4;
    map.MasterBase = 8;
    map.DescriptorVersion = version;
    FLASH_DESCRIPTOR_COMPONENT_SECTION component = {};
    component.FlashParameters.ReadClockFrequency = clock;
    std::memcpy(descriptor.data(), &header, sizeof(header));
    std::memcpy(descriptor.data() + sizeof(header), &map, sizeof(map));
    std::memcpy(descriptor.data() + 0x30, &component, sizeof(component));
    std::memcpy(descriptor.data() + 0x80, &biosMaster, sizeof(biosMaster));
    return descriptor;
}

static FlashDescriptorAccessLayout detect(const UByteArray& descriptor)
{
    FLASH_DESCRIPTOR_MAP map;
    FLASH_DESCRIPTOR_COMPONENT_SECTION component;
    std::memcpy(&map, descriptor.constData() + sizeof(FLASH_DESCRIPTOR_HEADER), sizeof(map));
    std::memcpy(&component, descriptor.constData() + 0x30, sizeof(component));
    return detectFlashDescriptorAccessLayout(map, component);
}

static std::string format(const UByteArray& descriptor, FlashDescriptorAccessLayout layout)
{
    UString info;
    expect(formatFlashDescriptorAccess(descriptor, layout, info) == U_SUCCESS, "format succeeds");
    return std::string(info.toLocal8Bit());
}

int main()
{
    const UINT32 knownVersion = (FLASH_DESCRIPTOR_VERSION_MAJOR << 21)
        | (FLASH_DESCRIPTOR_VERSION_MINOR << 14);
    const UByteArray zeroClock = fixture(0, 0, 0x00A00F00);
    expect(detect(zeroClock) == FlashDescriptorAccessLayout::Ambiguous,
        "zero clock without a version marker is ambiguous");
    const std::string ambiguous = format(zeroClock, detect(zeroClock));
    expect(ambiguous.find("Descriptor access layout: ambiguous") != std::string::npos,
        "ambiguity is explicit");
    expect(ambiguous.find("Legacy interpretation:") != std::string::npos,
        "legacy interpretation is labelled");
    expect(ambiguous.find("Modern (Skylake+) interpretation:") != std::string::npos,
        "modern interpretation is labelled");
    expect(ambiguous.find("BIOS: A0h 00h") != std::string::npos, "legacy masks preserved");
    expect(ambiguous.find("BIOS: 00Fh 00Ah") != std::string::npos, "modern masks decoded");

    const std::string modern = format(zeroClock, FlashDescriptorAccessLayout::Modern);
    expect(modern.find("Desc       Yes   No") != std::string::npos, "modern descriptor is read-only");
    expect(modern.find("ME         Yes   No") != std::string::npos, "modern ME is read-only");
    expect(modern.find("live write-protection measurement") != std::string::npos,
        "file permissions are not presented as live hardware state");

    const UByteArray legacy = fixture(FLASH_DESCRIPTOR_VERSION_INVALID, 0, 0x03070000);
    expect(detect(legacy) == FlashDescriptorAccessLayout::Ambiguous,
        "an erased version marker does not prove legacy format");
    const std::string legacyInfo = format(legacy, FlashDescriptorAccessLayout::Legacy);
    expect(legacyInfo.find("BIOS: 07h 03h") != std::string::npos, "legacy access format preserved");
    expect(legacyInfo.find("ME         Yes   No") != std::string::npos, "legacy ME access preserved");

    const std::string denied = format(fixture(0, 0, 0), FlashDescriptorAccessLayout::Modern);
    expect(denied.find("BIOS       No    No") != std::string::npos, "BIOS access is not hardcoded");
    expect(format(fixture(0, 0, 0x00200200), FlashDescriptorAccessLayout::Modern)
        .find("BIOS       Yes   Yes") != std::string::npos, "modern BIOS bit1 grants access");
    expect(format(fixture(0, 0, 0x02020000), FlashDescriptorAccessLayout::Legacy)
        .find("BIOS       Yes   Yes") != std::string::npos, "legacy BIOS bit1 grants access");
    expect(format(fixture(0, 0, 0x02002000), FlashDescriptorAccessLayout::Modern)
        .find("EC         No    No") != std::string::npos, "modern bit5 is not EC");
    expect(format(fixture(0, 0, 0x10010000), FlashDescriptorAccessLayout::Modern)
        .find("EC         Yes   Yes") != std::string::npos, "modern EC uses bit8");
    const std::string extended = format(fixture(0, 0, 0x00000011), FlashDescriptorAccessLayout::Modern);
    expect(extended.find("Cannon Lake+ only; platform not identified") != std::string::npos,
        "extended permissions are conditional, not silently enabled");
    expect(extended.find("10GbE2     Yes   Yes") != std::string::npos, "conditional extended region12 uses low nibbles");
    UByteArray compact = fixture(0, 0, 0);
    compact[0x1A] = 9;
    expect(format(compact, FlashDescriptorAccessLayout::Modern).find("EC master: unavailable") != std::string::npos,
        "EC master is not read through the next mapped section");

    const UByteArray clockModern = fixture(FLASH_DESCRIPTOR_VERSION_INVALID, 1, 0x00A00F00);
    expect(detect(clockModern) == FlashDescriptorAccessLayout::Modern,
        "existing nonzero-clock modern detection preserved");
    expect(format(clockModern, detect(clockModern)).find("Legacy interpretation:") == std::string::npos,
        "nonzero-clock modern output is not marked ambiguous");
    expect(detect(fixture(knownVersion, 0, 0x00A00F00)) == FlashDescriptorAccessLayout::Modern,
        "known version marker identifies modern access despite zero clock");
    expect(detect(fixture(knownVersion | 1, 0, 0x00A00F00)) == FlashDescriptorAccessLayout::Ambiguous,
        "invalid reserved version bits do not identify a layout");
    expect(detect(fixture(2U << 21, 0, 0x00A00F00)) == FlashDescriptorAccessLayout::Ambiguous,
        "unknown version is not silently treated as legacy");

    const UByteArray unlocked = fixture(0, 0, 0xFFFFFFFF);
    const std::string unlockedInfo = format(unlocked, detect(unlocked));
    expect(unlockedInfo.find("BIOS: FFh FFh") != std::string::npos, "legacy all-ones masks");
    expect(unlockedInfo.find("BIOS: FFFh FFFh") != std::string::npos, "modern all-ones masks");

    UString info;
    expect(formatFlashDescriptorAccess(zeroClock.left(4095), detect(zeroClock), info)
        == U_INVALID_FLASH_DESCRIPTOR, "truncated descriptor rejected");
    UByteArray invalid = zeroClock;
    invalid[0x10] = 0;
    expect(formatFlashDescriptorAccess(invalid, detect(zeroClock), info)
        == U_INVALID_FLASH_DESCRIPTOR, "invalid signature rejected");
    invalid = zeroClock;
    invalid[0x18] = static_cast<char>(0xFF);
    expect(formatFlashDescriptorAccess(invalid, detect(zeroClock), info)
        == U_INVALID_FLASH_DESCRIPTOR, "invalid master base rejected");
    invalid = zeroClock;
    invalid[0x18] = 4;
    expect(formatFlashDescriptorAccess(invalid, detect(zeroClock), info)
        == U_INVALID_FLASH_DESCRIPTOR, "overlapping master base rejected");
    expect(formatFlashDescriptorAccess(zeroClock, static_cast<FlashDescriptorAccessLayout>(99), info)
        == U_INVALID_PARAMETER, "invalid layout rejected");

    FLASH_DESCRIPTOR_MAP map = {};
    map.ComponentBase = 3;
    map.RegionBase = 4;
    map.MasterBase = 8;
    expect(flashDescriptorRegionCount(map, FlashDescriptorAccessLayout::Ambiguous) == 16,
        "zero-count ambiguous descriptor retains sixteen bounded modern slots");
    for (UINT8 count = 0; count < 8; count++) {
        map.NumberOfRegions = count;
        expect(flashDescriptorRegionCount(map, FlashDescriptorAccessLayout::Legacy) == count + 1,
            "legacy traversal respects every declared table length");
        expect(flashDescriptorRegionCount(map, FlashDescriptorAccessLayout::Modern) == 16,
            "modern traversal ignores every value of the reserved legacy count");
        expect(flashDescriptorRegionCount(map, FlashDescriptorAccessLayout::Ambiguous)
            == (count ? count + 1 : 16),
            "ambiguous traversal honors a nonzero legacy count without truncating zero-count modern maps");
    }
    map.NumberOfRegions = 0;
    map.MasterBase = 6;
    expect(flashDescriptorRegionCount(map, FlashDescriptorAccessLayout::Ambiguous) == 8,
        "compact table stops at master section");
    map.ProcStrapsBase = 5;
    expect(flashDescriptorRegionCount(map, FlashDescriptorAccessLayout::Ambiguous) == 4,
        "processor straps also bound region slots");
    map.NumberOfRegions = 7;
    expect(flashDescriptorRegionCount(map, FlashDescriptorAccessLayout::Legacy) == 4
        && flashDescriptorRegionCount(map, FlashDescriptorAccessLayout::Ambiguous) == 4
        && flashDescriptorRegionCount(map, FlashDescriptorAccessLayout::Modern) == 4,
        "declared or reserved counts can never extend traversal into the following section");
    expect(calculateRegionSize(8, 7) == 0, "disabled region does not underflow");
    expect(calculateRegionSize(0, 0) == FLASH_DESCRIPTOR_SIZE, "single-page descriptor extent");

    UByteArray vscc = zeroClock;
    FLASH_DESCRIPTOR_UPPER_MAP upper = {};
    upper.VsccTableBase = 0xDF;
    upper.VsccTableSize = 2;
    VSCC_TABLE_ENTRY entry = {};
    entry.VendorId = 0xC8;
    entry.DeviceId0 = 0x40;
    entry.DeviceId1 = 0x19;
    std::memcpy(vscc.data() + FLASH_DESCRIPTOR_UPPER_MAP_BASE, &upper, sizeof(upper));
    std::memcpy(vscc.data() + 0xDF0, &entry, sizeof(entry));
    expect(formatFlashDescriptorVscc(vscc, info) == U_SUCCESS
        && info.contains("valid (1 entries)") && info.contains("C84019 (GigaDevice GD25x256)"),
        "valid VSCC entry is decoded at the indicated offset");
    expect(formatFlashDescriptorVscc(vscc.left(FLASH_DESCRIPTOR_UPPER_MAP_BASE + 3), info)
        == U_INVALID_FLASH_DESCRIPTOR && info.contains("truncated upper map"), "truncated VSCC upper map is explicit");
    upper.VsccTableSize = 1;
    std::memcpy(vscc.data() + FLASH_DESCRIPTOR_UPPER_MAP_BASE, &upper, sizeof(upper));
    expect(formatFlashDescriptorVscc(vscc, info) == U_INVALID_FLASH_DESCRIPTOR
        && info.contains("incomplete entry") && !info.contains("C84019"), "partial VSCC entry is never decoded");
    upper.VsccTableBase = 0xFF;
    upper.VsccTableSize = 8;
    std::memcpy(vscc.data() + FLASH_DESCRIPTOR_UPPER_MAP_BASE, &upper, sizeof(upper));
    expect(formatFlashDescriptorVscc(vscc, info) == U_INVALID_FLASH_DESCRIPTOR
        && info.contains("outside configuration"), "out-of-block VSCC table rejected");
    vscc += UByteArray(FLASH_DESCRIPTOR_SIZE * 3, '\0');
    expect(formatFlashDescriptorVscc(vscc, info) == U_INVALID_FLASH_DESCRIPTOR,
        "large descriptor region does not extend the selected VSCC block");
    upper.VsccTableBase = 0xEF;
    upper.VsccTableSize = 4;
    std::memcpy(vscc.data() + FLASH_DESCRIPTOR_UPPER_MAP_BASE, &upper, sizeof(upper));
    expect(formatFlashDescriptorVscc(vscc, info) == U_INVALID_FLASH_DESCRIPTOR
        && info.contains("overlaps upper map"), "VSCC may not overlap its upper map");
    upper.VsccTableBase = 0;
    upper.VsccTableSize = 2;
    std::memcpy(vscc.data() + FLASH_DESCRIPTOR_UPPER_MAP_BASE, &upper, sizeof(upper));
    expect(formatFlashDescriptorVscc(vscc, info) == U_INVALID_FLASH_DESCRIPTOR, "VSCC may not point into the descriptor header");
    upper.VsccTableBase = 0xFF;
    upper.VsccTableSize = 0;
    std::memcpy(vscc.data() + FLASH_DESCRIPTOR_UPPER_MAP_BASE, &upper, sizeof(upper));
    expect(formatFlashDescriptorVscc(vscc, info) == U_SUCCESS && info.contains("empty (zero table length)"),
        "zero-length VSCC is empty even when its unused base isFF");

    if (failures)
        std::cerr << failures << " descriptor access checks failed\n";
    else
        std::cout << "Descriptor access checks passed\n";
    return failures ? 1 : 0;
}

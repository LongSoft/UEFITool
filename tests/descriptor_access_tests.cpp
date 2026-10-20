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
    expect(modern.find("Desc  Yes   No") != std::string::npos, "modern descriptor is read-only");
    expect(modern.find("ME    Yes   No") != std::string::npos, "modern ME is read-only");
    expect(modern.find("live write-protection measurement") != std::string::npos,
        "file permissions are not presented as live hardware state");

    const UByteArray legacy = fixture(FLASH_DESCRIPTOR_VERSION_INVALID, 0, 0x03070000);
    expect(detect(legacy) == FlashDescriptorAccessLayout::Ambiguous,
        "an erased version marker does not prove legacy format");
    const std::string legacyInfo = format(legacy, FlashDescriptorAccessLayout::Legacy);
    expect(legacyInfo.find("BIOS: 07h 03h") != std::string::npos, "legacy access format preserved");
    expect(legacyInfo.find("ME    Yes   No") != std::string::npos, "legacy ME access preserved");

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

    if (failures)
        std::cerr << failures << " descriptor access checks failed\n";
    else
        std::cout << "Descriptor access checks passed\n";
    return failures ? 1 : 0;
}

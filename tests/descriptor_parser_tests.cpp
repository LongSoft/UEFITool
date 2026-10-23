#include "../common/ffsparser.h"
#include "../common/filesystem.h"
#include "../common/utility.h"

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

static void word(UByteArray& image, UINT32 offset, UINT32 value)
{
    std::memcpy(image.data() + offset, &value, sizeof(value));
}

static UByteArray fixture(UINT16 descriptorLimit = 0, UINT8 masterBase = 8)
{
    UByteArray image(0x10000, '\xFF');
    FLASH_DESCRIPTOR_HEADER header = {};
    header.Signature = FLASH_DESCRIPTOR_SIGNATURE;
    FLASH_DESCRIPTOR_MAP map = {};
    map.ComponentBase = 3;
    map.RegionBase = 4;
    map.MasterBase = masterBase;
    map.PchStrapsBase = 0x20;
    FLASH_DESCRIPTOR_COMPONENT_SECTION component = {};
    std::memcpy(image.data(), &header, sizeof(header));
    std::memcpy(image.data() + sizeof(header), &map, sizeof(map));
    std::memcpy(image.data() + 0x30, &component, sizeof(component));
    word(image, 0x40, static_cast<UINT32>(descriptorLimit) << 16);
    word(image, 0x44, 0x000F0008);
    for (UINT32 i = 2; i < (static_cast<UINT32>(masterBase) * 16 - 0x40) / 4; i++)
        word(image, 0x40 + i * 4, 0x00007FFF);
    word(image, masterBase * 16, 0x00A00F00);
    word(image, FLASH_DESCRIPTOR_UPPER_MAP_BASE, 0xC0FF00FF);

    EFI_FIRMWARE_VOLUME_HEADER volume = {};
    std::memcpy(&volume.FileSystemGuid, EFI_FIRMWARE_FILE_SYSTEM2_GUID.constData(), sizeof(EFI_GUID));
    volume.FvLength = 0x8000;
    volume.Signature = EFI_FV_SIGNATURE;
    volume.Attributes = EFI_FVB2_ERASE_POLARITY;
    volume.HeaderLength = sizeof(volume) + 2 * sizeof(EFI_FV_BLOCK_MAP_ENTRY);
    volume.Revision = 2;
    EFI_FV_BLOCK_MAP_ENTRY blocks[2] = {};
    blocks[0].NumBlocks = 8;
    blocks[0].Length = 0x1000;
    std::memcpy(image.data() + 0x8000, &volume, sizeof(volume));
    std::memcpy(image.data() + 0x8000 + sizeof(volume), blocks, sizeof(blocks));
    volume.Checksum = calculateChecksum16(reinterpret_cast<const UINT16*>(image.constData() + 0x8000), volume.HeaderLength);
    std::memcpy(image.data() + 0x8000, &volume, sizeof(volume));
    return image;
}

static UModelIndex node(const TreeModel& model, UINT8 subtype, const UModelIndex& parent = UModelIndex())
{
    for (int row = 0; row < model.rowCount(parent); row++) {
        const UModelIndex child = model.index(row, 0, parent);
        if (model.type(child) == Types::Region && model.subtype(child) == subtype)
            return child;
        const UModelIndex found = node(model, subtype, child);
        if (found.isValid())
            return found;
    }
    return UModelIndex();
}

static bool message(FfsParser& parser, const char* text)
{
    const auto messages = parser.getMessages();
    for (size_t i = 0; i < messages.size(); i++) {
        if (std::string(messages[i].first.toLocal8Bit()).find(text) != std::string::npos)
            return true;
    }
    return false;
}

static void validFixture(UByteArray image, UINT32 extent, bool extended)
{
    TreeModel model;
    FfsParser parser(&model);
    expect(parser.parse(image) == U_SUCCESS, "synthetic Intel image parses through production entry");
    expect(model.rowCount() == 1, "valid Intel image has exactly one root");
    const UModelIndex descriptor = node(model, Subtypes::DescriptorRegion);
    expect(descriptor.isValid(), "descriptor region exists");
    if (!descriptor.isValid())
        return;
    expect(model.body(descriptor).size() == extent, "descriptor node covers its entire declared extent");
    expect(model.base(descriptor) == 0, "raw descriptor base is zero");
    UString info = model.info(descriptor);
    expect(info.contains("bounded by following descriptor sections"), "table traversal basis is explicit");
    expect(info.contains("Descriptor access layout: ambiguous"), "traversal does not silently choose a permission layout");
    expect(info.contains("VSCC status: empty"), "zero-length VSCC is explicit");
    expect(node(model, Subtypes::DevExp2Region).isValid() == extended, "zero-clock DevExp2 is traversed only inside the region table");
    expect(node(model, Subtypes::IeRegion).isValid() == extended, "zero-clock IE slot is discovered without IPSE-content parsing");
    expect(!node(model, Subtypes::EcRegion).isValid(), "disabled EC base greater than limit is ignored");
    if (extended) {
        expect(model.base(node(model, Subtypes::DevExp2Region)) == 0x4000, "extended region offset preserved");
        expect(model.body(node(model, Subtypes::IeRegion)).size() == 0x1000, "extended region length preserved");
    }
    if (extent == 0x4000)
        expect(info.contains("1000h (identical to first block)"), "duplicate descriptor configuration block reported");
}

int main(int argc, char** argv)
{
    if (argc == 3 && std::string(argv[1]) == "--inspect") {
        UByteArray image;
        if (!readFileIntoBuffer(UString(argv[2]), image))
            return 2;
        TreeModel model;
        FfsParser parser(&model);
        const USTATUS status = parser.parse(image);
        const UModelIndex descriptor = node(model, Subtypes::DescriptorRegion);
        if (status != U_SUCCESS || !descriptor.isValid()) {
            std::cerr << "Parse status: " << status << '\n';
            return 3;
        }
        std::cout << model.info(descriptor).toLocal8Bit() << '\n';
        for (UINT8 i = Subtypes::DescriptorRegion; i <= Subtypes::PttRegion; i++) {
            const UModelIndex region = node(model, i);
            if (region.isValid())
                std::cout << "REGION " << static_cast<unsigned>(i) << " base=" << model.base(region)
                    << " size=" << model.body(region).size() << '\n';
        }
        return 0;
    }
    if (argc != 1)
        return 1;
    UByteArray image = fixture();
    validFixture(image, 0x1000, false);
    validFixture(fixture(0, 6), 0x1000, false);
    for (UINT8 masterBase = 0; masterBase < 3; masterBase++) {
        TreeModel model;
        FfsParser parser(&model);
        UByteArray malformed = fixture();
        FLASH_DESCRIPTOR_MAP map = {};
        std::memcpy(&map, malformed.constData() + sizeof(FLASH_DESCRIPTOR_HEADER), sizeof(map));
        map.MasterBase = masterBase;
        std::memcpy(malformed.data() + sizeof(FLASH_DESCRIPTOR_HEADER), &map, sizeof(map));
        expect(parser.parse(malformed) == U_SUCCESS, "header-overlapping master base falls back to UEFI parsing");
        const UModelIndex root = model.index(0, 0);
        expect(model.rowCount() == 1 && model.subtype(root) == Subtypes::UefiImage
            && model.rowCount(root) > 0, "invalid master base leaves one populated UEFI root, not an empty Intel root");
    }
    word(image, 0x64, 0x00040004);
    word(image, 0x68, 0x00050005);
    word(image, 0x60, 0x00060007);
    validFixture(image, 0x1000, true);

    image = fixture(3);
    word(image, 0x64, 0x00040004);
    word(image, 0x68, 0x00050005);
    std::memcpy(image.data() + 0x1000, image.constData(), FLASH_DESCRIPTOR_SIZE);
    validFixture(image, 0x4000, true);

    {
        UByteArray capsule(0x1000, '\0');
        EFI_CAPSULE_HEADER header = {};
        std::memcpy(&header.CapsuleGuid, EFI_CAPSULE_GUID.constData(), sizeof(EFI_GUID));
        header.HeaderSize = 0x1000;
        header.CapsuleImageSize = 0x1000 + image.size();
        std::memcpy(capsule.data(), &header, sizeof(header));
        capsule += image;
        TreeModel model;
        FfsParser parser(&model);
        expect(parser.parse(capsule) == U_SUCCESS, "capsule-wrapped descriptor parses");
        const UModelIndex descriptor = node(model, Subtypes::DescriptorRegion);
        expect(descriptor.isValid() && model.base(descriptor) == 0x1000,
            "capsule descriptor offset is not counted twice");
    }
    {
        TreeModel model;
        FfsParser parser(&model);
        UByteArray malformed = fixture(0x10);
        parser.parse(malformed);
        expect(!node(model, Subtypes::DescriptorRegion).isValid()
            && message(parser, "declared descriptor region extent"), "oversized descriptor extent rejected");
    }
    {
        TreeModel model;
        FfsParser parser(&model);
        UByteArray malformed = fixture(4);
        word(malformed, 0x64, 0x00040004);
        parser.parse(malformed);
        expect(!node(model, Subtypes::DescriptorRegion).isValid()
            && message(parser, "intersection with flash descriptor"), "region overlapping declared descriptor extent rejected");
    }
    {
        TreeModel model;
        FfsParser parser(&model);
        UByteArray malformed = fixture();
        word(malformed, 0x64, 0x00100010);
        parser.parse(malformed);
        expect(!node(model, Subtypes::DescriptorRegion).isValid()
            && message(parser, "outside of the opened image"), "out-of-image extended region rejected");
    }
    {
        TreeModel model;
        FfsParser parser(&model);
        UByteArray malformed = fixture();
        word(malformed, FLASH_DESCRIPTOR_UPPER_MAP_BASE, 0xC0FF08FF);
        expect(parser.parse(malformed) == U_SUCCESS, "malformed VSCC does not discard valid image geometry");
        const UModelIndex descriptor = node(model, Subtypes::DescriptorRegion);
        expect(descriptor.isValid() && model.info(descriptor).contains("VSCC status: invalid")
            && message(parser, "VSCC status: invalid"), "malformed VSCC is surfaced in pane and parser messages");
    }
    {
        TreeModel model;
        FfsParser parser(&model);
        UByteArray malformed = fixture();
        malformed[0x16] = 1;
        parser.parse(malformed);
        expect(!node(model, Subtypes::DescriptorRegion).isValid(), "region table pointing inside header is rejected");
    }
    if (failures)
        std::cerr << failures << " descriptor parser checks failed\n";
    else
        std::cout << "Descriptor parser checks passed\n";
    return failures ? 1 : 0;
}

#pragma once

// This is a generated file! Please edit source .ksy file and use kaitai-struct-compiler to rebuild

#include "../kaitai/kaitaistruct.h"
#include <stdint.h>
#include <memory>
#include <vector>

#if KAITAI_STRUCT_VERSION < 9000L
#error "Incompatible Kaitai Struct C++/STL API: version 0.9 or later is required"
#endif

class phoenix_flm_t : public kaitai::kstruct {

public:
    class flm_entry_t;

    phoenix_flm_t(kaitai::kstream* p__io, kaitai::kstruct* p__parent = nullptr, phoenix_flm_t* p__root = nullptr);

private:
    void _read();
    void _clean_up();

public:
    ~phoenix_flm_t();

    class flm_entry_t : public kaitai::kstruct {

    public:

        flm_entry_t(kaitai::kstream* p__io, phoenix_flm_t* p__parent = nullptr, phoenix_flm_t* p__root = nullptr);

    private:
        void _read();
        void _clean_up();

    public:
        ~flm_entry_t();

    private:
        std::string m_guid;
        uint16_t m_data_type;
        uint16_t m_entry_type;
        uint64_t m_physical_address;
        uint32_t m_size;
        uint32_t m_offset;
        phoenix_flm_t* m__root;
        phoenix_flm_t* m__parent;

    public:
        std::string guid() const { return m_guid; }
        uint16_t data_type() const { return m_data_type; }
        uint16_t entry_type() const { return m_entry_type; }
        uint64_t physical_address() const { return m_physical_address; }
        uint32_t size() const { return m_size; }
        uint32_t offset() const { return m_offset; }
        phoenix_flm_t* _root() const { return m__root; }
        phoenix_flm_t* _parent() const { return m__parent; }
    };

private:
    bool f_len_flm_store_header;
    int8_t m_len_flm_store_header;

public:
    int8_t len_flm_store_header();

private:
    bool f_len_flm_entry;
    int8_t m_len_flm_entry;

public:
    int8_t len_flm_entry();

private:
    std::string m_signature;
    uint16_t m_num_entries;
    uint32_t m_reserved;
    std::unique_ptr<std::vector<std::unique_ptr<flm_entry_t>>> m_entries;
    phoenix_flm_t* m__root;
    kaitai::kstruct* m__parent;

public:
    std::string signature() const { return m_signature; }
    uint16_t num_entries() const { return m_num_entries; }
    uint32_t reserved() const { return m_reserved; }
    std::vector<std::unique_ptr<flm_entry_t>>* entries() const { return m_entries.get(); }
    phoenix_flm_t* _root() const { return m__root; }
    kaitai::kstruct* _parent() const { return m__parent; }
};

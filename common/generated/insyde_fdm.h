#pragma once

// This is a generated file! Please edit source .ksy file and use kaitai-struct-compiler to rebuild

#include "../kaitai/kaitaistruct.h"
#include <stdint.h>
#include <memory>
#include <vector>

#if KAITAI_STRUCT_VERSION < 9000L
#error "Incompatible Kaitai Struct C++/STL API: version 0.9 or later is required"
#endif

class insyde_fdm_t : public kaitai::kstruct {

public:
    class fdm_entries_t;
    class fdm_extension_t;
    class fdm_board_ids_t;
    class fdm_extensions_t;
    class fdm_entry_t;

    insyde_fdm_t(kaitai::kstream* p__io, kaitai::kstruct* p__parent = nullptr, insyde_fdm_t* p__root = nullptr);

private:
    void _read();
    void _clean_up();

public:
    ~insyde_fdm_t();

    class fdm_entries_t : public kaitai::kstruct {

    public:

        fdm_entries_t(kaitai::kstream* p__io, insyde_fdm_t* p__parent = nullptr, insyde_fdm_t* p__root = nullptr);

    private:
        void _read();
        void _clean_up();

    public:
        ~fdm_entries_t();

    private:
        std::unique_ptr<std::vector<std::unique_ptr<fdm_entry_t>>> m_entries;
        insyde_fdm_t* m__root;
        insyde_fdm_t* m__parent;

    public:
        std::vector<std::unique_ptr<fdm_entry_t>>* entries() const { return m_entries.get(); }
        insyde_fdm_t* _root() const { return m__root; }
        insyde_fdm_t* _parent() const { return m__parent; }
    };

    class fdm_extension_t : public kaitai::kstruct {

    public:

        fdm_extension_t(kaitai::kstream* p__io, insyde_fdm_t::fdm_extensions_t* p__parent = nullptr, insyde_fdm_t* p__root = nullptr);

    private:
        void _read();
        void _clean_up();

    public:
        ~fdm_extension_t();

    private:
        uint16_t m_offset;
        uint16_t m_count;
        insyde_fdm_t* m__root;
        insyde_fdm_t::fdm_extensions_t* m__parent;

    public:
        uint16_t offset() const { return m_offset; }
        uint16_t count() const { return m_count; }
        insyde_fdm_t* _root() const { return m__root; }
        insyde_fdm_t::fdm_extensions_t* _parent() const { return m__parent; }
    };

    class fdm_board_ids_t : public kaitai::kstruct {

    public:

        fdm_board_ids_t(kaitai::kstream* p__io, insyde_fdm_t* p__parent = nullptr, insyde_fdm_t* p__root = nullptr);

    private:
        void _read();
        void _clean_up();

    public:
        ~fdm_board_ids_t();

    private:
        uint32_t m_region_index;
        uint32_t m_num_board_ids;
        std::unique_ptr<std::vector<uint64_t>> m_board_ids;
        insyde_fdm_t* m__root;
        insyde_fdm_t* m__parent;

    public:
        uint32_t region_index() const { return m_region_index; }
        uint32_t num_board_ids() const { return m_num_board_ids; }
        std::vector<uint64_t>* board_ids() const { return m_board_ids.get(); }
        insyde_fdm_t* _root() const { return m__root; }
        insyde_fdm_t* _parent() const { return m__parent; }
    };

    class fdm_extensions_t : public kaitai::kstruct {

    public:

        fdm_extensions_t(kaitai::kstream* p__io, insyde_fdm_t* p__parent = nullptr, insyde_fdm_t* p__root = nullptr);

    private:
        void _read();
        void _clean_up();

    public:
        ~fdm_extensions_t();

    private:
        std::unique_ptr<std::vector<std::unique_ptr<fdm_extension_t>>> m_extensions;
        insyde_fdm_t* m__root;
        insyde_fdm_t* m__parent;

    public:
        std::vector<std::unique_ptr<fdm_extension_t>>* extensions() const { return m_extensions.get(); }
        insyde_fdm_t* _root() const { return m__root; }
        insyde_fdm_t* _parent() const { return m__parent; }
    };

    class fdm_entry_t : public kaitai::kstruct {

    public:

        fdm_entry_t(kaitai::kstream* p__io, insyde_fdm_t::fdm_entries_t* p__parent = nullptr, insyde_fdm_t* p__root = nullptr);

    private:
        void _read();
        void _clean_up();

    public:
        ~fdm_entry_t();

    private:
        bool f_region_base;
        int32_t m_region_base;

    public:
        int32_t region_base();

    private:
        std::string m_guid;
        std::string m_region_id;
        uint64_t m_region_offset;
        uint64_t m_region_size;
        uint32_t m_attributes;
        std::string m_hash;
        insyde_fdm_t* m__root;
        insyde_fdm_t::fdm_entries_t* m__parent;

    public:
        std::string guid() const { return m_guid; }
        std::string region_id() const { return m_region_id; }
        uint64_t region_offset() const { return m_region_offset; }
        uint64_t region_size() const { return m_region_size; }
        uint32_t attributes() const { return m_attributes; }
        std::string hash() const { return m_hash; }
        insyde_fdm_t* _root() const { return m__root; }
        insyde_fdm_t::fdm_entries_t* _parent() const { return m__parent; }
    };

private:
    uint32_t m_signature;
    uint32_t m_store_size;
    uint32_t m_data_offset;
    uint32_t m_entry_size;
    uint8_t m_entry_format;
    uint8_t m_revision;
    uint8_t m_num_extensions;
    uint8_t m_checksum;
    uint64_t m_fd_base_address;
    std::unique_ptr<fdm_extensions_t> m_extensions;
    bool n_extensions;

public:
    bool _is_null_extensions() { extensions(); return n_extensions; };

private:
    std::unique_ptr<fdm_board_ids_t> m_board_ids;
    bool n_board_ids;

public:
    bool _is_null_board_ids() { board_ids(); return n_board_ids; };

private:
    std::unique_ptr<fdm_entries_t> m_entries;
    insyde_fdm_t* m__root;
    kaitai::kstruct* m__parent;
    std::string m__raw_extensions;
    bool n__raw_extensions;

public:
    bool _is_null__raw_extensions() { _raw_extensions(); return n__raw_extensions; };

private:
    std::unique_ptr<kaitai::kstream> m__io__raw_extensions;
    std::string m__raw_entries;
    std::unique_ptr<kaitai::kstream> m__io__raw_entries;

public:
    uint32_t signature() const { return m_signature; }
    uint32_t store_size() const { return m_store_size; }
    uint32_t data_offset() const { return m_data_offset; }
    uint32_t entry_size() const { return m_entry_size; }
    uint8_t entry_format() const { return m_entry_format; }
    uint8_t revision() const { return m_revision; }
    uint8_t num_extensions() const { return m_num_extensions; }
    uint8_t checksum() const { return m_checksum; }
    uint64_t fd_base_address() const { return m_fd_base_address; }
    fdm_extensions_t* extensions() const { return m_extensions.get(); }
    fdm_board_ids_t* board_ids() const { return m_board_ids.get(); }
    fdm_entries_t* entries() const { return m_entries.get(); }
    insyde_fdm_t* _root() const { return m__root; }
    kaitai::kstruct* _parent() const { return m__parent; }
    std::string _raw_extensions() const { return m__raw_extensions; }
    kaitai::kstream* _io__raw_extensions() const { return m__io__raw_extensions.get(); }
    std::string _raw_entries() const { return m__raw_entries; }
    kaitai::kstream* _io__raw_entries() const { return m__io__raw_entries.get(); }
};

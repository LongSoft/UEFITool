#pragma once

// This is a generated file! Please edit source .ksy file and use kaitai-struct-compiler to rebuild

#include "../kaitai/kaitaistruct.h"
#include <stdint.h>
#include <memory>
#include <vector>

#if KAITAI_STRUCT_VERSION < 9000L
#error "Incompatible Kaitai Struct C++/STL API: version 0.9 or later is required"
#endif

class phoenix_evsa_t : public kaitai::kstruct {

public:
    class evsa_entry_t;
    class evsa_unknown_t;
    class evsa_body_t;
    class evsa_name_t;
    class evsa_guid_t;
    class evsa_variable_attributes_t;
    class evsa_data_t;

    phoenix_evsa_t(kaitai::kstream* p__io, kaitai::kstruct* p__parent = nullptr, phoenix_evsa_t* p__root = nullptr);

private:
    void _read();
    void _clean_up();

public:
    ~phoenix_evsa_t();

    class evsa_entry_t : public kaitai::kstruct {

    public:

        evsa_entry_t(kaitai::kstream* p__io, phoenix_evsa_t::evsa_body_t* p__parent = nullptr, phoenix_evsa_t* p__root = nullptr);

    private:
        void _read();
        void _clean_up();

    public:
        ~evsa_entry_t();

    private:
        uint8_t m_entry_type;
        uint8_t m_checksum;
        bool n_checksum;

    public:
        bool _is_null_checksum() { checksum(); return n_checksum; };

    private:
        uint16_t m_len_evsa_entry;
        bool n_len_evsa_entry;

    public:
        bool _is_null_len_evsa_entry() { len_evsa_entry(); return n_len_evsa_entry; };

    private:
        std::unique_ptr<kaitai::kstruct> m_body;
        phoenix_evsa_t* m__root;
        phoenix_evsa_t::evsa_body_t* m__parent;

    public:
        uint8_t entry_type() const { return m_entry_type; }
        uint8_t checksum() const { return m_checksum; }
        uint16_t len_evsa_entry() const { return m_len_evsa_entry; }
        kaitai::kstruct* body() const { return m_body.get(); }
        phoenix_evsa_t* _root() const { return m__root; }
        phoenix_evsa_t::evsa_body_t* _parent() const { return m__parent; }
    };

    class evsa_unknown_t : public kaitai::kstruct {

    public:

        evsa_unknown_t(kaitai::kstream* p__io, phoenix_evsa_t::evsa_entry_t* p__parent = nullptr, phoenix_evsa_t* p__root = nullptr);

    private:
        void _read();
        void _clean_up();

    public:
        ~evsa_unknown_t();

    private:
        std::string m_unknown;
        phoenix_evsa_t* m__root;
        phoenix_evsa_t::evsa_entry_t* m__parent;

    public:
        std::string unknown() const { return m_unknown; }
        phoenix_evsa_t* _root() const { return m__root; }
        phoenix_evsa_t::evsa_entry_t* _parent() const { return m__parent; }
    };

    class evsa_body_t : public kaitai::kstruct {

    public:

        evsa_body_t(kaitai::kstream* p__io, phoenix_evsa_t* p__parent = nullptr, phoenix_evsa_t* p__root = nullptr);

    private:
        void _read();
        void _clean_up();

    public:
        ~evsa_body_t();

    private:
        std::unique_ptr<std::vector<std::unique_ptr<evsa_entry_t>>> m_entries;
        std::unique_ptr<std::vector<uint8_t>> m_free_space;
        phoenix_evsa_t* m__root;
        phoenix_evsa_t* m__parent;

    public:
        std::vector<std::unique_ptr<evsa_entry_t>>* entries() const { return m_entries.get(); }
        std::vector<uint8_t>* free_space() const { return m_free_space.get(); }
        phoenix_evsa_t* _root() const { return m__root; }
        phoenix_evsa_t* _parent() const { return m__parent; }
    };

    class evsa_name_t : public kaitai::kstruct {

    public:

        evsa_name_t(kaitai::kstream* p__io, phoenix_evsa_t::evsa_entry_t* p__parent = nullptr, phoenix_evsa_t* p__root = nullptr);

    private:
        void _read();
        void _clean_up();

    public:
        ~evsa_name_t();

    private:
        uint16_t m_var_id;
        std::string m_name;
        phoenix_evsa_t* m__root;
        phoenix_evsa_t::evsa_entry_t* m__parent;

    public:
        uint16_t var_id() const { return m_var_id; }
        std::string name() const { return m_name; }
        phoenix_evsa_t* _root() const { return m__root; }
        phoenix_evsa_t::evsa_entry_t* _parent() const { return m__parent; }
    };

    class evsa_guid_t : public kaitai::kstruct {

    public:

        evsa_guid_t(kaitai::kstream* p__io, phoenix_evsa_t::evsa_entry_t* p__parent = nullptr, phoenix_evsa_t* p__root = nullptr);

    private:
        void _read();
        void _clean_up();

    public:
        ~evsa_guid_t();

    private:
        uint16_t m_guid_id;
        std::string m_guid;
        phoenix_evsa_t* m__root;
        phoenix_evsa_t::evsa_entry_t* m__parent;

    public:
        uint16_t guid_id() const { return m_guid_id; }
        std::string guid() const { return m_guid; }
        phoenix_evsa_t* _root() const { return m__root; }
        phoenix_evsa_t::evsa_entry_t* _parent() const { return m__parent; }
    };

    class evsa_variable_attributes_t : public kaitai::kstruct {

    public:

        evsa_variable_attributes_t(kaitai::kstream* p__io, phoenix_evsa_t::evsa_data_t* p__parent = nullptr, phoenix_evsa_t* p__root = nullptr);

    private:
        void _read();
        void _clean_up();

    public:
        ~evsa_variable_attributes_t();

    private:
        bool m_non_volatile;
        bool m_boot_service;
        bool m_runtime;
        bool m_hw_error_record;
        bool m_auth_write;
        bool m_time_based_auth;
        bool m_append_write;
        uint64_t m_reserved;
        bool m_extended_header;
        uint64_t m_reserved1;
        phoenix_evsa_t* m__root;
        phoenix_evsa_t::evsa_data_t* m__parent;

    public:
        bool non_volatile() const { return m_non_volatile; }
        bool boot_service() const { return m_boot_service; }
        bool runtime() const { return m_runtime; }
        bool hw_error_record() const { return m_hw_error_record; }
        bool auth_write() const { return m_auth_write; }
        bool time_based_auth() const { return m_time_based_auth; }
        bool append_write() const { return m_append_write; }
        uint64_t reserved() const { return m_reserved; }
        bool extended_header() const { return m_extended_header; }
        uint64_t reserved1() const { return m_reserved1; }
        phoenix_evsa_t* _root() const { return m__root; }
        phoenix_evsa_t::evsa_data_t* _parent() const { return m__parent; }
    };

    class evsa_data_t : public kaitai::kstruct {

    public:

        evsa_data_t(kaitai::kstream* p__io, phoenix_evsa_t::evsa_entry_t* p__parent = nullptr, phoenix_evsa_t* p__root = nullptr);

    private:
        void _read();
        void _clean_up();

    public:
        ~evsa_data_t();

    private:
        uint16_t m_guid_id;
        uint16_t m_var_id;
        std::unique_ptr<evsa_variable_attributes_t> m_attributes;
        uint32_t m_len_data_ext;
        bool n_len_data_ext;

    public:
        bool _is_null_len_data_ext() { len_data_ext(); return n_len_data_ext; };

    private:
        std::string m_data;
        bool n_data;

    public:
        bool _is_null_data() { data(); return n_data; };

    private:
        std::string m_data_ext;
        bool n_data_ext;

    public:
        bool _is_null_data_ext() { data_ext(); return n_data_ext; };

    private:
        phoenix_evsa_t* m__root;
        phoenix_evsa_t::evsa_entry_t* m__parent;

    public:
        uint16_t guid_id() const { return m_guid_id; }
        uint16_t var_id() const { return m_var_id; }
        evsa_variable_attributes_t* attributes() const { return m_attributes.get(); }
        uint32_t len_data_ext() const { return m_len_data_ext; }
        std::string data() const { return m_data; }
        std::string data_ext() const { return m_data_ext; }
        phoenix_evsa_t* _root() const { return m__root; }
        phoenix_evsa_t::evsa_entry_t* _parent() const { return m__parent; }
    };

private:
    uint8_t m_type;
    uint8_t m_checksum;
    uint16_t m_len_evsa_store_header;
    uint32_t m_signature;
    uint32_t m_attributes;
    uint32_t m_len_evsa_store;
    uint32_t m_reserved;
    std::unique_ptr<evsa_body_t> m_body;
    phoenix_evsa_t* m__root;
    kaitai::kstruct* m__parent;
    std::string m__raw_body;
    std::unique_ptr<kaitai::kstream> m__io__raw_body;

public:
    uint8_t type() const { return m_type; }
    uint8_t checksum() const { return m_checksum; }
    uint16_t len_evsa_store_header() const { return m_len_evsa_store_header; }
    uint32_t signature() const { return m_signature; }
    uint32_t attributes() const { return m_attributes; }
    uint32_t len_evsa_store() const { return m_len_evsa_store; }
    uint32_t reserved() const { return m_reserved; }
    evsa_body_t* body() const { return m_body.get(); }
    phoenix_evsa_t* _root() const { return m__root; }
    kaitai::kstruct* _parent() const { return m__parent; }
    std::string _raw_body() const { return m__raw_body; }
    kaitai::kstream* _io__raw_body() const { return m__io__raw_body.get(); }
};

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
        uint8_t m_type;
        uint8_t m_checksum;
        uint16_t m_size;
        std::unique_ptr<kaitai::kstruct> m_body;
        phoenix_evsa_t* m__root;
        phoenix_evsa_t::evsa_body_t* m__parent;

    public:
        uint8_t type() const { return m_type; }
        uint8_t checksum() const { return m_checksum; }
        uint16_t size() const { return m_size; }
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
        phoenix_evsa_t* m__root;
        phoenix_evsa_t* m__parent;

    public:
        std::vector<std::unique_ptr<evsa_entry_t>>* entries() const { return m_entries.get(); }
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
        uint32_t m_attributes;
        uint32_t m_data_size;
        bool n_data_size;

    public:
        bool _is_null_data_size() { data_size(); return n_data_size; };

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
        uint32_t attributes() const { return m_attributes; }
        uint32_t data_size() const { return m_data_size; }
        std::string data() const { return m_data; }
        std::string data_ext() const { return m_data_ext; }
        phoenix_evsa_t* _root() const { return m__root; }
        phoenix_evsa_t::evsa_entry_t* _parent() const { return m__parent; }
    };

private:
    uint8_t m_type;
    uint8_t m_checksum;
    uint16_t m_size;
    uint32_t m_signature;
    uint32_t m_attributes;
    uint32_t m_store_size;
    uint32_t m_reserved;
    std::unique_ptr<evsa_body_t> m_body;
    phoenix_evsa_t* m__root;
    kaitai::kstruct* m__parent;
    std::string m__raw_body;
    std::unique_ptr<kaitai::kstream> m__io__raw_body;

public:
    uint8_t type() const { return m_type; }
    uint8_t checksum() const { return m_checksum; }
    uint16_t size() const { return m_size; }
    uint32_t signature() const { return m_signature; }
    uint32_t attributes() const { return m_attributes; }
    uint32_t store_size() const { return m_store_size; }
    uint32_t reserved() const { return m_reserved; }
    evsa_body_t* body() const { return m_body.get(); }
    phoenix_evsa_t* _root() const { return m__root; }
    kaitai::kstruct* _parent() const { return m__parent; }
    std::string _raw_body() const { return m__raw_body; }
    kaitai::kstream* _io__raw_body() const { return m__io__raw_body.get(); }
};

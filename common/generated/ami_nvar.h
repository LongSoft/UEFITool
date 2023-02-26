#pragma once

// This is a generated file! Please edit source .ksy file and use kaitai-struct-compiler to rebuild

#include "../kaitai/kaitaistruct.h"
#include <stdint.h>
#include <memory>
#include <vector>

#if KAITAI_STRUCT_VERSION < 9000L
#error "Incompatible Kaitai Struct C++/STL API: version 0.9 or later is required"
#endif

class ami_nvar_t : public kaitai::kstruct {

public:
    class nvar_attributes_t;
    class ucs2_string_t;
    class nvar_extended_attributes_t;
    class nvar_entry_t;
    class nvar_entry_body_t;

    ami_nvar_t(kaitai::kstream* p__io, kaitai::kstruct* p__parent = nullptr, ami_nvar_t* p__root = nullptr);

private:
    void _read();
    void _clean_up();

public:
    ~ami_nvar_t();

    class nvar_attributes_t : public kaitai::kstruct {

    public:

        nvar_attributes_t(kaitai::kstream* p__io, ami_nvar_t::nvar_entry_t* p__parent = nullptr, ami_nvar_t* p__root = nullptr);

    private:
        void _read();
        void _clean_up();

    public:
        ~nvar_attributes_t();

    private:
        bool m_valid;
        bool m_auth_write;
        bool m_hw_error_record;
        bool m_extended_header;
        bool m_data_only;
        bool m_local_guid;
        bool m_ascii_name;
        bool m_runtime;
        ami_nvar_t* m__root;
        ami_nvar_t::nvar_entry_t* m__parent;

    public:
        bool valid() const { return m_valid; }
        bool auth_write() const { return m_auth_write; }
        bool hw_error_record() const { return m_hw_error_record; }
        bool extended_header() const { return m_extended_header; }
        bool data_only() const { return m_data_only; }
        bool local_guid() const { return m_local_guid; }
        bool ascii_name() const { return m_ascii_name; }
        bool runtime() const { return m_runtime; }
        ami_nvar_t* _root() const { return m__root; }
        ami_nvar_t::nvar_entry_t* _parent() const { return m__parent; }
    };

    class ucs2_string_t : public kaitai::kstruct {

    public:

        ucs2_string_t(kaitai::kstream* p__io, ami_nvar_t::nvar_entry_body_t* p__parent = nullptr, ami_nvar_t* p__root = nullptr);

    private:
        void _read();
        void _clean_up();

    public:
        ~ucs2_string_t();

    private:
        std::unique_ptr<std::vector<uint16_t>> m_ucs2_chars;
        ami_nvar_t* m__root;
        ami_nvar_t::nvar_entry_body_t* m__parent;

    public:
        std::vector<uint16_t>* ucs2_chars() const { return m_ucs2_chars.get(); }
        ami_nvar_t* _root() const { return m__root; }
        ami_nvar_t::nvar_entry_body_t* _parent() const { return m__parent; }
    };

    class nvar_extended_attributes_t : public kaitai::kstruct {

    public:

        nvar_extended_attributes_t(kaitai::kstream* p__io, ami_nvar_t::nvar_entry_body_t* p__parent = nullptr, ami_nvar_t* p__root = nullptr);

    private:
        void _read();
        void _clean_up();

    public:
        ~nvar_extended_attributes_t();

    private:
        uint64_t m_reserved_high;
        bool m_time_based_auth;
        bool m_auth_write;
        uint64_t m_reserved_low;
        bool m_checksum;
        ami_nvar_t* m__root;
        ami_nvar_t::nvar_entry_body_t* m__parent;

    public:
        uint64_t reserved_high() const { return m_reserved_high; }
        bool time_based_auth() const { return m_time_based_auth; }
        bool auth_write() const { return m_auth_write; }
        uint64_t reserved_low() const { return m_reserved_low; }
        bool checksum() const { return m_checksum; }
        ami_nvar_t* _root() const { return m__root; }
        ami_nvar_t::nvar_entry_body_t* _parent() const { return m__parent; }
    };

    class nvar_entry_t : public kaitai::kstruct {

    public:

        nvar_entry_t(kaitai::kstream* p__io, ami_nvar_t* p__parent = nullptr, ami_nvar_t* p__root = nullptr);

    private:
        void _read();
        void _clean_up();

    public:
        ~nvar_entry_t();

    private:
        bool f_offset;
        int32_t m_offset;

    public:
        int32_t offset();

    private:
        bool f_end_offset;
        int32_t m_end_offset;

    public:
        int32_t end_offset();

    private:
        std::string m_invoke_offset;
        bool n_invoke_offset;

    public:
        bool _is_null_invoke_offset() { invoke_offset(); return n_invoke_offset; };

    private:
        uint8_t m_signature_first;
        std::string m_signature_rest;
        bool n_signature_rest;

    public:
        bool _is_null_signature_rest() { signature_rest(); return n_signature_rest; };

    private:
        uint16_t m_size;
        bool n_size;

    public:
        bool _is_null_size() { size(); return n_size; };

    private:
        uint64_t m_next;
        bool n_next;

    public:
        bool _is_null_next() { next(); return n_next; };

    private:
        std::unique_ptr<nvar_attributes_t> m_attributes;
        bool n_attributes;

    public:
        bool _is_null_attributes() { attributes(); return n_attributes; };

    private:
        std::unique_ptr<nvar_entry_body_t> m_body;
        bool n_body;

    public:
        bool _is_null_body() { body(); return n_body; };

    private:
        std::string m_invoke_end_offset;
        bool n_invoke_end_offset;

    public:
        bool _is_null_invoke_end_offset() { invoke_end_offset(); return n_invoke_end_offset; };

    private:
        ami_nvar_t* m__root;
        ami_nvar_t* m__parent;
        std::string m__raw_body;
        bool n__raw_body;

    public:
        bool _is_null__raw_body() { _raw_body(); return n__raw_body; };

    private:
        std::unique_ptr<kaitai::kstream> m__io__raw_body;

    public:
        std::string invoke_offset() const { return m_invoke_offset; }
        uint8_t signature_first() const { return m_signature_first; }
        std::string signature_rest() const { return m_signature_rest; }
        uint16_t size() const { return m_size; }
        uint64_t next() const { return m_next; }
        nvar_attributes_t* attributes() const { return m_attributes.get(); }
        nvar_entry_body_t* body() const { return m_body.get(); }
        std::string invoke_end_offset() const { return m_invoke_end_offset; }
        ami_nvar_t* _root() const { return m__root; }
        ami_nvar_t* _parent() const { return m__parent; }
        std::string _raw_body() const { return m__raw_body; }
        kaitai::kstream* _io__raw_body() const { return m__io__raw_body.get(); }
    };

    class nvar_entry_body_t : public kaitai::kstruct {

    public:

        nvar_entry_body_t(kaitai::kstream* p__io, ami_nvar_t::nvar_entry_t* p__parent = nullptr, ami_nvar_t* p__root = nullptr);

    private:
        void _read();
        void _clean_up();

    public:
        ~nvar_entry_body_t();

    private:
        bool f_extended_header_attributes;
        std::unique_ptr<nvar_extended_attributes_t> m_extended_header_attributes;
        bool n_extended_header_attributes;

    public:
        bool _is_null_extended_header_attributes() { extended_header_attributes(); return n_extended_header_attributes; };

    private:

    public:
        nvar_extended_attributes_t* extended_header_attributes();

    private:
        bool f_data_start_offset;
        int32_t m_data_start_offset;

    public:
        int32_t data_start_offset();

    private:
        bool f_extended_header_size_field;
        uint16_t m_extended_header_size_field;
        bool n_extended_header_size_field;

    public:
        bool _is_null_extended_header_size_field() { extended_header_size_field(); return n_extended_header_size_field; };

    private:

    public:
        uint16_t extended_header_size_field();

    private:
        bool f_extended_header_timestamp;
        uint64_t m_extended_header_timestamp;
        bool n_extended_header_timestamp;

    public:
        bool _is_null_extended_header_timestamp() { extended_header_timestamp(); return n_extended_header_timestamp; };

    private:

    public:
        uint64_t extended_header_timestamp();

    private:
        bool f_data_size;
        int32_t m_data_size;

    public:
        int32_t data_size();

    private:
        bool f_extended_header_checksum;
        uint8_t m_extended_header_checksum;
        bool n_extended_header_checksum;

    public:
        bool _is_null_extended_header_checksum() { extended_header_checksum(); return n_extended_header_checksum; };

    private:

    public:
        uint8_t extended_header_checksum();

    private:
        bool f_data_end_offset;
        int32_t m_data_end_offset;

    public:
        int32_t data_end_offset();

    private:
        bool f_extended_header_size;
        uint16_t m_extended_header_size;

    public:
        uint16_t extended_header_size();

    private:
        bool f_extended_header_hash;
        std::string m_extended_header_hash;
        bool n_extended_header_hash;

    public:
        bool _is_null_extended_header_hash() { extended_header_hash(); return n_extended_header_hash; };

    private:

    public:
        std::string extended_header_hash();

    private:
        uint8_t m_guid_index;
        bool n_guid_index;

    public:
        bool _is_null_guid_index() { guid_index(); return n_guid_index; };

    private:
        std::string m_guid;
        bool n_guid;

    public:
        bool _is_null_guid() { guid(); return n_guid; };

    private:
        std::string m_ascii_name;
        bool n_ascii_name;

    public:
        bool _is_null_ascii_name() { ascii_name(); return n_ascii_name; };

    private:
        std::unique_ptr<ucs2_string_t> m_ucs2_name;
        bool n_ucs2_name;

    public:
        bool _is_null_ucs2_name() { ucs2_name(); return n_ucs2_name; };

    private:
        std::string m_invoke_data_start;
        bool n_invoke_data_start;

    public:
        bool _is_null_invoke_data_start() { invoke_data_start(); return n_invoke_data_start; };

    private:
        std::string m_data;
        ami_nvar_t* m__root;
        ami_nvar_t::nvar_entry_t* m__parent;

    public:
        uint8_t guid_index() const { return m_guid_index; }
        std::string guid() const { return m_guid; }
        std::string ascii_name() const { return m_ascii_name; }
        ucs2_string_t* ucs2_name() const { return m_ucs2_name.get(); }
        std::string invoke_data_start() const { return m_invoke_data_start; }
        std::string data() const { return m_data; }
        ami_nvar_t* _root() const { return m__root; }
        ami_nvar_t::nvar_entry_t* _parent() const { return m__parent; }
    };

private:
    std::unique_ptr<std::vector<std::unique_ptr<nvar_entry_t>>> m_entries;
    ami_nvar_t* m__root;
    kaitai::kstruct* m__parent;

public:
    std::vector<std::unique_ptr<nvar_entry_t>>* entries() const { return m_entries.get(); }
    ami_nvar_t* _root() const { return m__root; }
    kaitai::kstruct* _parent() const { return m__parent; }
};

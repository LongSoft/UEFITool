#pragma once

// This is a generated file! Please edit source .ksy file and use kaitai-struct-compiler to rebuild

#include "../kaitai/kaitaistruct.h"
#include <stdint.h>
#include <memory>
#include <vector>

#if KAITAI_STRUCT_VERSION < 9000L
#error "Incompatible Kaitai Struct C++/STL API: version 0.9 or later is required"
#endif

class apple_sysf_t : public kaitai::kstruct {

public:
    class sysf_store_body_t;
    class sysf_variable_t;

    apple_sysf_t(kaitai::kstream* p__io, kaitai::kstruct* p__parent = nullptr, apple_sysf_t* p__root = nullptr);

private:
    void _read();
    void _clean_up();

public:
    ~apple_sysf_t();

    class sysf_store_body_t : public kaitai::kstruct {

    public:

        sysf_store_body_t(kaitai::kstream* p__io, apple_sysf_t* p__parent = nullptr, apple_sysf_t* p__root = nullptr);

    private:
        void _read();
        void _clean_up();

    public:
        ~sysf_store_body_t();

    private:
        std::unique_ptr<std::vector<std::unique_ptr<sysf_variable_t>>> m_variables;
        std::unique_ptr<std::vector<uint8_t>> m_zeroes;
        apple_sysf_t* m__root;
        apple_sysf_t* m__parent;

    public:
        std::vector<std::unique_ptr<sysf_variable_t>>* variables() const { return m_variables.get(); }
        std::vector<uint8_t>* zeroes() const { return m_zeroes.get(); }
        apple_sysf_t* _root() const { return m__root; }
        apple_sysf_t* _parent() const { return m__parent; }
    };

    class sysf_variable_t : public kaitai::kstruct {

    public:

        sysf_variable_t(kaitai::kstream* p__io, apple_sysf_t::sysf_store_body_t* p__parent = nullptr, apple_sysf_t* p__root = nullptr);

    private:
        void _read();
        void _clean_up();

    public:
        ~sysf_variable_t();

    private:
        uint64_t m_len_name;
        bool m_invalid_flag;
        std::string m_name;
        uint16_t m_len_data;
        bool n_len_data;

    public:
        bool _is_null_len_data() { len_data(); return n_len_data; };

    private:
        std::string m_data;
        bool n_data;

    public:
        bool _is_null_data() { data(); return n_data; };

    private:
        apple_sysf_t* m__root;
        apple_sysf_t::sysf_store_body_t* m__parent;

    public:
        uint64_t len_name() const { return m_len_name; }
        bool invalid_flag() const { return m_invalid_flag; }
        std::string name() const { return m_name; }
        uint16_t len_data() const { return m_len_data; }
        std::string data() const { return m_data; }
        apple_sysf_t* _root() const { return m__root; }
        apple_sysf_t::sysf_store_body_t* _parent() const { return m__parent; }
    };

private:
    bool f_len_sysf_store_header;
    int8_t m_len_sysf_store_header;

public:
    int8_t len_sysf_store_header();

private:
    uint32_t m_signature;
    uint8_t m_unknown;
    uint32_t m_unknown1;
    uint16_t m_sysf_size;
    std::unique_ptr<sysf_store_body_t> m_body;
    uint32_t m_crc;
    apple_sysf_t* m__root;
    kaitai::kstruct* m__parent;
    std::string m__raw_body;
    std::unique_ptr<kaitai::kstream> m__io__raw_body;

public:
    uint32_t signature() const { return m_signature; }
    uint8_t unknown() const { return m_unknown; }
    uint32_t unknown1() const { return m_unknown1; }
    uint16_t sysf_size() const { return m_sysf_size; }
    sysf_store_body_t* body() const { return m_body.get(); }
    uint32_t crc() const { return m_crc; }
    apple_sysf_t* _root() const { return m__root; }
    kaitai::kstruct* _parent() const { return m__parent; }
    std::string _raw_body() const { return m__raw_body; }
    kaitai::kstream* _io__raw_body() const { return m__io__raw_body.get(); }
};

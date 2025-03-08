#pragma once

// This is a generated file! Please edit source .ksy file and use kaitai-struct-compiler to rebuild

#include "../kaitai/kaitaistruct.h"
#include <stdint.h>
#include <memory>

#if KAITAI_STRUCT_VERSION < 9000L
#error "Incompatible Kaitai Struct C++/STL API: version 0.9 or later is required"
#endif

class ms_slic_pubkey_t : public kaitai::kstruct {

public:

    ms_slic_pubkey_t(kaitai::kstream* p__io, kaitai::kstruct* p__parent = nullptr, ms_slic_pubkey_t* p__root = nullptr);

private:
    void _read();
    void _clean_up();

public:
    ~ms_slic_pubkey_t();

private:
    uint32_t m_type;
    uint32_t m_len_pubkey;
    uint8_t m_key_type;
    uint8_t m_version;
    uint16_t m_reserved;
    uint32_t m_algorithm;
    uint32_t m_magic;
    uint32_t m_bit_length;
    uint32_t m_exponent;
    std::string m_modulus;
    ms_slic_pubkey_t* m__root;
    kaitai::kstruct* m__parent;

public:
    uint32_t type() const { return m_type; }
    uint32_t len_pubkey() const { return m_len_pubkey; }
    uint8_t key_type() const { return m_key_type; }
    uint8_t version() const { return m_version; }
    uint16_t reserved() const { return m_reserved; }
    uint32_t algorithm() const { return m_algorithm; }
    uint32_t magic() const { return m_magic; }
    uint32_t bit_length() const { return m_bit_length; }
    uint32_t exponent() const { return m_exponent; }
    std::string modulus() const { return m_modulus; }
    ms_slic_pubkey_t* _root() const { return m__root; }
    kaitai::kstruct* _parent() const { return m__parent; }
};

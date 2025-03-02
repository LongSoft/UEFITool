#pragma once

// This is a generated file! Please edit source .ksy file and use kaitai-struct-compiler to rebuild

#include "../kaitai/kaitaistruct.h"
#include <stdint.h>
#include <memory>

#if KAITAI_STRUCT_VERSION < 9000L
#error "Incompatible Kaitai Struct C++/STL API: version 0.9 or later is required"
#endif

class edk2_ftw_t : public kaitai::kstruct {

public:

    edk2_ftw_t(kaitai::kstream* p__io, kaitai::kstruct* p__parent = nullptr, edk2_ftw_t* p__root = nullptr);

private:
    void _read();
    void _clean_up();

public:
    ~edk2_ftw_t();

private:
    bool f_len_ftw_store_header_32;
    int8_t m_len_ftw_store_header_32;

public:
    int8_t len_ftw_store_header_32();

private:
    bool f_len_ftw_store_header_64;
    int8_t m_len_ftw_store_header_64;

public:
    int8_t len_ftw_store_header_64();

private:
    std::string m_signature;
    uint32_t m_crc;
    uint8_t m_state;
    std::string m_reserved;
    uint32_t m_len_write_queue_32;
    uint32_t m_len_write_queue_64;
    bool n_len_write_queue_64;

public:
    bool _is_null_len_write_queue_64() { len_write_queue_64(); return n_len_write_queue_64; };

private:
    std::string m_write_queue_32;
    bool n_write_queue_32;

public:
    bool _is_null_write_queue_32() { write_queue_32(); return n_write_queue_32; };

private:
    std::string m_write_queue_64;
    bool n_write_queue_64;

public:
    bool _is_null_write_queue_64() { write_queue_64(); return n_write_queue_64; };

private:
    edk2_ftw_t* m__root;
    kaitai::kstruct* m__parent;

public:
    std::string signature() const { return m_signature; }
    uint32_t crc() const { return m_crc; }
    uint8_t state() const { return m_state; }
    std::string reserved() const { return m_reserved; }
    uint32_t len_write_queue_32() const { return m_len_write_queue_32; }
    uint32_t len_write_queue_64() const { return m_len_write_queue_64; }
    std::string write_queue_32() const { return m_write_queue_32; }
    std::string write_queue_64() const { return m_write_queue_64; }
    edk2_ftw_t* _root() const { return m__root; }
    kaitai::kstruct* _parent() const { return m__parent; }
};

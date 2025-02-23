#pragma once

// This is a generated file! Please edit source .ksy file and use kaitai-struct-compiler to rebuild

#include "../kaitai/kaitaistruct.h"
#include <stdint.h>
#include <memory>

#if KAITAI_STRUCT_VERSION < 9000L
#error "Incompatible Kaitai Struct C++/STL API: version 0.9 or later is required"
#endif

class insyde_fdc_t : public kaitai::kstruct {

public:

    insyde_fdc_t(kaitai::kstream* p__io, kaitai::kstruct* p__parent = nullptr, insyde_fdc_t* p__root = nullptr);

private:
    void _read();
    void _clean_up();

public:
    ~insyde_fdc_t();

private:
    bool f_len_fdc_store_header;
    int8_t m_len_fdc_store_header;

public:
    int8_t len_fdc_store_header();

private:
    uint32_t m_signature;
    uint32_t m_fdc_size;
    std::string m_body;
    insyde_fdc_t* m__root;
    kaitai::kstruct* m__parent;

public:
    uint32_t signature() const { return m_signature; }
    uint32_t fdc_size() const { return m_fdc_size; }
    std::string body() const { return m_body; }
    insyde_fdc_t* _root() const { return m__root; }
    kaitai::kstruct* _parent() const { return m__parent; }
};

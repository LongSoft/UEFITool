#pragma once

// This is a generated file! Please edit source .ksy file and use kaitai-struct-compiler to rebuild

#include "../kaitai/kaitaistruct.h"
#include <stdint.h>
#include <memory>

#if KAITAI_STRUCT_VERSION < 9000L
#error "Incompatible Kaitai Struct C++/STL API: version 0.9 or later is required"
#endif

class ms_slic_marker_t : public kaitai::kstruct {

public:

    ms_slic_marker_t(kaitai::kstream* p__io, kaitai::kstruct* p__parent = nullptr, ms_slic_marker_t* p__root = nullptr);

private:
    void _read();
    void _clean_up();

public:
    ~ms_slic_marker_t();

private:
    uint32_t m_type;
    uint32_t m_len_marker;
    uint32_t m_version;
    std::string m_oem_id;
    std::string m_oem_table_id;
    uint64_t m_windows_flag;
    uint32_t m_slic_version;
    std::string m_reserved;
    std::string m_signature;
    ms_slic_marker_t* m__root;
    kaitai::kstruct* m__parent;

public:
    uint32_t type() const { return m_type; }
    uint32_t len_marker() const { return m_len_marker; }
    uint32_t version() const { return m_version; }
    std::string oem_id() const { return m_oem_id; }
    std::string oem_table_id() const { return m_oem_table_id; }
    uint64_t windows_flag() const { return m_windows_flag; }
    uint32_t slic_version() const { return m_slic_version; }
    std::string reserved() const { return m_reserved; }
    std::string signature() const { return m_signature; }
    ms_slic_marker_t* _root() const { return m__root; }
    kaitai::kstruct* _parent() const { return m__parent; }
};

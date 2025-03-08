// This is a generated file! Please edit source .ksy file and use kaitai-struct-compiler to rebuild

#include "ms_slic_marker.h"

ms_slic_marker_t::ms_slic_marker_t(kaitai::kstream* p__io, kaitai::kstruct* p__parent, ms_slic_marker_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = this; (void)p__root;
    _read();
}

void ms_slic_marker_t::_read() {
    m_type = m__io->read_u4le();
    m_len_marker = m__io->read_u4le();
    m_version = m__io->read_u4le();
    m_oem_id = m__io->read_bytes(6);
    m_oem_table_id = m__io->read_bytes(8);
    m_windows_flag = m__io->read_u8le();
    m_slic_version = m__io->read_u4le();
    m_reserved = m__io->read_bytes(16);
    m_signature = m__io->read_bytes(128);
}

ms_slic_marker_t::~ms_slic_marker_t() {
    _clean_up();
}

void ms_slic_marker_t::_clean_up() {
}

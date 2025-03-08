// This is a generated file! Please edit source .ksy file and use kaitai-struct-compiler to rebuild

#include "ms_slic_pubkey.h"

ms_slic_pubkey_t::ms_slic_pubkey_t(kaitai::kstream* p__io, kaitai::kstruct* p__parent, ms_slic_pubkey_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = this; (void)p__root;
    _read();
}

void ms_slic_pubkey_t::_read() {
    m_type = m__io->read_u4le();
    m_len_pubkey = m__io->read_u4le();
    m_key_type = m__io->read_u1();
    m_version = m__io->read_u1();
    m_reserved = m__io->read_u2le();
    m_algorithm = m__io->read_u4le();
    m_magic = m__io->read_u4le();
    m_bit_length = m__io->read_u4le();
    m_exponent = m__io->read_u4le();
    m_modulus = m__io->read_bytes(128);
}

ms_slic_pubkey_t::~ms_slic_pubkey_t() {
    _clean_up();
}

void ms_slic_pubkey_t::_clean_up() {
}

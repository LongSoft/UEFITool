// This is a generated file! Please edit source .ksy file and use kaitai-struct-compiler to rebuild

#include "phoenix_flm.h"

phoenix_flm_t::phoenix_flm_t(kaitai::kstream* p__io, kaitai::kstruct* p__parent, phoenix_flm_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = this; (void)p__root;
    m_entries = nullptr;
    f_len_flm_store_header = false;
    f_len_flm_entry = false;
    _read();
}

void phoenix_flm_t::_read() {
    m_signature = m__io->read_bytes(10);
    m_num_entries = m__io->read_u2le();
    m_reserved = m__io->read_u4le();
    m_entries = std::unique_ptr<std::vector<std::unique_ptr<flm_entry_t>>>(new std::vector<std::unique_ptr<flm_entry_t>>());
    const int l_entries = num_entries();
    for (int i = 0; i < l_entries; i++) {
        m_entries->push_back(std::move(std::unique_ptr<flm_entry_t>(new flm_entry_t(m__io, this, m__root))));
    }
}

phoenix_flm_t::~phoenix_flm_t() {
    _clean_up();
}

void phoenix_flm_t::_clean_up() {
}

phoenix_flm_t::flm_entry_t::flm_entry_t(kaitai::kstream* p__io, phoenix_flm_t* p__parent, phoenix_flm_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;
    _read();
}

void phoenix_flm_t::flm_entry_t::_read() {
    m_guid = m__io->read_bytes(16);
    m_data_type = m__io->read_u2le();
    m_entry_type = m__io->read_u2le();
    m_physical_address = m__io->read_u8le();
    m_size = m__io->read_u4le();
    m_offset = m__io->read_u4le();
}

phoenix_flm_t::flm_entry_t::~flm_entry_t() {
    _clean_up();
}

void phoenix_flm_t::flm_entry_t::_clean_up() {
}

int8_t phoenix_flm_t::len_flm_store_header() {
    if (f_len_flm_store_header)
        return m_len_flm_store_header;
    m_len_flm_store_header = 16;
    f_len_flm_store_header = true;
    return m_len_flm_store_header;
}

int8_t phoenix_flm_t::len_flm_entry() {
    if (f_len_flm_entry)
        return m_len_flm_entry;
    m_len_flm_entry = 36;
    f_len_flm_entry = true;
    return m_len_flm_entry;
}

// This is a generated file! Please edit source .ksy file and use kaitai-struct-compiler to rebuild

#include "phoenix_flm.h"
#include "../kaitai/exceptions.h"

phoenix_flm_t::phoenix_flm_t(kaitai::kstream* p__io, kaitai::kstruct* p__parent, phoenix_flm_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = this; (void)p__root;
    m_entries = nullptr;
    m_free_space = nullptr;
    f_len_flm_store = false;
    f_len_flm_store_header = false;
    f_len_flm_entry = false;
    _read();
}

void phoenix_flm_t::_read() {
    m_signature = m__io->read_bytes(10);
    if (!(signature() == std::string("\x5F\x46\x4C\x41\x53\x48\x5F\x4D\x41\x50", 10))) {
        throw kaitai::validation_not_equal_error<std::string>(std::string("\x5F\x46\x4C\x41\x53\x48\x5F\x4D\x41\x50", 10), signature(), _io(), std::string("/seq/0"));
    }
    m_num_entries = m__io->read_u2le();
    {
        uint16_t _ = num_entries();
        if (!(_ <= 113)) {
            throw kaitai::validation_expr_error<uint16_t>(num_entries(), _io(), std::string("/seq/1"));
        }
    }
    m_reserved = m__io->read_u4le();
    m_entries = std::unique_ptr<std::vector<std::unique_ptr<flm_entry_t>>>(new std::vector<std::unique_ptr<flm_entry_t>>());
    const int l_entries = num_entries();
    for (int i = 0; i < l_entries; i++) {
        m_entries->push_back(std::move(std::unique_ptr<flm_entry_t>(new flm_entry_t(m__io, this, m__root))));
    }
    m_free_space = std::unique_ptr<std::vector<uint8_t>>(new std::vector<uint8_t>());
    const int l_free_space = ((len_flm_store() - len_flm_store_header()) - (len_flm_entry() * num_entries()));
    for (int i = 0; i < l_free_space; i++) {
        m_free_space->push_back(std::move(m__io->read_u1()));
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

int32_t phoenix_flm_t::len_flm_store() {
    if (f_len_flm_store)
        return m_len_flm_store;
    m_len_flm_store = 4096;
    f_len_flm_store = true;
    return m_len_flm_store;
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

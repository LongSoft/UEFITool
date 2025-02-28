// This is a generated file! Please edit source .ksy file and use kaitai-struct-compiler to rebuild

#include "phoenix_evsa.h"
#include "../kaitai/exceptions.h"

phoenix_evsa_t::phoenix_evsa_t(kaitai::kstream* p__io, kaitai::kstruct* p__parent, phoenix_evsa_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = this; (void)p__root;
    m_body = nullptr;
    m__io__raw_body = nullptr;
    _read();
}

void phoenix_evsa_t::_read() {
    m_type = m__io->read_u1();
    if (!(type() == 236)) {
        throw kaitai::validation_not_equal_error<uint8_t>(236, type(), _io(), std::string("/seq/0"));
    }
    m_checksum = m__io->read_u1();
    m_size = m__io->read_u2le();
    if (!(size() == 20)) {
        throw kaitai::validation_not_equal_error<uint16_t>(20, size(), _io(), std::string("/seq/2"));
    }
    m_signature = m__io->read_u4le();
    if (!(signature() == 1095980613)) {
        throw kaitai::validation_not_equal_error<uint32_t>(1095980613, signature(), _io(), std::string("/seq/3"));
    }
    m_attributes = m__io->read_u4le();
    m_store_size = m__io->read_u4le();
    m_reserved = m__io->read_u4le();
    m__raw_body = m__io->read_bytes((store_size() - 20));
    m__io__raw_body = std::unique_ptr<kaitai::kstream>(new kaitai::kstream(m__raw_body));
    m_body = std::unique_ptr<evsa_body_t>(new evsa_body_t(m__io__raw_body.get(), this, m__root));
}

phoenix_evsa_t::~phoenix_evsa_t() {
    _clean_up();
}

void phoenix_evsa_t::_clean_up() {
}

phoenix_evsa_t::evsa_entry_t::evsa_entry_t(kaitai::kstream* p__io, phoenix_evsa_t::evsa_body_t* p__parent, phoenix_evsa_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;
    _read();
}

void phoenix_evsa_t::evsa_entry_t::_read() {
    m_type = m__io->read_u1();
    m_checksum = m__io->read_u1();
    m_size = m__io->read_u2le();
    switch (type()) {
    case 239: {
        m_body = std::unique_ptr<evsa_data_t>(new evsa_data_t(m__io, this, m__root));
        break;
    }
    case 131: {
        m_body = std::unique_ptr<evsa_data_t>(new evsa_data_t(m__io, this, m__root));
        break;
    }
    case 227: {
        m_body = std::unique_ptr<evsa_data_t>(new evsa_data_t(m__io, this, m__root));
        break;
    }
    case 237: {
        m_body = std::unique_ptr<evsa_guid_t>(new evsa_guid_t(m__io, this, m__root));
        break;
    }
    case 226: {
        m_body = std::unique_ptr<evsa_name_t>(new evsa_name_t(m__io, this, m__root));
        break;
    }
    case 225: {
        m_body = std::unique_ptr<evsa_guid_t>(new evsa_guid_t(m__io, this, m__root));
        break;
    }
    case 238: {
        m_body = std::unique_ptr<evsa_name_t>(new evsa_name_t(m__io, this, m__root));
        break;
    }
    default: {
        m_body = std::unique_ptr<evsa_unknown_t>(new evsa_unknown_t(m__io, this, m__root));
        break;
    }
    }
}

phoenix_evsa_t::evsa_entry_t::~evsa_entry_t() {
    _clean_up();
}

void phoenix_evsa_t::evsa_entry_t::_clean_up() {
}

phoenix_evsa_t::evsa_unknown_t::evsa_unknown_t(kaitai::kstream* p__io, phoenix_evsa_t::evsa_entry_t* p__parent, phoenix_evsa_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;
    _read();
}

void phoenix_evsa_t::evsa_unknown_t::_read() {
    m_unknown = m__io->read_bytes(0);
}

phoenix_evsa_t::evsa_unknown_t::~evsa_unknown_t() {
    _clean_up();
}

void phoenix_evsa_t::evsa_unknown_t::_clean_up() {
}

phoenix_evsa_t::evsa_body_t::evsa_body_t(kaitai::kstream* p__io, phoenix_evsa_t* p__parent, phoenix_evsa_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;
    m_entries = nullptr;
    _read();
}

void phoenix_evsa_t::evsa_body_t::_read() {
    m_entries = std::unique_ptr<std::vector<std::unique_ptr<evsa_entry_t>>>(new std::vector<std::unique_ptr<evsa_entry_t>>());
    {
        int i = 0;
        while (!m__io->is_eof()) {
            m_entries->push_back(std::move(std::unique_ptr<evsa_entry_t>(new evsa_entry_t(m__io, this, m__root))));
            i++;
        }
    }
}

phoenix_evsa_t::evsa_body_t::~evsa_body_t() {
    _clean_up();
}

void phoenix_evsa_t::evsa_body_t::_clean_up() {
}

phoenix_evsa_t::evsa_name_t::evsa_name_t(kaitai::kstream* p__io, phoenix_evsa_t::evsa_entry_t* p__parent, phoenix_evsa_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;
    _read();
}

void phoenix_evsa_t::evsa_name_t::_read() {
    m_var_id = m__io->read_u2le();
    m_name = m__io->read_bytes((_parent()->size() - 6));
}

phoenix_evsa_t::evsa_name_t::~evsa_name_t() {
    _clean_up();
}

void phoenix_evsa_t::evsa_name_t::_clean_up() {
}

phoenix_evsa_t::evsa_guid_t::evsa_guid_t(kaitai::kstream* p__io, phoenix_evsa_t::evsa_entry_t* p__parent, phoenix_evsa_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;
    _read();
}

void phoenix_evsa_t::evsa_guid_t::_read() {
    m_guid_id = m__io->read_u2le();
    m_guid = m__io->read_bytes(16);
}

phoenix_evsa_t::evsa_guid_t::~evsa_guid_t() {
    _clean_up();
}

void phoenix_evsa_t::evsa_guid_t::_clean_up() {
}

phoenix_evsa_t::evsa_data_t::evsa_data_t(kaitai::kstream* p__io, phoenix_evsa_t::evsa_entry_t* p__parent, phoenix_evsa_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;
    _read();
}

void phoenix_evsa_t::evsa_data_t::_read() {
    m_guid_id = m__io->read_u2le();
    m_var_id = m__io->read_u2le();
    m_attributes = m__io->read_u4le();
    n_data_size = true;
    if ((attributes() & 268435456) != 268435456) {
        n_data_size = false;
        m_data_size = m__io->read_u4le();
    }
    n_data = true;
    if ((attributes() & 268435456) == 268435456) {
        n_data = false;
        m_data = m__io->read_bytes((_parent()->size() - 12));
    }
    n_data_ext = true;
    if ((attributes() & 268435456) != 268435456) {
        n_data_ext = false;
        m_data_ext = m__io->read_bytes(data_size());
    }
}

phoenix_evsa_t::evsa_data_t::~evsa_data_t() {
    _clean_up();
}

void phoenix_evsa_t::evsa_data_t::_clean_up() {
    if (!n_data_size) {
    }
    if (!n_data) {
    }
    if (!n_data_ext) {
    }
}

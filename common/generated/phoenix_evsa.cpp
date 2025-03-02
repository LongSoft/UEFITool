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
    m_len_evsa_store_header = m__io->read_u2le();
    if (!(len_evsa_store_header() == 20)) {
        throw kaitai::validation_not_equal_error<uint16_t>(20, len_evsa_store_header(), _io(), std::string("/seq/2"));
    }
    m_signature = m__io->read_u4le();
    if (!(signature() == 1095980613)) {
        throw kaitai::validation_not_equal_error<uint32_t>(1095980613, signature(), _io(), std::string("/seq/3"));
    }
    m_attributes = m__io->read_u4le();
    m_len_evsa_store = m__io->read_u4le();
    m_reserved = m__io->read_u4le();
    m__raw_body = m__io->read_bytes((len_evsa_store() - len_evsa_store_header()));
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
    m_entry_type = m__io->read_u1();
    n_checksum = true;
    if ( ((entry_type() == 225) || (entry_type() == 226) || (entry_type() == 227) || (entry_type() == 237) || (entry_type() == 238) || (entry_type() == 239) || (entry_type() == 131)) ) {
        n_checksum = false;
        m_checksum = m__io->read_u1();
    }
    n_len_evsa_entry = true;
    if ( ((entry_type() == 225) || (entry_type() == 226) || (entry_type() == 227) || (entry_type() == 237) || (entry_type() == 238) || (entry_type() == 239) || (entry_type() == 131)) ) {
        n_len_evsa_entry = false;
        m_len_evsa_entry = m__io->read_u2le();
    }
    switch (entry_type()) {
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
    if (!n_checksum) {
    }
    if (!n_len_evsa_entry) {
    }
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
    m_free_space = nullptr;
    _read();
}

void phoenix_evsa_t::evsa_body_t::_read() {
    m_entries = std::unique_ptr<std::vector<std::unique_ptr<evsa_entry_t>>>(new std::vector<std::unique_ptr<evsa_entry_t>>());
    {
        int i = 0;
        evsa_entry_t* _;
        do {
            _ = new evsa_entry_t(m__io, this, m__root);
            m_entries->push_back(std::move(std::unique_ptr<evsa_entry_t>(_)));
            i++;
        } while (!( (( ((_->entry_type() != 237) && (_->entry_type() != 238) && (_->entry_type() != 239) && (_->entry_type() != 225) && (_->entry_type() != 226) && (_->entry_type() != 227) && (_->entry_type() != 131)) ) || (_io()->is_eof())) ));
    }
    m_free_space = std::unique_ptr<std::vector<uint8_t>>(new std::vector<uint8_t>());
    {
        int i = 0;
        while (!m__io->is_eof()) {
            m_free_space->push_back(std::move(m__io->read_u1()));
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
    m_name = m__io->read_bytes((_parent()->len_evsa_entry() - 6));
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
    {
        std::string _ = guid();
        if (!(_parent()->len_evsa_entry() == 22)) {
            throw kaitai::validation_expr_error<std::string>(guid(), _io(), std::string("/types/evsa_guid/seq/1"));
        }
    }
}

phoenix_evsa_t::evsa_guid_t::~evsa_guid_t() {
    _clean_up();
}

void phoenix_evsa_t::evsa_guid_t::_clean_up() {
}

phoenix_evsa_t::evsa_variable_attributes_t::evsa_variable_attributes_t(kaitai::kstream* p__io, phoenix_evsa_t::evsa_data_t* p__parent, phoenix_evsa_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;
    _read();
}

void phoenix_evsa_t::evsa_variable_attributes_t::_read() {
    m_non_volatile = m__io->read_bits_int_le(1);
    m_boot_service = m__io->read_bits_int_le(1);
    m_runtime = m__io->read_bits_int_le(1);
    m_hw_error_record = m__io->read_bits_int_le(1);
    m_auth_write = m__io->read_bits_int_le(1);
    m_time_based_auth = m__io->read_bits_int_le(1);
    m_append_write = m__io->read_bits_int_le(1);
    m_reserved = m__io->read_bits_int_le(21);
    m_extended_header = m__io->read_bits_int_le(1);
    m_reserved1 = m__io->read_bits_int_le(3);
}

phoenix_evsa_t::evsa_variable_attributes_t::~evsa_variable_attributes_t() {
    _clean_up();
}

void phoenix_evsa_t::evsa_variable_attributes_t::_clean_up() {
}

phoenix_evsa_t::evsa_data_t::evsa_data_t(kaitai::kstream* p__io, phoenix_evsa_t::evsa_entry_t* p__parent, phoenix_evsa_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;
    m_attributes = nullptr;
    _read();
}

void phoenix_evsa_t::evsa_data_t::_read() {
    m_guid_id = m__io->read_u2le();
    m_var_id = m__io->read_u2le();
    m_attributes = std::unique_ptr<evsa_variable_attributes_t>(new evsa_variable_attributes_t(m__io, this, m__root));
    n_len_data_ext = true;
    if (attributes()->extended_header()) {
        n_len_data_ext = false;
        m_len_data_ext = m__io->read_u4le();
    }
    n_data = true;
    if (!(attributes()->extended_header())) {
        n_data = false;
        m_data = m__io->read_bytes((_parent()->len_evsa_entry() - 12));
    }
    n_data_ext = true;
    if (attributes()->extended_header()) {
        n_data_ext = false;
        m_data_ext = m__io->read_bytes(len_data_ext());
    }
}

phoenix_evsa_t::evsa_data_t::~evsa_data_t() {
    _clean_up();
}

void phoenix_evsa_t::evsa_data_t::_clean_up() {
    if (!n_len_data_ext) {
    }
    if (!n_data) {
    }
    if (!n_data_ext) {
    }
}

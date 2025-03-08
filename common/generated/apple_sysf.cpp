// This is a generated file! Please edit source .ksy file and use kaitai-struct-compiler to rebuild

#include "apple_sysf.h"

apple_sysf_t::apple_sysf_t(kaitai::kstream* p__io, kaitai::kstruct* p__parent, apple_sysf_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = this; (void)p__root;
    m_body = nullptr;
    m__io__raw_body = nullptr;
    f_len_sysf_store_header = false;
    _read();
}

void apple_sysf_t::_read() {
    m_signature = m__io->read_u4le();
    m_unknown = m__io->read_u1();
    m_unknown1 = m__io->read_u4le();
    m_sysf_size = m__io->read_u2le();
    m__raw_body = m__io->read_bytes(((sysf_size() - len_sysf_store_header()) - 4));
    m__io__raw_body = std::unique_ptr<kaitai::kstream>(new kaitai::kstream(m__raw_body));
    m_body = std::unique_ptr<sysf_store_body_t>(new sysf_store_body_t(m__io__raw_body.get(), this, m__root));
    m_crc = m__io->read_u4le();
}

apple_sysf_t::~apple_sysf_t() {
    _clean_up();
}

void apple_sysf_t::_clean_up() {
}

apple_sysf_t::sysf_store_body_t::sysf_store_body_t(kaitai::kstream* p__io, apple_sysf_t* p__parent, apple_sysf_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;
    m_variables = nullptr;
    m_zeroes = nullptr;
    _read();
}

void apple_sysf_t::sysf_store_body_t::_read() {
    m_variables = std::unique_ptr<std::vector<std::unique_ptr<sysf_variable_t>>>(new std::vector<std::unique_ptr<sysf_variable_t>>());
    {
        int i = 0;
        sysf_variable_t* _;
        do {
            _ = new sysf_variable_t(m__io, this, m__root);
            m_variables->push_back(std::move(std::unique_ptr<sysf_variable_t>(_)));
            i++;
        } while (!( (( ((_->len_name() == 3) && (_->name() == (std::string("EOF")))) ) || (_io()->is_eof())) ));
    }
    m_zeroes = std::unique_ptr<std::vector<uint8_t>>(new std::vector<uint8_t>());
    {
        int i = 0;
        while (!m__io->is_eof()) {
            m_zeroes->push_back(std::move(m__io->read_u1()));
            i++;
        }
    }
}

apple_sysf_t::sysf_store_body_t::~sysf_store_body_t() {
    _clean_up();
}

void apple_sysf_t::sysf_store_body_t::_clean_up() {
}

apple_sysf_t::sysf_variable_t::sysf_variable_t(kaitai::kstream* p__io, apple_sysf_t::sysf_store_body_t* p__parent, apple_sysf_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;
    _read();
}

void apple_sysf_t::sysf_variable_t::_read() {
    m_len_name = m__io->read_bits_int_le(7);
    m_invalid_flag = m__io->read_bits_int_le(1);
    m__io->align_to_byte();
    m_name = kaitai::kstream::bytes_to_str(kaitai::kstream::bytes_terminate(m__io->read_bytes(len_name()), 0, false), std::string("ascii"));
    n_len_data = true;
    if (name() != std::string("EOF")) {
        n_len_data = false;
        m_len_data = m__io->read_u2le();
    }
    n_data = true;
    if (name() != std::string("EOF")) {
        n_data = false;
        m_data = m__io->read_bytes(len_data());
    }
}

apple_sysf_t::sysf_variable_t::~sysf_variable_t() {
    _clean_up();
}

void apple_sysf_t::sysf_variable_t::_clean_up() {
    if (!n_len_data) {
    }
    if (!n_data) {
    }
}

int8_t apple_sysf_t::len_sysf_store_header() {
    if (f_len_sysf_store_header)
        return m_len_sysf_store_header;
    m_len_sysf_store_header = 11;
    f_len_sysf_store_header = true;
    return m_len_sysf_store_header;
}

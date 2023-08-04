// This is a generated file! Please edit source .ksy file and use kaitai-struct-compiler to rebuild

#include "ami_nvar.h"
#include "../kaitai/exceptions.h"

ami_nvar_t::ami_nvar_t(kaitai::kstream* p__io, kaitai::kstruct* p__parent, ami_nvar_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = this;
    m_entries = nullptr;
    _read();
}

void ami_nvar_t::_read() {
    m_entries = std::unique_ptr<std::vector<std::unique_ptr<nvar_entry_t>>>(new std::vector<std::unique_ptr<nvar_entry_t>>());
    {
        int i = 0;
        nvar_entry_t* _;
        do {
            _ = new nvar_entry_t(m__io, this, m__root);
            m_entries->push_back(std::move(std::unique_ptr<nvar_entry_t>(_)));
            i++;
        } while (!( ((_->signature_first() != 78) || (_io()->is_eof())) ));
    }
}

ami_nvar_t::~ami_nvar_t() {
    _clean_up();
}

void ami_nvar_t::_clean_up() {
}

ami_nvar_t::nvar_attributes_t::nvar_attributes_t(kaitai::kstream* p__io, ami_nvar_t::nvar_entry_t* p__parent, ami_nvar_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;
    _read();
}

void ami_nvar_t::nvar_attributes_t::_read() {
    m_valid = m__io->read_bits_int_be(1);
    m_auth_write = m__io->read_bits_int_be(1);
    m_hw_error_record = m__io->read_bits_int_be(1);
    m_extended_header = m__io->read_bits_int_be(1);
    m_data_only = m__io->read_bits_int_be(1);
    m_local_guid = m__io->read_bits_int_be(1);
    m_ascii_name = m__io->read_bits_int_be(1);
    m_runtime = m__io->read_bits_int_be(1);
}

ami_nvar_t::nvar_attributes_t::~nvar_attributes_t() {
    _clean_up();
}

void ami_nvar_t::nvar_attributes_t::_clean_up() {
}

ami_nvar_t::ucs2_string_t::ucs2_string_t(kaitai::kstream* p__io, ami_nvar_t::nvar_entry_body_t* p__parent, ami_nvar_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;
    m_ucs2_chars = nullptr;
    _read();
}

void ami_nvar_t::ucs2_string_t::_read() {
    m_ucs2_chars = std::unique_ptr<std::vector<uint16_t>>(new std::vector<uint16_t>());
    {
        int i = 0;
        uint16_t _;
        do {
            _ = m__io->read_u2le();
            m_ucs2_chars->push_back(_);
            i++;
        } while (!(_ == 0));
    }
}

ami_nvar_t::ucs2_string_t::~ucs2_string_t() {
    _clean_up();
}

void ami_nvar_t::ucs2_string_t::_clean_up() {
}

ami_nvar_t::nvar_extended_attributes_t::nvar_extended_attributes_t(kaitai::kstream* p__io, ami_nvar_t::nvar_entry_body_t* p__parent, ami_nvar_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;
    _read();
}

void ami_nvar_t::nvar_extended_attributes_t::_read() {
    m_reserved_high = m__io->read_bits_int_be(2);
    m_time_based_auth = m__io->read_bits_int_be(1);
    m_auth_write = m__io->read_bits_int_be(1);
    m_reserved_low = m__io->read_bits_int_be(3);
    m_checksum = m__io->read_bits_int_be(1);
}

ami_nvar_t::nvar_extended_attributes_t::~nvar_extended_attributes_t() {
    _clean_up();
}

void ami_nvar_t::nvar_extended_attributes_t::_clean_up() {
}

ami_nvar_t::nvar_entry_t::nvar_entry_t(kaitai::kstream* p__io, ami_nvar_t* p__parent, ami_nvar_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;
    m_attributes = nullptr;
    m_body = nullptr;
    m__io__raw_body = nullptr;
    f_offset = false;
    f_end_offset = false;
    _read();
}

void ami_nvar_t::nvar_entry_t::_read() {
    n_invoke_offset = true;
    if (offset() >= 0) {
        n_invoke_offset = false;
        m_invoke_offset = m__io->read_bytes(0);
    }
    m_signature_first = m__io->read_u1();
    n_signature_rest = true;
    if (signature_first() == 78) {
        n_signature_rest = false;
        m_signature_rest = m__io->read_bytes(3);
        if (!(signature_rest() == std::string("\x56\x41\x52", 3))) {
            throw kaitai::validation_not_equal_error<std::string>(std::string("\x56\x41\x52", 3), signature_rest(), _io(), std::string("/types/nvar_entry/seq/2"));
        }
    }
    n_size = true;
    if (signature_first() == 78) {
        n_size = false;
        m_size = m__io->read_u2le();
        {
            uint16_t _ = size();
            if (!(_ > ((4 + 2) + 4))) {
                throw kaitai::validation_expr_error<uint16_t>(size(), _io(), std::string("/types/nvar_entry/seq/3"));
            }
        }
    }
    n_next = true;
    if (signature_first() == 78) {
        n_next = false;
        m_next = m__io->read_bits_int_le(24);
    }
    m__io->align_to_byte();
    n_attributes = true;
    if (signature_first() == 78) {
        n_attributes = false;
        m_attributes = std::unique_ptr<nvar_attributes_t>(new nvar_attributes_t(m__io, this, m__root));
    }
    n_body = true;
    if (signature_first() == 78) {
        n_body = false;
        m__raw_body = m__io->read_bytes((size() - ((4 + 2) + 4)));
        m__io__raw_body = std::unique_ptr<kaitai::kstream>(new kaitai::kstream(m__raw_body));
        m_body = std::unique_ptr<nvar_entry_body_t>(new nvar_entry_body_t(m__io__raw_body.get(), this, m__root));
    }
    n_invoke_end_offset = true;
    if ( ((signature_first() == 78) && (end_offset() >= 0)) ) {
        n_invoke_end_offset = false;
        m_invoke_end_offset = m__io->read_bytes(0);
    }
}

ami_nvar_t::nvar_entry_t::~nvar_entry_t() {
    _clean_up();
}

void ami_nvar_t::nvar_entry_t::_clean_up() {
    if (!n_invoke_offset) {
    }
    if (!n_signature_rest) {
    }
    if (!n_size) {
    }
    if (!n_next) {
    }
    if (!n_attributes) {
    }
    if (!n_body) {
    }
    if (!n_invoke_end_offset) {
    }
}

int32_t ami_nvar_t::nvar_entry_t::offset() {
    if (f_offset)
        return m_offset;
    m_offset = _io()->pos();
    f_offset = true;
    return m_offset;
}

int32_t ami_nvar_t::nvar_entry_t::end_offset() {
    if (f_end_offset)
        return m_end_offset;
    m_end_offset = _io()->pos();
    f_end_offset = true;
    return m_end_offset;
}

ami_nvar_t::nvar_entry_body_t::nvar_entry_body_t(kaitai::kstream* p__io, ami_nvar_t::nvar_entry_t* p__parent, ami_nvar_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;
    m_ucs2_name = nullptr;
    m_extended_header_attributes = nullptr;
    f_extended_header_attributes = false;
    f_data_start_offset = false;
    f_extended_header_size_field = false;
    f_extended_header_timestamp = false;
    f_data_size = false;
    f_extended_header_checksum = false;
    f_data_end_offset = false;
    f_extended_header_size = false;
    f_extended_header_hash = false;
    _read();
}

void ami_nvar_t::nvar_entry_body_t::_read() {
    n_guid_index = true;
    if ( ((!(_parent()->attributes()->local_guid())) && (!(_parent()->attributes()->data_only())) && (_parent()->attributes()->valid())) ) {
        n_guid_index = false;
        m_guid_index = m__io->read_u1();
    }
    n_guid = true;
    if ( ((_parent()->attributes()->local_guid()) && (!(_parent()->attributes()->data_only())) && (_parent()->attributes()->valid())) ) {
        n_guid = false;
        m_guid = m__io->read_bytes(16);
    }
    n_ascii_name = true;
    if ( ((_parent()->attributes()->ascii_name()) && (!(_parent()->attributes()->data_only())) && (_parent()->attributes()->valid())) ) {
        n_ascii_name = false;
        m_ascii_name = kaitai::kstream::bytes_to_str(m__io->read_bytes_term(0, false, true, true), std::string("ASCII"));
    }
    n_ucs2_name = true;
    if ( ((!(_parent()->attributes()->ascii_name())) && (!(_parent()->attributes()->data_only())) && (_parent()->attributes()->valid())) ) {
        n_ucs2_name = false;
        m_ucs2_name = std::unique_ptr<ucs2_string_t>(new ucs2_string_t(m__io, this, m__root));
    }
    n_invoke_data_start = true;
    if (data_start_offset() >= 0) {
        n_invoke_data_start = false;
        m_invoke_data_start = m__io->read_bytes(0);
    }
    m_data = m__io->read_bytes_full();
}

ami_nvar_t::nvar_entry_body_t::~nvar_entry_body_t() {
    _clean_up();
}

void ami_nvar_t::nvar_entry_body_t::_clean_up() {
    if (!n_guid_index) {
    }
    if (!n_guid) {
    }
    if (!n_ascii_name) {
    }
    if (!n_ucs2_name) {
    }
    if (!n_invoke_data_start) {
    }
    if (f_extended_header_attributes && !n_extended_header_attributes) {
    }
    if (f_extended_header_size_field && !n_extended_header_size_field) {
    }
    if (f_extended_header_timestamp && !n_extended_header_timestamp) {
    }
    if (f_extended_header_checksum && !n_extended_header_checksum) {
    }
    if (f_extended_header_hash && !n_extended_header_hash) {
    }
}

ami_nvar_t::nvar_extended_attributes_t* ami_nvar_t::nvar_entry_body_t::extended_header_attributes() {
    if (f_extended_header_attributes)
        return m_extended_header_attributes.get();
    n_extended_header_attributes = true;
    if ( ((_parent()->attributes()->valid()) && (_parent()->attributes()->extended_header()) && (extended_header_size() >= (1 + 2))) ) {
        n_extended_header_attributes = false;
        std::streampos _pos = m__io->pos();
        m__io->seek((_io()->pos() - extended_header_size()));
        m_extended_header_attributes = std::unique_ptr<nvar_extended_attributes_t>(new nvar_extended_attributes_t(m__io, this, m__root));
        m__io->seek(_pos);
        f_extended_header_attributes = true;
    }
    return m_extended_header_attributes.get();
}

int32_t ami_nvar_t::nvar_entry_body_t::data_start_offset() {
    if (f_data_start_offset)
        return m_data_start_offset;
    m_data_start_offset = _io()->pos();
    f_data_start_offset = true;
    return m_data_start_offset;
}

uint16_t ami_nvar_t::nvar_entry_body_t::extended_header_size_field() {
    if (f_extended_header_size_field)
        return m_extended_header_size_field;
    n_extended_header_size_field = true;
    if ( ((_parent()->attributes()->valid()) && (_parent()->attributes()->extended_header()) && (_parent()->size() > (((4 + 2) + 4) + 2))) ) {
        n_extended_header_size_field = false;
        std::streampos _pos = m__io->pos();
        m__io->seek((_io()->pos() - 2));
        m_extended_header_size_field = m__io->read_u2le();
        m__io->seek(_pos);
        f_extended_header_size_field = true;
    }
    return m_extended_header_size_field;
}

uint64_t ami_nvar_t::nvar_entry_body_t::extended_header_timestamp() {
    if (f_extended_header_timestamp)
        return m_extended_header_timestamp;
    n_extended_header_timestamp = true;
    if ( ((_parent()->attributes()->valid()) && (_parent()->attributes()->extended_header()) && (extended_header_size() >= ((1 + 8) + 2)) && (extended_header_attributes()->time_based_auth())) ) {
        n_extended_header_timestamp = false;
        std::streampos _pos = m__io->pos();
        m__io->seek(((_io()->pos() - extended_header_size()) + 1));
        m_extended_header_timestamp = m__io->read_u8le();
        m__io->seek(_pos);
        f_extended_header_timestamp = true;
    }
    return m_extended_header_timestamp;
}

int32_t ami_nvar_t::nvar_entry_body_t::data_size() {
    if (f_data_size)
        return m_data_size;
    m_data_size = ((data_end_offset() - data_start_offset()) - extended_header_size());
    f_data_size = true;
    return m_data_size;
}

uint8_t ami_nvar_t::nvar_entry_body_t::extended_header_checksum() {
    if (f_extended_header_checksum)
        return m_extended_header_checksum;
    n_extended_header_checksum = true;
    if ( ((_parent()->attributes()->valid()) && (_parent()->attributes()->extended_header()) && (extended_header_size() >= ((1 + 1) + 2)) && (extended_header_attributes()->checksum())) ) {
        n_extended_header_checksum = false;
        std::streampos _pos = m__io->pos();
        m__io->seek(((_io()->pos() - 2) - 1));
        m_extended_header_checksum = m__io->read_u1();
        m__io->seek(_pos);
        f_extended_header_checksum = true;
    }
    return m_extended_header_checksum;
}

int32_t ami_nvar_t::nvar_entry_body_t::data_end_offset() {
    if (f_data_end_offset)
        return m_data_end_offset;
    m_data_end_offset = _io()->pos();
    f_data_end_offset = true;
    return m_data_end_offset;
}

uint16_t ami_nvar_t::nvar_entry_body_t::extended_header_size() {
    if (f_extended_header_size)
        return m_extended_header_size;
    m_extended_header_size = (( ((_parent()->attributes()->extended_header()) && (_parent()->attributes()->valid()) && (_parent()->size() > (((4 + 2) + 4) + 2))) ) ? (((extended_header_size_field() >= (1 + 2)) ? (extended_header_size_field()) : (0))) : (0));
    f_extended_header_size = true;
    return m_extended_header_size;
}

std::string ami_nvar_t::nvar_entry_body_t::extended_header_hash() {
    if (f_extended_header_hash)
        return m_extended_header_hash;
    n_extended_header_hash = true;
    if ( ((_parent()->attributes()->valid()) && (_parent()->attributes()->extended_header()) && (extended_header_size() >= (((1 + 8) + 32) + 2)) && (extended_header_attributes()->time_based_auth()) && (!(_parent()->attributes()->data_only()))) ) {
        n_extended_header_hash = false;
        std::streampos _pos = m__io->pos();
        m__io->seek((((_io()->pos() - extended_header_size()) + 1) + 8));
        m_extended_header_hash = m__io->read_bytes(32);
        m__io->seek(_pos);
        f_extended_header_hash = true;
    }
    return m_extended_header_hash;
}

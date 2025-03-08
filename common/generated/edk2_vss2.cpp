// This is a generated file! Please edit source .ksy file and use kaitai-struct-compiler to rebuild

#include "edk2_vss2.h"
#include "../kaitai/exceptions.h"

edk2_vss2_t::edk2_vss2_t(kaitai::kstream* p__io, kaitai::kstruct* p__parent, edk2_vss2_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = this; (void)p__root;
    m_body = nullptr;
    m__io__raw_body = nullptr;
    f_len_vss2_store_header = false;
    _read();
}

void edk2_vss2_t::_read() {
    m_signature = m__io->read_bytes(16);
    m_vss2_size = m__io->read_u4le();
    {
        uint32_t _ = vss2_size();
        if (!( ((_ > static_cast<uint32_t>(len_vss2_store_header())) && (_ < 4294967295UL)) )) {
            throw kaitai::validation_expr_error<uint32_t>(vss2_size(), _io(), std::string("/seq/1"));
        }
    }
    m_format = m__io->read_u1();
    m_state = m__io->read_u1();
    m_reserved = m__io->read_u2le();
    m_reserved1 = m__io->read_u4le();
    m__raw_body = m__io->read_bytes((vss2_size() - len_vss2_store_header()));
    m__io__raw_body = std::unique_ptr<kaitai::kstream>(new kaitai::kstream(m__raw_body));
    m_body = std::unique_ptr<vss2_store_body_t>(new vss2_store_body_t(m__io__raw_body.get(), this, m__root));
}

edk2_vss2_t::~edk2_vss2_t() {
    _clean_up();
}

void edk2_vss2_t::_clean_up() {
}

edk2_vss2_t::vss2_store_body_t::vss2_store_body_t(kaitai::kstream* p__io, edk2_vss2_t* p__parent, edk2_vss2_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;
    m_variables = nullptr;
    _read();
}

void edk2_vss2_t::vss2_store_body_t::_read() {
    m_variables = std::unique_ptr<std::vector<std::unique_ptr<vss2_variable_t>>>(new std::vector<std::unique_ptr<vss2_variable_t>>());
    {
        int i = 0;
        vss2_variable_t* _;
        do {
            _ = new vss2_variable_t(m__io, this, m__root);
            m_variables->push_back(std::move(std::unique_ptr<vss2_variable_t>(_)));
            i++;
        } while (!( ((_->signature_first() != 170) || (_io()->is_eof())) ));
    }
}

edk2_vss2_t::vss2_store_body_t::~vss2_store_body_t() {
    _clean_up();
}

void edk2_vss2_t::vss2_store_body_t::_clean_up() {
}

edk2_vss2_t::vss2_variable_attributes_t::vss2_variable_attributes_t(kaitai::kstream* p__io, edk2_vss2_t::vss2_variable_t* p__parent, edk2_vss2_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;
    _read();
}

void edk2_vss2_t::vss2_variable_attributes_t::_read() {
    m_non_volatile = m__io->read_bits_int_le(1);
    m_boot_service = m__io->read_bits_int_le(1);
    m_runtime = m__io->read_bits_int_le(1);
    m_hw_error_record = m__io->read_bits_int_le(1);
    m_auth_write = m__io->read_bits_int_le(1);
    m_time_based_auth = m__io->read_bits_int_le(1);
    m_append_write = m__io->read_bits_int_le(1);
    m_reserved = m__io->read_bits_int_le(25);
}

edk2_vss2_t::vss2_variable_attributes_t::~vss2_variable_attributes_t() {
    _clean_up();
}

void edk2_vss2_t::vss2_variable_attributes_t::_clean_up() {
}

edk2_vss2_t::vss2_variable_t::vss2_variable_t(kaitai::kstream* p__io, edk2_vss2_t::vss2_store_body_t* p__parent, edk2_vss2_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;
    m_attributes = nullptr;
    f_is_auth = false;
    f_len_standard_header = false;
    f_end_offset_auth = false;
    f_len_alignment_padding = false;
    f_len_auth_header = false;
    f_end_offset = false;
    f_len_alignment_padding_auth = false;
    f_is_valid = false;
    f_offset = false;
    _read();
}

void edk2_vss2_t::vss2_variable_t::_read() {
    n_invoke_offset = true;
    if (offset() >= 0) {
        n_invoke_offset = false;
        m_invoke_offset = m__io->read_bytes(0);
    }
    m_signature_first = m__io->read_u1();
    n_signature_last = true;
    if (signature_first() == 170) {
        n_signature_last = false;
        m_signature_last = m__io->read_u1();
        {
            uint8_t _ = signature_last();
            if (!(_ == 85)) {
                throw kaitai::validation_expr_error<uint8_t>(signature_last(), _io(), std::string("/types/vss2_variable/seq/2"));
            }
        }
    }
    n_state = true;
    if (signature_first() == 170) {
        n_state = false;
        m_state = m__io->read_u1();
    }
    n_reserved = true;
    if (signature_first() == 170) {
        n_reserved = false;
        m_reserved = m__io->read_u1();
    }
    n_attributes = true;
    if (signature_first() == 170) {
        n_attributes = false;
        m_attributes = std::unique_ptr<vss2_variable_attributes_t>(new vss2_variable_attributes_t(m__io, this, m__root));
    }
    n_len_name = true;
    if (signature_first() == 170) {
        n_len_name = false;
        m_len_name = m__io->read_u4le();
    }
    n_len_data = true;
    if (signature_first() == 170) {
        n_len_data = false;
        m_len_data = m__io->read_u4le();
    }
    n_timestamp = true;
    if ( ((signature_first() == 170) && (is_auth())) ) {
        n_timestamp = false;
        m_timestamp = m__io->read_bytes(16);
    }
    n_pubkey_index = true;
    if ( ((signature_first() == 170) && (is_auth())) ) {
        n_pubkey_index = false;
        m_pubkey_index = m__io->read_u4le();
    }
    n_len_name_auth = true;
    if ( ((signature_first() == 170) && (is_auth())) ) {
        n_len_name_auth = false;
        m_len_name_auth = m__io->read_u4le();
    }
    n_len_data_auth = true;
    if ( ((signature_first() == 170) && (is_auth())) ) {
        n_len_data_auth = false;
        m_len_data_auth = m__io->read_u4le();
    }
    n_vendor_guid = true;
    if (signature_first() == 170) {
        n_vendor_guid = false;
        m_vendor_guid = m__io->read_bytes(16);
    }
    n_name_auth = true;
    if ( ((signature_first() == 170) && (is_auth())) ) {
        n_name_auth = false;
        m_name_auth = m__io->read_bytes(len_name_auth());
    }
    n_data_auth = true;
    if ( ((signature_first() == 170) && (is_auth())) ) {
        n_data_auth = false;
        m_data_auth = m__io->read_bytes(len_data_auth());
    }
    n_invoke_end_offset_auth = true;
    if ( ((signature_first() == 170) && (is_auth()) && (end_offset_auth() >= 0)) ) {
        n_invoke_end_offset_auth = false;
        m_invoke_end_offset_auth = m__io->read_bytes(0);
    }
    n_alignment_padding_auth = true;
    if ( ((signature_first() == 170) && (is_auth())) ) {
        n_alignment_padding_auth = false;
        m_alignment_padding_auth = m__io->read_bytes(len_alignment_padding_auth());
    }
    n_name = true;
    if ( ((signature_first() == 170) && (!(is_auth()))) ) {
        n_name = false;
        m_name = m__io->read_bytes(len_name());
    }
    n_data = true;
    if ( ((signature_first() == 170) && (!(is_auth()))) ) {
        n_data = false;
        m_data = m__io->read_bytes(len_data());
    }
    n_invoke_end_offset = true;
    if ( ((signature_first() == 170) && (!(is_auth())) && (end_offset() >= 0)) ) {
        n_invoke_end_offset = false;
        m_invoke_end_offset = m__io->read_bytes(0);
    }
    n_alignment_padding = true;
    if ( ((signature_first() == 170) && (!(is_auth()))) ) {
        n_alignment_padding = false;
        m_alignment_padding = m__io->read_bytes(len_alignment_padding());
    }
}

edk2_vss2_t::vss2_variable_t::~vss2_variable_t() {
    _clean_up();
}

void edk2_vss2_t::vss2_variable_t::_clean_up() {
    if (!n_invoke_offset) {
    }
    if (!n_signature_last) {
    }
    if (!n_state) {
    }
    if (!n_reserved) {
    }
    if (!n_attributes) {
    }
    if (!n_len_name) {
    }
    if (!n_len_data) {
    }
    if (!n_timestamp) {
    }
    if (!n_pubkey_index) {
    }
    if (!n_len_name_auth) {
    }
    if (!n_len_data_auth) {
    }
    if (!n_vendor_guid) {
    }
    if (!n_name_auth) {
    }
    if (!n_data_auth) {
    }
    if (!n_invoke_end_offset_auth) {
    }
    if (!n_alignment_padding_auth) {
    }
    if (!n_name) {
    }
    if (!n_data) {
    }
    if (!n_invoke_end_offset) {
    }
    if (!n_alignment_padding) {
    }
}

bool edk2_vss2_t::vss2_variable_t::is_auth() {
    if (f_is_auth)
        return m_is_auth;
    m_is_auth =  (( ((attributes()->auth_write()) || (attributes()->time_based_auth()) || (attributes()->append_write())) ) || ( ((len_name() == 0) || (len_data() == 0)) )) ;
    f_is_auth = true;
    return m_is_auth;
}

int8_t edk2_vss2_t::vss2_variable_t::len_standard_header() {
    if (f_len_standard_header)
        return m_len_standard_header;
    m_len_standard_header = 32;
    f_len_standard_header = true;
    return m_len_standard_header;
}

int32_t edk2_vss2_t::vss2_variable_t::end_offset_auth() {
    if (f_end_offset_auth)
        return m_end_offset_auth;
    m_end_offset_auth = (int32_t)_io()->pos();
    f_end_offset_auth = true;
    return m_end_offset_auth;
}

int32_t edk2_vss2_t::vss2_variable_t::len_alignment_padding() {
    if (f_len_alignment_padding)
        return m_len_alignment_padding;
    m_len_alignment_padding = ((((end_offset() - offset()) + 3) & ~3) - (end_offset() - offset()));
    f_len_alignment_padding = true;
    return m_len_alignment_padding;
}

int8_t edk2_vss2_t::vss2_variable_t::len_auth_header() {
    if (f_len_auth_header)
        return m_len_auth_header;
    m_len_auth_header = 60;
    f_len_auth_header = true;
    return m_len_auth_header;
}

int32_t edk2_vss2_t::vss2_variable_t::end_offset() {
    if (f_end_offset)
        return m_end_offset;
    m_end_offset = (int32_t)_io()->pos();
    f_end_offset = true;
    return m_end_offset;
}

int32_t edk2_vss2_t::vss2_variable_t::len_alignment_padding_auth() {
    if (f_len_alignment_padding_auth)
        return m_len_alignment_padding_auth;
    m_len_alignment_padding_auth = ((((end_offset_auth() - offset()) + 3) & ~3) - (end_offset_auth() - offset()));
    f_len_alignment_padding_auth = true;
    return m_len_alignment_padding_auth;
}

bool edk2_vss2_t::vss2_variable_t::is_valid() {
    if (f_is_valid)
        return m_is_valid;
    m_is_valid =  ((state() == 127) || (state() == 63)) ;
    f_is_valid = true;
    return m_is_valid;
}

int32_t edk2_vss2_t::vss2_variable_t::offset() {
    if (f_offset)
        return m_offset;
    m_offset = (int32_t)_io()->pos();
    f_offset = true;
    return m_offset;
}

int32_t edk2_vss2_t::len_vss2_store_header() {
    if (f_len_vss2_store_header)
        return m_len_vss2_store_header;
    m_len_vss2_store_header = (7 * 4);
    f_len_vss2_store_header = true;
    return m_len_vss2_store_header;
}

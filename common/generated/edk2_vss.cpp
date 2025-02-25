// This is a generated file! Please edit source .ksy file and use kaitai-struct-compiler to rebuild

#include "edk2_vss.h"
#include "../kaitai/exceptions.h"

edk2_vss_t::edk2_vss_t(kaitai::kstream* p__io, kaitai::kstruct* p__parent, edk2_vss_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = this; (void)p__root;
    m_body = nullptr;
    m__io__raw_body = nullptr;
    f_len_vss_store_header = false;
    _read();
}

void edk2_vss_t::_read() {
    m_signature = m__io->read_u4le();
    {
        uint32_t _ = signature();
        if (!( ((_ == 1397970468) || (_ == 1398166308) || (_ == 1397968420)) )) {
            throw kaitai::validation_expr_error<uint32_t>(signature(), _io(), std::string("/seq/0"));
        }
    }
    m_vss_size = m__io->read_u4le();
    {
        uint32_t _ = vss_size();
        if (!( ((_ > static_cast<uint32_t>(len_vss_store_header())) && (_ < 4294967295UL)) )) {
            throw kaitai::validation_expr_error<uint32_t>(vss_size(), _io(), std::string("/seq/1"));
        }
    }
    m_format = m__io->read_u1();
    {
        uint8_t _ = format();
        if (!(_ == 90)) {
            throw kaitai::validation_expr_error<uint8_t>(format(), _io(), std::string("/seq/2"));
        }
    }
    m_state = m__io->read_u1();
    m_reserved = m__io->read_u2le();
    m_reserved1 = m__io->read_u4le();
    m__raw_body = m__io->read_bytes((vss_size() - len_vss_store_header()));
    m__io__raw_body = std::unique_ptr<kaitai::kstream>(new kaitai::kstream(m__raw_body));
    m_body = std::unique_ptr<vss_store_body_t>(new vss_store_body_t(m__io__raw_body.get(), this, m__root));
}

edk2_vss_t::~edk2_vss_t() {
    _clean_up();
}

void edk2_vss_t::_clean_up() {
}

edk2_vss_t::vss_store_body_t::vss_store_body_t(kaitai::kstream* p__io, edk2_vss_t* p__parent, edk2_vss_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;
    m_variables = nullptr;
    _read();
}

void edk2_vss_t::vss_store_body_t::_read() {
    m_variables = std::unique_ptr<std::vector<std::unique_ptr<vss_variable_t>>>(new std::vector<std::unique_ptr<vss_variable_t>>());
    {
        int i = 0;
        vss_variable_t* _;
        do {
            _ = new vss_variable_t(m__io, this, m__root);
            m_variables->push_back(std::move(std::unique_ptr<vss_variable_t>(_)));
            i++;
        } while (!( ((_->signature_first() != 170) || (_io()->is_eof())) ));
    }
}

edk2_vss_t::vss_store_body_t::~vss_store_body_t() {
    _clean_up();
}

void edk2_vss_t::vss_store_body_t::_clean_up() {
}

edk2_vss_t::vss_variable_attributes_t::vss_variable_attributes_t(kaitai::kstream* p__io, edk2_vss_t::vss_variable_t* p__parent, edk2_vss_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;
    _read();
}

void edk2_vss_t::vss_variable_attributes_t::_read() {
    m_non_volatile = m__io->read_bits_int_le(1);
    m_boot_service = m__io->read_bits_int_le(1);
    m_runtime = m__io->read_bits_int_le(1);
    m_hw_error_record = m__io->read_bits_int_le(1);
    m_auth_write = m__io->read_bits_int_le(1);
    m_time_based_auth = m__io->read_bits_int_le(1);
    m_append_write = m__io->read_bits_int_le(1);
    m_reserved = m__io->read_bits_int_le(24);
    m_apple_data_checksum = m__io->read_bits_int_le(1);
}

edk2_vss_t::vss_variable_attributes_t::~vss_variable_attributes_t() {
    _clean_up();
}

void edk2_vss_t::vss_variable_attributes_t::_clean_up() {
}

edk2_vss_t::vss_variable_t::vss_variable_t(kaitai::kstream* p__io, edk2_vss_t::vss_store_body_t* p__parent, edk2_vss_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;
    m_attributes = nullptr;
    f_is_auth = false;
    f_len_standard_header = false;
    f_is_intel_legacy = false;
    f_len_auth_header = false;
    f_len_apple_header = false;
    f_len_intel_legacy_header = false;
    f_is_valid = false;
    _read();
}

void edk2_vss_t::vss_variable_t::_read() {
    m_signature_first = m__io->read_u1();
    n_signature_last = true;
    if (signature_first() == 170) {
        n_signature_last = false;
        m_signature_last = m__io->read_u1();
        {
            uint8_t _ = signature_last();
            if (!(_ == 85)) {
                throw kaitai::validation_expr_error<uint8_t>(signature_last(), _io(), std::string("/types/vss_variable/seq/1"));
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
        m_attributes = std::unique_ptr<vss_variable_attributes_t>(new vss_variable_attributes_t(m__io, this, m__root));
    }
    n_len_total = true;
    if ( ((signature_first() == 170) && (is_intel_legacy())) ) {
        n_len_total = false;
        m_len_total = m__io->read_u4le();
        {
            uint32_t _ = len_total();
            if (!(_ >= ((static_cast<uint32_t>(len_intel_legacy_header()) + 4) + 1))) {
                throw kaitai::validation_expr_error<uint32_t>(len_total(), _io(), std::string("/types/vss_variable/seq/5"));
            }
        }
    }
    n_len_name = true;
    if ( ((signature_first() == 170) && (!(is_intel_legacy()))) ) {
        n_len_name = false;
        m_len_name = m__io->read_u4le();
    }
    n_len_data = true;
    if ( ((signature_first() == 170) && (!(is_intel_legacy()))) ) {
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
        {
            uint32_t _ = len_name_auth();
            if (!( ((_ >= 4) && (kaitai::kstream::mod(_, 2) == 0)) )) {
                throw kaitai::validation_expr_error<uint32_t>(len_name_auth(), _io(), std::string("/types/vss_variable/seq/10"));
            }
        }
    }
    n_len_data_auth = true;
    if ( ((signature_first() == 170) && (is_auth())) ) {
        n_len_data_auth = false;
        m_len_data_auth = m__io->read_u4le();
        {
            uint32_t _ = len_data_auth();
            if (!(_ > 0)) {
                throw kaitai::validation_expr_error<uint32_t>(len_data_auth(), _io(), std::string("/types/vss_variable/seq/11"));
            }
        }
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
    n_apple_data_crc32 = true;
    if ( ((signature_first() == 170) && (!(is_intel_legacy())) && (!(is_auth())) && (attributes()->apple_data_checksum())) ) {
        n_apple_data_crc32 = false;
        m_apple_data_crc32 = m__io->read_u4le();
    }
    n_intel_legacy_data = true;
    if ( ((signature_first() == 170) && (is_intel_legacy())) ) {
        n_intel_legacy_data = false;
        m_intel_legacy_data = m__io->read_bytes((len_total() - len_intel_legacy_header()));
    }
    n_name = true;
    if ( ((signature_first() == 170) && (!(is_intel_legacy())) && (!(is_auth()))) ) {
        n_name = false;
        m_name = m__io->read_bytes(len_name());
        {
            std::string _ = name();
            if (!( ((len_name() >= 4) && (kaitai::kstream::mod(len_name(), 2) == 0)) )) {
                throw kaitai::validation_expr_error<std::string>(name(), _io(), std::string("/types/vss_variable/seq/17"));
            }
        }
    }
    n_data = true;
    if ( ((signature_first() == 170) && (!(is_intel_legacy())) && (!(is_auth()))) ) {
        n_data = false;
        m_data = m__io->read_bytes(len_data());
        {
            std::string _ = data();
            if (!(len_name() > 0)) {
                throw kaitai::validation_expr_error<std::string>(data(), _io(), std::string("/types/vss_variable/seq/18"));
            }
        }
    }
}

edk2_vss_t::vss_variable_t::~vss_variable_t() {
    _clean_up();
}

void edk2_vss_t::vss_variable_t::_clean_up() {
    if (!n_signature_last) {
    }
    if (!n_state) {
    }
    if (!n_reserved) {
    }
    if (!n_attributes) {
    }
    if (!n_len_total) {
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
    if (!n_apple_data_crc32) {
    }
    if (!n_intel_legacy_data) {
    }
    if (!n_name) {
    }
    if (!n_data) {
    }
}

bool edk2_vss_t::vss_variable_t::is_auth() {
    if (f_is_auth)
        return m_is_auth;
    m_is_auth =  ((state() != 248) && (state() != 252) && ( (( ((attributes()->auth_write()) || (attributes()->time_based_auth()) || (attributes()->append_write())) ) || ( ((len_name() == 0) || (len_data() == 0)) )) )) ;
    f_is_auth = true;
    return m_is_auth;
}

int8_t edk2_vss_t::vss_variable_t::len_standard_header() {
    if (f_len_standard_header)
        return m_len_standard_header;
    m_len_standard_header = 32;
    f_len_standard_header = true;
    return m_len_standard_header;
}

bool edk2_vss_t::vss_variable_t::is_intel_legacy() {
    if (f_is_intel_legacy)
        return m_is_intel_legacy;
    m_is_intel_legacy =  ((state() == 248) || (state() == 252)) ;
    f_is_intel_legacy = true;
    return m_is_intel_legacy;
}

int8_t edk2_vss_t::vss_variable_t::len_auth_header() {
    if (f_len_auth_header)
        return m_len_auth_header;
    m_len_auth_header = 60;
    f_len_auth_header = true;
    return m_len_auth_header;
}

int8_t edk2_vss_t::vss_variable_t::len_apple_header() {
    if (f_len_apple_header)
        return m_len_apple_header;
    m_len_apple_header = 36;
    f_len_apple_header = true;
    return m_len_apple_header;
}

int8_t edk2_vss_t::vss_variable_t::len_intel_legacy_header() {
    if (f_len_intel_legacy_header)
        return m_len_intel_legacy_header;
    m_len_intel_legacy_header = 28;
    f_len_intel_legacy_header = true;
    return m_len_intel_legacy_header;
}

bool edk2_vss_t::vss_variable_t::is_valid() {
    if (f_is_valid)
        return m_is_valid;
    m_is_valid =  ((state() == 252) || (state() == 127) || (state() == 63)) ;
    f_is_valid = true;
    return m_is_valid;
}

int8_t edk2_vss_t::len_vss_store_header() {
    if (f_len_vss_store_header)
        return m_len_vss_store_header;
    m_len_vss_store_header = 16;
    f_len_vss_store_header = true;
    return m_len_vss_store_header;
}

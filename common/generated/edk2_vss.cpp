// This is a generated file! Please edit source .ksy file and use kaitai-struct-compiler to rebuild

#include "edk2_vss.h"
#include "../kaitai/exceptions.h"

edk2_vss_t::edk2_vss_t(kaitai::kstream* p__io, kaitai::kstruct* p__parent, edk2_vss_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = this; (void)p__root;
    m_body = nullptr;
    m__io__raw_body = nullptr;
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
    m_size = m__io->read_u4le();
    {
        uint32_t _ = size();
        if (!(_ > (4 * 4))) {
            throw kaitai::validation_expr_error<uint32_t>(size(), _io(), std::string("/seq/1"));
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
    m__raw_body = m__io->read_bytes((size() - (4 * 4)));
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
    n_vendor_guid = true;
    if (signature_first() == 170) {
        n_vendor_guid = false;
        m_vendor_guid = m__io->read_bytes(16);
    }
    n_apple_data_crc32 = true;
    if ( ((signature_first() == 170) && (attributes()->apple_data_checksum())) ) {
        n_apple_data_crc32 = false;
        m_apple_data_crc32 = m__io->read_u4le();
    }
    n_name = true;
    if (signature_first() == 170) {
        n_name = false;
        m_name = m__io->read_bytes(len_name());
    }
    n_data = true;
    if (signature_first() == 170) {
        n_data = false;
        m_data = m__io->read_bytes(len_data());
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
    if (!n_len_name) {
    }
    if (!n_len_data) {
    }
    if (!n_vendor_guid) {
    }
    if (!n_apple_data_crc32) {
    }
    if (!n_name) {
    }
    if (!n_data) {
    }
}

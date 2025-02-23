// This is a generated file! Please edit source .ksy file and use kaitai-struct-compiler to rebuild

#include "edk2_ftw.h"
#include "../kaitai/exceptions.h"

edk2_ftw_t::edk2_ftw_t(kaitai::kstream* p__io, kaitai::kstruct* p__parent, edk2_ftw_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = this; (void)p__root;
    f_len_ftw_store_header_32 = false;
    f_len_ftw_store_header_64 = false;
    _read();
}

void edk2_ftw_t::_read() {
    m_signature = m__io->read_u4le();
    {
        uint32_t _ = signature();
        if (!( ((_ == 4293995405UL) || (_ == 2656577835UL)) )) {
            throw kaitai::validation_expr_error<uint32_t>(signature(), _io(), std::string("/seq/0"));
        }
    }
    n_signature_main = true;
    if (signature() == 4293995405UL) {
        n_signature_main = false;
        m_signature_main = m__io->read_bytes(12);
        if (!(signature_main() == std::string("\x96\x76\x8B\x4C\xA9\x85\x27\x47\x07\x5B\x4F\x50", 12))) {
            throw kaitai::validation_not_equal_error<std::string>(std::string("\x96\x76\x8B\x4C\xA9\x85\x27\x47\x07\x5B\x4F\x50", 12), signature_main(), _io(), std::string("/seq/1"));
        }
    }
    n_signature_edk2_working_block = true;
    if (signature() == 2656577835UL) {
        n_signature_edk2_working_block = false;
        m_signature_edk2_working_block = m__io->read_bytes(12);
        if (!(signature_edk2_working_block() == std::string("\x68\x7C\x7D\x49\x0A\xCE\x65\x00\xFD\x9F\x1B\x95", 12))) {
            throw kaitai::validation_not_equal_error<std::string>(std::string("\x68\x7C\x7D\x49\x0A\xCE\x65\x00\xFD\x9F\x1B\x95", 12), signature_edk2_working_block(), _io(), std::string("/seq/2"));
        }
    }
    n_signature_vss2_working_block = true;
    if (signature() == 2656577835UL) {
        n_signature_vss2_working_block = false;
        m_signature_vss2_working_block = m__io->read_bytes(12);
        if (!(signature_vss2_working_block() == std::string("\x68\x7C\x7D\x49\xA0\xCE\x65\x00\xFD\x9F\x1B\x95", 12))) {
            throw kaitai::validation_not_equal_error<std::string>(std::string("\x68\x7C\x7D\x49\xA0\xCE\x65\x00\xFD\x9F\x1B\x95", 12), signature_vss2_working_block(), _io(), std::string("/seq/3"));
        }
    }
    m_crc = m__io->read_u4le();
    m_state = m__io->read_u1();
    m_reserved = m__io->read_bytes(3);
    m_len_write_queue_32 = m__io->read_u4le();
    n_len_write_queue_64 = true;
    if (kaitai::kstream::mod(len_write_queue_32(), 16) == 0) {
        n_len_write_queue_64 = false;
        m_len_write_queue_64 = m__io->read_u4le();
    }
    n_write_queue_32 = true;
    if (kaitai::kstream::mod(len_write_queue_32(), 16) == 4) {
        n_write_queue_32 = false;
        m_write_queue_32 = m__io->read_bytes(len_write_queue_32());
    }
    n_write_queue_64 = true;
    if (kaitai::kstream::mod(len_write_queue_32(), 16) == 0) {
        n_write_queue_64 = false;
        m_write_queue_64 = m__io->read_bytes(((static_cast<uint64_t>(len_write_queue_64()) << 32) + len_write_queue_32()));
    }
}

edk2_ftw_t::~edk2_ftw_t() {
    _clean_up();
}

void edk2_ftw_t::_clean_up() {
    if (!n_signature_main) {
    }
    if (!n_signature_edk2_working_block) {
    }
    if (!n_signature_vss2_working_block) {
    }
    if (!n_len_write_queue_64) {
    }
    if (!n_write_queue_32) {
    }
    if (!n_write_queue_64) {
    }
}

int8_t edk2_ftw_t::len_ftw_store_header_32() {
    if (f_len_ftw_store_header_32)
        return m_len_ftw_store_header_32;
    m_len_ftw_store_header_32 = 28;
    f_len_ftw_store_header_32 = true;
    return m_len_ftw_store_header_32;
}

int8_t edk2_ftw_t::len_ftw_store_header_64() {
    if (f_len_ftw_store_header_64)
        return m_len_ftw_store_header_64;
    m_len_ftw_store_header_64 = 32;
    f_len_ftw_store_header_64 = true;
    return m_len_ftw_store_header_64;
}

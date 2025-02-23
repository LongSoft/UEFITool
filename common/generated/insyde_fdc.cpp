// This is a generated file! Please edit source .ksy file and use kaitai-struct-compiler to rebuild

#include "insyde_fdc.h"
#include "../kaitai/exceptions.h"

insyde_fdc_t::insyde_fdc_t(kaitai::kstream* p__io, kaitai::kstruct* p__parent, insyde_fdc_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = this; (void)p__root;
    f_len_fdc_store_header = false;
    _read();
}

void insyde_fdc_t::_read() {
    m_signature = m__io->read_u4le();
    {
        uint32_t _ = signature();
        if (!(_ == 1128547935)) {
            throw kaitai::validation_expr_error<uint32_t>(signature(), _io(), std::string("/seq/0"));
        }
    }
    m_fdc_size = m__io->read_u4le();
    {
        uint32_t _ = fdc_size();
        if (!( ((_ > len_fdc_store_header()) && (_ < 4294967295UL)) )) {
            throw kaitai::validation_expr_error<uint32_t>(fdc_size(), _io(), std::string("/seq/1"));
        }
    }
    m_body = m__io->read_bytes((fdc_size() - len_fdc_store_header()));
}

insyde_fdc_t::~insyde_fdc_t() {
    _clean_up();
}

void insyde_fdc_t::_clean_up() {
}

int8_t insyde_fdc_t::len_fdc_store_header() {
    if (f_len_fdc_store_header)
        return m_len_fdc_store_header;
    m_len_fdc_store_header = 80;
    f_len_fdc_store_header = true;
    return m_len_fdc_store_header;
}

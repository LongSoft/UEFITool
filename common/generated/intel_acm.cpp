// This is a generated file! Please edit source .ksy file and use kaitai-struct-compiler to rebuild

#include "intel_acm.h"
#include "../kaitai/exceptions.h"

intel_acm_t::intel_acm_t(kaitai::kstream* p__io, kaitai::kstruct* p__parent, intel_acm_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = this; (void)p__root;
    m_header = 0;

    try {
        _read();
    } catch(...) {
        _clean_up();
        throw;
    }
}

void intel_acm_t::_read() {
    m_header = new header_t(m__io, this, m__root);
    m_body = m__io->read_bytes((4 * ((header()->module_size() - header()->header_size()) - header()->scratch_space_size())));
}

intel_acm_t::~intel_acm_t() {
    _clean_up();
}

void intel_acm_t::_clean_up() {
    if (m_header) {
        delete m_header; m_header = 0;
    }
}

intel_acm_t::header_t::header_t(kaitai::kstream* p__io, intel_acm_t* p__parent, intel_acm_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;

    try {
        _read();
    } catch(...) {
        _clean_up();
        throw;
    }
}

void intel_acm_t::header_t::_read() {
    m_module_type = m__io->read_u2le();
    if (!(module_type() == 2)) {
        throw kaitai::validation_not_equal_error<uint16_t>(2, module_type(), _io(), std::string("/types/header/seq/0"));
    }
    m_module_subtype = static_cast<intel_acm_t::module_subtype_t>(m__io->read_u2le());
    m_header_size = m__io->read_u4le();
    m_header_version = m__io->read_u4le();
    m_chipset_id = m__io->read_u2le();
    m_flags = m__io->read_u2le();
    m_module_vendor = m__io->read_u4le();
    if (!(module_vendor() == 32902)) {
        throw kaitai::validation_not_equal_error<uint32_t>(32902, module_vendor(), _io(), std::string("/types/header/seq/6"));
    }
    m_date_day = m__io->read_u1();
    m_date_month = m__io->read_u1();
    m_date_year = m__io->read_u2le();
    m_module_size = m__io->read_u4le();
    m_acm_svn = m__io->read_u2le();
    m_se_svn = m__io->read_u2le();
    m_code_control_flags = m__io->read_u4le();
    m_error_entry_point = m__io->read_u4le();
    m_gdt_max = m__io->read_u4le();
    m_gdt_base = m__io->read_u4le();
    m_segment_sel = m__io->read_u4le();
    m_entry_point = m__io->read_u4le();
    m_reserved = m__io->read_bytes(64);
    m_key_size = m__io->read_u4le();
    m_scratch_space_size = m__io->read_u4le();
    m_rsa_public_key = m__io->read_bytes((4 * key_size()));
    n_rsa_exponent = true;
    if (header_version() == 0) {
        n_rsa_exponent = false;
        m_rsa_exponent = m__io->read_u4le();
    }
    m_rsa_signature = m__io->read_bytes((4 * key_size()));
    m_scratch_space = m__io->read_bytes((4 * scratch_space_size()));
}

intel_acm_t::header_t::~header_t() {
    _clean_up();
}

void intel_acm_t::header_t::_clean_up() {
    if (!n_rsa_exponent) {
    }
}

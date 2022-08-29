// This is a generated file! Please edit source .ksy file and use kaitai-struct-compiler to rebuild

#include "intel_keym_v2.h"
#include "../kaitai/exceptions.h"

intel_keym_v2_t::intel_keym_v2_t(kaitai::kstream* p__io, kaitai::kstruct* p__parent, intel_keym_v2_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = this; (void)p__root;
    m_header = 0;
    m_reserved = 0;
    m_km_hashes = 0;
    m_key_signature = 0;

    try {
        _read();
    } catch(...) {
        _clean_up();
        throw;
    }
}

void intel_keym_v2_t::_read() {
    m_header = new header_t(m__io, this, m__root);
    m_key_signature_offset = m__io->read_u2le();
    m_reserved = new std::vector<uint8_t>();
    const int l_reserved = 3;
    for (int i = 0; i < l_reserved; i++) {
        m_reserved->push_back(m__io->read_u1());
    }
    m_km_version = m__io->read_u1();
    m_km_svn = m__io->read_u1();
    m_km_id = m__io->read_u1();
    m_fpf_hash_algorithm_id = m__io->read_u2le();
    m_num_km_hashes = m__io->read_u2le();
    m_km_hashes = new std::vector<km_hash_t*>();
    const int l_km_hashes = num_km_hashes();
    for (int i = 0; i < l_km_hashes; i++) {
        m_km_hashes->push_back(new km_hash_t(m__io, this, m__root));
    }
    m_key_signature = new key_signature_t(m__io, this, m__root);
}

intel_keym_v2_t::~intel_keym_v2_t() {
    _clean_up();
}

void intel_keym_v2_t::_clean_up() {
    if (m_header) {
        delete m_header; m_header = 0;
    }
    if (m_reserved) {
        delete m_reserved; m_reserved = 0;
    }
    if (m_km_hashes) {
        for (std::vector<km_hash_t*>::iterator it = m_km_hashes->begin(); it != m_km_hashes->end(); ++it) {
            delete *it;
        }
        delete m_km_hashes; m_km_hashes = 0;
    }
    if (m_key_signature) {
        delete m_key_signature; m_key_signature = 0;
    }
}

intel_keym_v2_t::key_signature_t::key_signature_t(kaitai::kstream* p__io, intel_keym_v2_t* p__parent, intel_keym_v2_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;
    m_public_key = 0;
    m_signature = 0;

    try {
        _read();
    } catch(...) {
        _clean_up();
        throw;
    }
}

void intel_keym_v2_t::key_signature_t::_read() {
    m_version = m__io->read_u1();
    m_key_id = m__io->read_u2le();
    m_public_key = new public_key_t(m__io, this, m__root);
    m_sig_scheme = m__io->read_u2le();
    m_signature = new signature_t(m__io, this, m__root);
}

intel_keym_v2_t::key_signature_t::~key_signature_t() {
    _clean_up();
}

void intel_keym_v2_t::key_signature_t::_clean_up() {
    if (m_public_key) {
        delete m_public_key; m_public_key = 0;
    }
    if (m_signature) {
        delete m_signature; m_signature = 0;
    }
}

intel_keym_v2_t::km_hash_t::km_hash_t(kaitai::kstream* p__io, intel_keym_v2_t* p__parent, intel_keym_v2_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;

    try {
        _read();
    } catch(...) {
        _clean_up();
        throw;
    }
}

void intel_keym_v2_t::km_hash_t::_read() {
    m_usage_flags = m__io->read_u8le();
    m_hash_algorithm_id = m__io->read_u2le();
    m_len_hash = m__io->read_u2le();
    m_hash = m__io->read_bytes(len_hash());
}

intel_keym_v2_t::km_hash_t::~km_hash_t() {
    _clean_up();
}

void intel_keym_v2_t::km_hash_t::_clean_up() {
}

intel_keym_v2_t::signature_t::signature_t(kaitai::kstream* p__io, intel_keym_v2_t::key_signature_t* p__parent, intel_keym_v2_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;

    try {
        _read();
    } catch(...) {
        _clean_up();
        throw;
    }
}

void intel_keym_v2_t::signature_t::_read() {
    m_version = m__io->read_u1();
    m_size_bits = m__io->read_u2le();
    m_hash_algorithm_id = m__io->read_u2le();
    m_signature = m__io->read_bytes((size_bits() / 8));
}

intel_keym_v2_t::signature_t::~signature_t() {
    _clean_up();
}

void intel_keym_v2_t::signature_t::_clean_up() {
}

intel_keym_v2_t::public_key_t::public_key_t(kaitai::kstream* p__io, intel_keym_v2_t::key_signature_t* p__parent, intel_keym_v2_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;

    try {
        _read();
    } catch(...) {
        _clean_up();
        throw;
    }
}

void intel_keym_v2_t::public_key_t::_read() {
    m_version = m__io->read_u1();
    m_size_bits = m__io->read_u2le();
    m_exponent = m__io->read_u4le();
    m_modulus = m__io->read_bytes((size_bits() / 8));
}

intel_keym_v2_t::public_key_t::~public_key_t() {
    _clean_up();
}

void intel_keym_v2_t::public_key_t::_clean_up() {
}

intel_keym_v2_t::header_t::header_t(kaitai::kstream* p__io, intel_keym_v2_t* p__parent, intel_keym_v2_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;

    try {
        _read();
    } catch(...) {
        _clean_up();
        throw;
    }
}

void intel_keym_v2_t::header_t::_read() {
    m_structure_id = static_cast<intel_keym_v2_t::structure_ids_t>(m__io->read_u8le());
    if (!(structure_id() == intel_keym_v2_t::STRUCTURE_IDS_KEYM)) {
        throw kaitai::validation_not_equal_error<intel_keym_v2_t::structure_ids_t>(intel_keym_v2_t::STRUCTURE_IDS_KEYM, structure_id(), _io(), std::string("/types/header/seq/0"));
    }
    m_version = m__io->read_u1();
    {
        uint8_t _ = version();
        if (!(_ >= 32)) {
            throw kaitai::validation_expr_error<uint8_t>(version(), _io(), std::string("/types/header/seq/1"));
        }
    }
    m_header_specific = m__io->read_u1();
    m_total_size = m__io->read_u2le();
    if (!(total_size() == 0)) {
        throw kaitai::validation_not_equal_error<uint16_t>(0, total_size(), _io(), std::string("/types/header/seq/3"));
    }
}

intel_keym_v2_t::header_t::~header_t() {
    _clean_up();
}

void intel_keym_v2_t::header_t::_clean_up() {
}

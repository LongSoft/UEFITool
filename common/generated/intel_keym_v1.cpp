// This is a generated file! Please edit source .ksy file and use kaitai-struct-compiler to rebuild

#include "intel_keym_v1.h"
#include "../kaitai/exceptions.h"

intel_keym_v1_t::intel_keym_v1_t(kaitai::kstream* p__io, kaitai::kstruct* p__parent, intel_keym_v1_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = this; (void)p__root;
    m_km_hash = 0;
    m_key_signature = 0;

    try {
        _read();
    } catch(...) {
        _clean_up();
        throw;
    }
}

void intel_keym_v1_t::_read() {
    m_structure_id = static_cast<intel_keym_v1_t::structure_ids_t>(m__io->read_u8le());
    if (!(structure_id() == intel_keym_v1_t::STRUCTURE_IDS_KEYM)) {
        throw kaitai::validation_not_equal_error<intel_keym_v1_t::structure_ids_t>(intel_keym_v1_t::STRUCTURE_IDS_KEYM, structure_id(), _io(), std::string("/seq/0"));
    }
    m_version = m__io->read_u1();
    {
        uint8_t _ = version();
        if (!(_ < 32)) {
            throw kaitai::validation_expr_error<uint8_t>(version(), _io(), std::string("/seq/1"));
        }
    }
    m_km_version = m__io->read_u1();
    m_km_svn = m__io->read_u1();
    m_km_id = m__io->read_u1();
    m_km_hash = new km_hash_t(m__io, this, m__root);
    m_key_signature = new key_signature_t(m__io, this, m__root);
}

intel_keym_v1_t::~intel_keym_v1_t() {
    _clean_up();
}

void intel_keym_v1_t::_clean_up() {
    if (m_km_hash) {
        delete m_km_hash; m_km_hash = 0;
    }
    if (m_key_signature) {
        delete m_key_signature; m_key_signature = 0;
    }
}

intel_keym_v1_t::km_hash_t::km_hash_t(kaitai::kstream* p__io, intel_keym_v1_t* p__parent, intel_keym_v1_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;

    try {
        _read();
    } catch(...) {
        _clean_up();
        throw;
    }
}

void intel_keym_v1_t::km_hash_t::_read() {
    m_hash_algorithm_id = m__io->read_u2le();
    m_len_hash = m__io->read_u2le();
    m_hash = m__io->read_bytes(len_hash());
}

intel_keym_v1_t::km_hash_t::~km_hash_t() {
    _clean_up();
}

void intel_keym_v1_t::km_hash_t::_clean_up() {
}

intel_keym_v1_t::public_key_t::public_key_t(kaitai::kstream* p__io, intel_keym_v1_t::key_signature_t* p__parent, intel_keym_v1_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;

    try {
        _read();
    } catch(...) {
        _clean_up();
        throw;
    }
}

void intel_keym_v1_t::public_key_t::_read() {
    m_version = m__io->read_u1();
    m_size_bits = m__io->read_u2le();
    m_exponent = m__io->read_u4le();
    m_modulus = m__io->read_bytes((size_bits() / 8));
}

intel_keym_v1_t::public_key_t::~public_key_t() {
    _clean_up();
}

void intel_keym_v1_t::public_key_t::_clean_up() {
}

intel_keym_v1_t::signature_t::signature_t(kaitai::kstream* p__io, intel_keym_v1_t::key_signature_t* p__parent, intel_keym_v1_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;

    try {
        _read();
    } catch(...) {
        _clean_up();
        throw;
    }
}

void intel_keym_v1_t::signature_t::_read() {
    m_version = m__io->read_u1();
    m_size_bits = m__io->read_u2le();
    m_hash_algorithm_id = m__io->read_u2le();
    m_signature = m__io->read_bytes((size_bits() / 8));
}

intel_keym_v1_t::signature_t::~signature_t() {
    _clean_up();
}

void intel_keym_v1_t::signature_t::_clean_up() {
}

intel_keym_v1_t::key_signature_t::key_signature_t(kaitai::kstream* p__io, intel_keym_v1_t* p__parent, intel_keym_v1_t* p__root) : kaitai::kstruct(p__io) {
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

void intel_keym_v1_t::key_signature_t::_read() {
    m_version = m__io->read_u1();
    m_key_id = m__io->read_u2le();
    m_public_key = new public_key_t(m__io, this, m__root);
    m_sig_scheme = m__io->read_u2le();
    m_signature = new signature_t(m__io, this, m__root);
}

intel_keym_v1_t::key_signature_t::~key_signature_t() {
    _clean_up();
}

void intel_keym_v1_t::key_signature_t::_clean_up() {
    if (m_public_key) {
        delete m_public_key; m_public_key = 0;
    }
    if (m_signature) {
        delete m_signature; m_signature = 0;
    }
}

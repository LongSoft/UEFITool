// This is a generated file! Please edit source .ksy file and use kaitai-struct-compiler to rebuild

#include "intel_acbp_v2.h"
#include "../kaitai/exceptions.h"

intel_acbp_v2_t::intel_acbp_v2_t(kaitai::kstream* p__io, kaitai::kstruct* p__parent, intel_acbp_v2_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = this; (void)p__root;
    m_elements = 0;
    m_key_signature = 0;

    try {
        _read();
    } catch(...) {
        _clean_up();
        throw;
    }
}

void intel_acbp_v2_t::_read() {
    m_structure_id = static_cast<intel_acbp_v2_t::structure_ids_t>(m__io->read_u8le());
    if (!(structure_id() == intel_acbp_v2_t::STRUCTURE_IDS_ACBP)) {
        throw kaitai::validation_not_equal_error<intel_acbp_v2_t::structure_ids_t>(intel_acbp_v2_t::STRUCTURE_IDS_ACBP, structure_id(), _io(), std::string("/seq/0"));
    }
    m_version = m__io->read_u1();
    {
        uint8_t _ = version();
        if (!(_ >= 32)) {
            throw kaitai::validation_expr_error<uint8_t>(version(), _io(), std::string("/seq/1"));
        }
    }
    m_header_specific = m__io->read_u1();
    m_total_size = m__io->read_u2le();
    if (!(total_size() == 20)) {
        throw kaitai::validation_not_equal_error<uint16_t>(20, total_size(), _io(), std::string("/seq/3"));
    }
    m_key_signature_offset = m__io->read_u2le();
    m_bpm_revision = m__io->read_u1();
    m_bp_svn = m__io->read_u1();
    m_acm_svn = m__io->read_u1();
    m_reserved = m__io->read_u1();
    m_nem_data_size = m__io->read_u2le();
    m_elements = new std::vector<acbp_element_t*>();
    {
        int i = 0;
        acbp_element_t* _;
        do {
            _ = new acbp_element_t(m__io, this, m__root);
            m_elements->push_back(_);
            i++;
        } while (!( ((_->header()->total_size() == 0) || (_->header()->structure_id() == intel_acbp_v2_t::STRUCTURE_IDS_PMSG)) ));
    }
    m_key_signature = new key_signature_t(m__io, this, m__root);
}

intel_acbp_v2_t::~intel_acbp_v2_t() {
    _clean_up();
}

void intel_acbp_v2_t::_clean_up() {
    if (m_elements) {
        for (std::vector<acbp_element_t*>::iterator it = m_elements->begin(); it != m_elements->end(); ++it) {
            delete *it;
        }
        delete m_elements; m_elements = 0;
    }
    if (m_key_signature) {
        delete m_key_signature; m_key_signature = 0;
    }
}

intel_acbp_v2_t::acbp_element_t::acbp_element_t(kaitai::kstream* p__io, intel_acbp_v2_t* p__parent, intel_acbp_v2_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;
    m_header = 0;
    m_ibbs_body = 0;
    m_pmda_body = 0;

    try {
        _read();
    } catch(...) {
        _clean_up();
        throw;
    }
}

void intel_acbp_v2_t::acbp_element_t::_read() {
    m_header = new header_t(m__io, this, m__root);
    n_ibbs_body = true;
    if ( ((header()->structure_id() == intel_acbp_v2_t::STRUCTURE_IDS_IBBS) && (header()->total_size() >= 12)) ) {
        n_ibbs_body = false;
        m_ibbs_body = new ibbs_body_t(m__io, this, m__root);
    }
    n_pmda_body = true;
    if ( ((header()->structure_id() == intel_acbp_v2_t::STRUCTURE_IDS_PMDA) && (header()->total_size() >= 12)) ) {
        n_pmda_body = false;
        m_pmda_body = new pmda_body_t(m__io, this, m__root);
    }
    n_generic_body = true;
    if ( ((header()->structure_id() != intel_acbp_v2_t::STRUCTURE_IDS_IBBS) && (header()->structure_id() != intel_acbp_v2_t::STRUCTURE_IDS_PMDA) && (header()->total_size() >= 12)) ) {
        n_generic_body = false;
        m_generic_body = m__io->read_bytes((header()->total_size() - 12));
    }
}

intel_acbp_v2_t::acbp_element_t::~acbp_element_t() {
    _clean_up();
}

void intel_acbp_v2_t::acbp_element_t::_clean_up() {
    if (m_header) {
        delete m_header; m_header = 0;
    }
    if (!n_ibbs_body) {
        if (m_ibbs_body) {
            delete m_ibbs_body; m_ibbs_body = 0;
        }
    }
    if (!n_pmda_body) {
        if (m_pmda_body) {
            delete m_pmda_body; m_pmda_body = 0;
        }
    }
    if (!n_generic_body) {
    }
}

intel_acbp_v2_t::key_signature_t::key_signature_t(kaitai::kstream* p__io, intel_acbp_v2_t* p__parent, intel_acbp_v2_t* p__root) : kaitai::kstruct(p__io) {
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

void intel_acbp_v2_t::key_signature_t::_read() {
    m_version = m__io->read_u1();
    m_key_id = m__io->read_u2le();
    m_public_key = new public_key_t(m__io, this, m__root);
    m_sig_scheme = m__io->read_u2le();
    m_signature = new signature_t(m__io, this, m__root);
}

intel_acbp_v2_t::key_signature_t::~key_signature_t() {
    _clean_up();
}

void intel_acbp_v2_t::key_signature_t::_clean_up() {
    if (m_public_key) {
        delete m_public_key; m_public_key = 0;
    }
    if (m_signature) {
        delete m_signature; m_signature = 0;
    }
}

intel_acbp_v2_t::signature_t::signature_t(kaitai::kstream* p__io, intel_acbp_v2_t::key_signature_t* p__parent, intel_acbp_v2_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;

    try {
        _read();
    } catch(...) {
        _clean_up();
        throw;
    }
}

void intel_acbp_v2_t::signature_t::_read() {
    m_version = m__io->read_u1();
    m_size_bits = m__io->read_u2le();
    m_hash_algorithm_id = m__io->read_u2le();
    m_signature = m__io->read_bytes((size_bits() / 8));
}

intel_acbp_v2_t::signature_t::~signature_t() {
    _clean_up();
}

void intel_acbp_v2_t::signature_t::_clean_up() {
}

intel_acbp_v2_t::ibb_segment_t::ibb_segment_t(kaitai::kstream* p__io, intel_acbp_v2_t::ibbs_body_t* p__parent, intel_acbp_v2_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;

    try {
        _read();
    } catch(...) {
        _clean_up();
        throw;
    }
}

void intel_acbp_v2_t::ibb_segment_t::_read() {
    m_reserved = m__io->read_u2le();
    m_flags = m__io->read_u2le();
    m_base = m__io->read_u4le();
    m_size = m__io->read_u4le();
}

intel_acbp_v2_t::ibb_segment_t::~ibb_segment_t() {
    _clean_up();
}

void intel_acbp_v2_t::ibb_segment_t::_clean_up() {
}

intel_acbp_v2_t::public_key_t::public_key_t(kaitai::kstream* p__io, intel_acbp_v2_t::key_signature_t* p__parent, intel_acbp_v2_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;

    try {
        _read();
    } catch(...) {
        _clean_up();
        throw;
    }
}

void intel_acbp_v2_t::public_key_t::_read() {
    m_version = m__io->read_u1();
    m_size_bits = m__io->read_u2le();
    m_exponent = m__io->read_u4le();
    m_modulus = m__io->read_bytes((size_bits() / 8));
}

intel_acbp_v2_t::public_key_t::~public_key_t() {
    _clean_up();
}

void intel_acbp_v2_t::public_key_t::_clean_up() {
}

intel_acbp_v2_t::hash_t::hash_t(kaitai::kstream* p__io, kaitai::kstruct* p__parent, intel_acbp_v2_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;

    try {
        _read();
    } catch(...) {
        _clean_up();
        throw;
    }
}

void intel_acbp_v2_t::hash_t::_read() {
    m_hash_algorithm_id = m__io->read_u2le();
    m_len_hash = m__io->read_u2le();
    m_hash = m__io->read_bytes(len_hash());
}

intel_acbp_v2_t::hash_t::~hash_t() {
    _clean_up();
}

void intel_acbp_v2_t::hash_t::_clean_up() {
}

intel_acbp_v2_t::header_t::header_t(kaitai::kstream* p__io, intel_acbp_v2_t::acbp_element_t* p__parent, intel_acbp_v2_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;

    try {
        _read();
    } catch(...) {
        _clean_up();
        throw;
    }
}

void intel_acbp_v2_t::header_t::_read() {
    m_structure_id = static_cast<intel_acbp_v2_t::structure_ids_t>(m__io->read_u8le());
    m_version = m__io->read_u1();
    m_header_specific = m__io->read_u1();
    m_total_size = m__io->read_u2le();
}

intel_acbp_v2_t::header_t::~header_t() {
    _clean_up();
}

void intel_acbp_v2_t::header_t::_clean_up() {
}

intel_acbp_v2_t::pmda_entry_v3_t::pmda_entry_v3_t(kaitai::kstream* p__io, intel_acbp_v2_t::pmda_body_t* p__parent, intel_acbp_v2_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;
    m_hash = 0;

    try {
        _read();
    } catch(...) {
        _clean_up();
        throw;
    }
}

void intel_acbp_v2_t::pmda_entry_v3_t::_read() {
    m_entry_id = m__io->read_u4le();
    m_base = m__io->read_u4le();
    m_size = m__io->read_u4le();
    m_total_entry_size = m__io->read_u2le();
    m_version = m__io->read_u2le();
    m_hash = new hash_t(m__io, this, m__root);
}

intel_acbp_v2_t::pmda_entry_v3_t::~pmda_entry_v3_t() {
    _clean_up();
}

void intel_acbp_v2_t::pmda_entry_v3_t::_clean_up() {
    if (m_hash) {
        delete m_hash; m_hash = 0;
    }
}

intel_acbp_v2_t::ibbs_body_t::ibbs_body_t(kaitai::kstream* p__io, intel_acbp_v2_t::acbp_element_t* p__parent, intel_acbp_v2_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;
    m_post_ibb_digest = 0;
    m_ibb_digests = 0;
    m_obb_digest = 0;
    m_reserved2 = 0;
    m_ibb_segments = 0;

    try {
        _read();
    } catch(...) {
        _clean_up();
        throw;
    }
}

void intel_acbp_v2_t::ibbs_body_t::_read() {
    m_reserved0 = m__io->read_u1();
    m_set_number = m__io->read_u1();
    m_reserved1 = m__io->read_u1();
    m_pbet_value = m__io->read_u1();
    m_flags = m__io->read_u4le();
    m_mch_bar = m__io->read_u8le();
    m_vtd_bar = m__io->read_u8le();
    m_dma_protection_base0 = m__io->read_u4le();
    m_dma_protection_limit0 = m__io->read_u4le();
    m_dma_protection_base1 = m__io->read_u8le();
    m_dma_protection_limit1 = m__io->read_u8le();
    m_post_ibb_digest = new hash_t(m__io, this, m__root);
    m_ibb_entry_point = m__io->read_u4le();
    m_ibb_digests_size = m__io->read_u2le();
    m_num_ibb_digests = m__io->read_u2le();
    m_ibb_digests = new std::vector<hash_t*>();
    const int l_ibb_digests = num_ibb_digests();
    for (int i = 0; i < l_ibb_digests; i++) {
        m_ibb_digests->push_back(new hash_t(m__io, this, m__root));
    }
    m_obb_digest = new hash_t(m__io, this, m__root);
    m_reserved2 = new std::vector<uint8_t>();
    const int l_reserved2 = 3;
    for (int i = 0; i < l_reserved2; i++) {
        m_reserved2->push_back(m__io->read_u1());
    }
    m_num_ibb_segments = m__io->read_u1();
    m_ibb_segments = new std::vector<ibb_segment_t*>();
    const int l_ibb_segments = num_ibb_segments();
    for (int i = 0; i < l_ibb_segments; i++) {
        m_ibb_segments->push_back(new ibb_segment_t(m__io, this, m__root));
    }
}

intel_acbp_v2_t::ibbs_body_t::~ibbs_body_t() {
    _clean_up();
}

void intel_acbp_v2_t::ibbs_body_t::_clean_up() {
    if (m_post_ibb_digest) {
        delete m_post_ibb_digest; m_post_ibb_digest = 0;
    }
    if (m_ibb_digests) {
        for (std::vector<hash_t*>::iterator it = m_ibb_digests->begin(); it != m_ibb_digests->end(); ++it) {
            delete *it;
        }
        delete m_ibb_digests; m_ibb_digests = 0;
    }
    if (m_obb_digest) {
        delete m_obb_digest; m_obb_digest = 0;
    }
    if (m_reserved2) {
        delete m_reserved2; m_reserved2 = 0;
    }
    if (m_ibb_segments) {
        for (std::vector<ibb_segment_t*>::iterator it = m_ibb_segments->begin(); it != m_ibb_segments->end(); ++it) {
            delete *it;
        }
        delete m_ibb_segments; m_ibb_segments = 0;
    }
}

intel_acbp_v2_t::pmda_body_t::pmda_body_t(kaitai::kstream* p__io, intel_acbp_v2_t::acbp_element_t* p__parent, intel_acbp_v2_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;
    m_entries = 0;

    try {
        _read();
    } catch(...) {
        _clean_up();
        throw;
    }
}

void intel_acbp_v2_t::pmda_body_t::_read() {
    m_reserved = m__io->read_u2le();
    m_total_size = m__io->read_u2le();
    m_version = m__io->read_u4le();
    if (!(version() == 3)) {
        throw kaitai::validation_not_equal_error<uint32_t>(3, version(), _io(), std::string("/types/pmda_body/seq/2"));
    }
    m_num_entries = m__io->read_u4le();
    m_entries = new std::vector<pmda_entry_v3_t*>();
    const int l_entries = num_entries();
    for (int i = 0; i < l_entries; i++) {
        m_entries->push_back(new pmda_entry_v3_t(m__io, this, m__root));
    }
}

intel_acbp_v2_t::pmda_body_t::~pmda_body_t() {
    _clean_up();
}

void intel_acbp_v2_t::pmda_body_t::_clean_up() {
    if (m_entries) {
        for (std::vector<pmda_entry_v3_t*>::iterator it = m_entries->begin(); it != m_entries->end(); ++it) {
            delete *it;
        }
        delete m_entries; m_entries = 0;
    }
}

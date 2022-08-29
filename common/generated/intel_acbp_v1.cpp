// This is a generated file! Please edit source .ksy file and use kaitai-struct-compiler to rebuild

#include "intel_acbp_v1.h"
#include "../kaitai/exceptions.h"

intel_acbp_v1_t::intel_acbp_v1_t(kaitai::kstream* p__io, kaitai::kstruct* p__parent, intel_acbp_v1_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = this; (void)p__root;
    m_elements = 0;

    try {
        _read();
    } catch(...) {
        _clean_up();
        throw;
    }
}

void intel_acbp_v1_t::_read() {
    m_structure_id = static_cast<intel_acbp_v1_t::structure_ids_t>(m__io->read_u8le());
    if (!(structure_id() == intel_acbp_v1_t::STRUCTURE_IDS_ACBP)) {
        throw kaitai::validation_not_equal_error<intel_acbp_v1_t::structure_ids_t>(intel_acbp_v1_t::STRUCTURE_IDS_ACBP, structure_id(), _io(), std::string("/seq/0"));
    }
    m_version = m__io->read_u1();
    {
        uint8_t _ = version();
        if (!(_ < 32)) {
            throw kaitai::validation_expr_error<uint8_t>(version(), _io(), std::string("/seq/1"));
        }
    }
    m_reserved0 = m__io->read_u1();
    m_bpm_revision = m__io->read_u1();
    m_bp_svn = m__io->read_u1();
    m_acm_svn = m__io->read_u1();
    m_reserved1 = m__io->read_u1();
    m_nem_data_size = m__io->read_u2le();
    m_elements = new std::vector<acbp_element_t*>();
    {
        int i = 0;
        acbp_element_t* _;
        do {
            _ = new acbp_element_t(m__io, this, m__root);
            m_elements->push_back(_);
            i++;
        } while (!( ((_->header()->structure_id() == intel_acbp_v1_t::STRUCTURE_IDS_PMSG) || (_io()->is_eof())) ));
    }
}

intel_acbp_v1_t::~intel_acbp_v1_t() {
    _clean_up();
}

void intel_acbp_v1_t::_clean_up() {
    if (m_elements) {
        for (std::vector<acbp_element_t*>::iterator it = m_elements->begin(); it != m_elements->end(); ++it) {
            delete *it;
        }
        delete m_elements; m_elements = 0;
    }
}

intel_acbp_v1_t::pmsg_body_t::pmsg_body_t(kaitai::kstream* p__io, intel_acbp_v1_t::acbp_element_t* p__parent, intel_acbp_v1_t* p__root) : kaitai::kstruct(p__io) {
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

void intel_acbp_v1_t::pmsg_body_t::_read() {
    m_version = m__io->read_u1();
    m_key_id = m__io->read_u2le();
    m_public_key = new public_key_t(m__io, this, m__root);
    m_sig_scheme = m__io->read_u2le();
    m_signature = new signature_t(m__io, this, m__root);
}

intel_acbp_v1_t::pmsg_body_t::~pmsg_body_t() {
    _clean_up();
}

void intel_acbp_v1_t::pmsg_body_t::_clean_up() {
    if (m_public_key) {
        delete m_public_key; m_public_key = 0;
    }
    if (m_signature) {
        delete m_signature; m_signature = 0;
    }
}

intel_acbp_v1_t::acbp_element_t::acbp_element_t(kaitai::kstream* p__io, intel_acbp_v1_t* p__parent, intel_acbp_v1_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;
    m_header = 0;
    m_ibbs_body = 0;
    m_pmda_body = 0;
    m_pmsg_body = 0;

    try {
        _read();
    } catch(...) {
        _clean_up();
        throw;
    }
}

void intel_acbp_v1_t::acbp_element_t::_read() {
    m_header = new common_header_t(m__io, this, m__root);
    n_ibbs_body = true;
    if (header()->structure_id() == intel_acbp_v1_t::STRUCTURE_IDS_IBBS) {
        n_ibbs_body = false;
        m_ibbs_body = new ibbs_body_t(m__io, this, m__root);
    }
    n_pmda_body = true;
    if (header()->structure_id() == intel_acbp_v1_t::STRUCTURE_IDS_PMDA) {
        n_pmda_body = false;
        m_pmda_body = new pmda_body_t(m__io, this, m__root);
    }
    n_pmsg_body = true;
    if (header()->structure_id() == intel_acbp_v1_t::STRUCTURE_IDS_PMSG) {
        n_pmsg_body = false;
        m_pmsg_body = new pmsg_body_t(m__io, this, m__root);
    }
    n_invalid_body = true;
    if ( ((header()->structure_id() != intel_acbp_v1_t::STRUCTURE_IDS_PMSG) && (header()->structure_id() != intel_acbp_v1_t::STRUCTURE_IDS_PMDA) && (header()->structure_id() != intel_acbp_v1_t::STRUCTURE_IDS_IBBS)) ) {
        n_invalid_body = false;
        m_invalid_body = m__io->read_bytes(0);
        {
            std::string _ = invalid_body();
            if (!(false)) {
                throw kaitai::validation_expr_error<std::string>(invalid_body(), _io(), std::string("/types/acbp_element/seq/4"));
            }
        }
    }
}

intel_acbp_v1_t::acbp_element_t::~acbp_element_t() {
    _clean_up();
}

void intel_acbp_v1_t::acbp_element_t::_clean_up() {
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
    if (!n_pmsg_body) {
        if (m_pmsg_body) {
            delete m_pmsg_body; m_pmsg_body = 0;
        }
    }
    if (!n_invalid_body) {
    }
}

intel_acbp_v1_t::common_header_t::common_header_t(kaitai::kstream* p__io, intel_acbp_v1_t::acbp_element_t* p__parent, intel_acbp_v1_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;

    try {
        _read();
    } catch(...) {
        _clean_up();
        throw;
    }
}

void intel_acbp_v1_t::common_header_t::_read() {
    m_structure_id = static_cast<intel_acbp_v1_t::structure_ids_t>(m__io->read_u8le());
    m_version = m__io->read_u1();
}

intel_acbp_v1_t::common_header_t::~common_header_t() {
    _clean_up();
}

void intel_acbp_v1_t::common_header_t::_clean_up() {
}

intel_acbp_v1_t::signature_t::signature_t(kaitai::kstream* p__io, intel_acbp_v1_t::pmsg_body_t* p__parent, intel_acbp_v1_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;

    try {
        _read();
    } catch(...) {
        _clean_up();
        throw;
    }
}

void intel_acbp_v1_t::signature_t::_read() {
    m_version = m__io->read_u1();
    m_size_bits = m__io->read_u2le();
    m_hash_algorithm_id = m__io->read_u2le();
    m_signature = m__io->read_bytes((size_bits() / 8));
}

intel_acbp_v1_t::signature_t::~signature_t() {
    _clean_up();
}

void intel_acbp_v1_t::signature_t::_clean_up() {
}

intel_acbp_v1_t::pmda_entry_v1_t::pmda_entry_v1_t(kaitai::kstream* p__io, intel_acbp_v1_t::pmda_body_t* p__parent, intel_acbp_v1_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;

    try {
        _read();
    } catch(...) {
        _clean_up();
        throw;
    }
}

void intel_acbp_v1_t::pmda_entry_v1_t::_read() {
    m_base = m__io->read_u4le();
    m_size = m__io->read_u4le();
    m_hash = m__io->read_bytes(32);
}

intel_acbp_v1_t::pmda_entry_v1_t::~pmda_entry_v1_t() {
    _clean_up();
}

void intel_acbp_v1_t::pmda_entry_v1_t::_clean_up() {
}

intel_acbp_v1_t::ibb_segment_t::ibb_segment_t(kaitai::kstream* p__io, intel_acbp_v1_t::ibbs_body_t* p__parent, intel_acbp_v1_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;

    try {
        _read();
    } catch(...) {
        _clean_up();
        throw;
    }
}

void intel_acbp_v1_t::ibb_segment_t::_read() {
    m_reserved = m__io->read_u2le();
    m_flags = m__io->read_u2le();
    m_base = m__io->read_u4le();
    m_size = m__io->read_u4le();
}

intel_acbp_v1_t::ibb_segment_t::~ibb_segment_t() {
    _clean_up();
}

void intel_acbp_v1_t::ibb_segment_t::_clean_up() {
}

intel_acbp_v1_t::public_key_t::public_key_t(kaitai::kstream* p__io, intel_acbp_v1_t::pmsg_body_t* p__parent, intel_acbp_v1_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;

    try {
        _read();
    } catch(...) {
        _clean_up();
        throw;
    }
}

void intel_acbp_v1_t::public_key_t::_read() {
    m_version = m__io->read_u1();
    m_size_bits = m__io->read_u2le();
    m_exponent = m__io->read_u4le();
    m_modulus = m__io->read_bytes((size_bits() / 8));
}

intel_acbp_v1_t::public_key_t::~public_key_t() {
    _clean_up();
}

void intel_acbp_v1_t::public_key_t::_clean_up() {
}

intel_acbp_v1_t::hash_t::hash_t(kaitai::kstream* p__io, kaitai::kstruct* p__parent, intel_acbp_v1_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;

    try {
        _read();
    } catch(...) {
        _clean_up();
        throw;
    }
}

void intel_acbp_v1_t::hash_t::_read() {
    m_hash_algorithm_id = m__io->read_u2le();
    m_len_hash = m__io->read_u2le();
    m_hash = m__io->read_bytes(32);
}

intel_acbp_v1_t::hash_t::~hash_t() {
    _clean_up();
}

void intel_acbp_v1_t::hash_t::_clean_up() {
}

intel_acbp_v1_t::pmda_entry_v2_t::pmda_entry_v2_t(kaitai::kstream* p__io, intel_acbp_v1_t::pmda_body_t* p__parent, intel_acbp_v1_t* p__root) : kaitai::kstruct(p__io) {
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

void intel_acbp_v1_t::pmda_entry_v2_t::_read() {
    m_base = m__io->read_u4le();
    m_size = m__io->read_u4le();
    m_hash = new hash_t(m__io, this, m__root);
}

intel_acbp_v1_t::pmda_entry_v2_t::~pmda_entry_v2_t() {
    _clean_up();
}

void intel_acbp_v1_t::pmda_entry_v2_t::_clean_up() {
    if (m_hash) {
        delete m_hash; m_hash = 0;
    }
}

intel_acbp_v1_t::ibbs_body_t::ibbs_body_t(kaitai::kstream* p__io, intel_acbp_v1_t::acbp_element_t* p__parent, intel_acbp_v1_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;
    m_reserved = 0;
    m_post_ibb_hash = 0;
    m_ibb_hash = 0;
    m_ibb_segments = 0;

    try {
        _read();
    } catch(...) {
        _clean_up();
        throw;
    }
}

void intel_acbp_v1_t::ibbs_body_t::_read() {
    m_reserved = new std::vector<uint8_t>();
    const int l_reserved = 3;
    for (int i = 0; i < l_reserved; i++) {
        m_reserved->push_back(m__io->read_u1());
    }
    m_flags = m__io->read_u4le();
    m_mch_bar = m__io->read_u8le();
    m_vtd_bar = m__io->read_u8le();
    m_dma_protection_base0 = m__io->read_u4le();
    m_dma_protection_limit0 = m__io->read_u4le();
    m_dma_protection_base1 = m__io->read_u8le();
    m_dma_protection_limit1 = m__io->read_u8le();
    m_post_ibb_hash = new hash_t(m__io, this, m__root);
    m_ibb_entry_point = m__io->read_u4le();
    m_ibb_hash = new hash_t(m__io, this, m__root);
    m_num_ibb_segments = m__io->read_u1();
    m_ibb_segments = new std::vector<ibb_segment_t*>();
    const int l_ibb_segments = num_ibb_segments();
    for (int i = 0; i < l_ibb_segments; i++) {
        m_ibb_segments->push_back(new ibb_segment_t(m__io, this, m__root));
    }
}

intel_acbp_v1_t::ibbs_body_t::~ibbs_body_t() {
    _clean_up();
}

void intel_acbp_v1_t::ibbs_body_t::_clean_up() {
    if (m_reserved) {
        delete m_reserved; m_reserved = 0;
    }
    if (m_post_ibb_hash) {
        delete m_post_ibb_hash; m_post_ibb_hash = 0;
    }
    if (m_ibb_hash) {
        delete m_ibb_hash; m_ibb_hash = 0;
    }
    if (m_ibb_segments) {
        for (std::vector<ibb_segment_t*>::iterator it = m_ibb_segments->begin(); it != m_ibb_segments->end(); ++it) {
            delete *it;
        }
        delete m_ibb_segments; m_ibb_segments = 0;
    }
}

intel_acbp_v1_t::pmda_body_t::pmda_body_t(kaitai::kstream* p__io, intel_acbp_v1_t::acbp_element_t* p__parent, intel_acbp_v1_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;
    m_entries_v1 = 0;
    m_entries_v2 = 0;

    try {
        _read();
    } catch(...) {
        _clean_up();
        throw;
    }
}

void intel_acbp_v1_t::pmda_body_t::_read() {
    m_total_size = m__io->read_u2le();
    m_version = m__io->read_u4le();
    m_num_entries = m__io->read_u4le();
    n_entries_v1 = true;
    if (version() == 1) {
        n_entries_v1 = false;
        m_entries_v1 = new std::vector<pmda_entry_v1_t*>();
        const int l_entries_v1 = num_entries();
        for (int i = 0; i < l_entries_v1; i++) {
            m_entries_v1->push_back(new pmda_entry_v1_t(m__io, this, m__root));
        }
    }
    n_entries_v2 = true;
    if (version() == 2) {
        n_entries_v2 = false;
        m_entries_v2 = new std::vector<pmda_entry_v2_t*>();
        const int l_entries_v2 = num_entries();
        for (int i = 0; i < l_entries_v2; i++) {
            m_entries_v2->push_back(new pmda_entry_v2_t(m__io, this, m__root));
        }
    }
}

intel_acbp_v1_t::pmda_body_t::~pmda_body_t() {
    _clean_up();
}

void intel_acbp_v1_t::pmda_body_t::_clean_up() {
    if (!n_entries_v1) {
        if (m_entries_v1) {
            for (std::vector<pmda_entry_v1_t*>::iterator it = m_entries_v1->begin(); it != m_entries_v1->end(); ++it) {
                delete *it;
            }
            delete m_entries_v1; m_entries_v1 = 0;
        }
    }
    if (!n_entries_v2) {
        if (m_entries_v2) {
            for (std::vector<pmda_entry_v2_t*>::iterator it = m_entries_v2->begin(); it != m_entries_v2->end(); ++it) {
                delete *it;
            }
            delete m_entries_v2; m_entries_v2 = 0;
        }
    }
}

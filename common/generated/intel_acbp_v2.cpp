// This is a generated file! Please edit source .ksy file and use kaitai-struct-compiler to rebuild

#include "intel_acbp_v2.h"
#include "../kaitai/exceptions.h"

intel_acbp_v2_t::intel_acbp_v2_t(kaitai::kstream* p__io, kaitai::kstruct* p__parent, intel_acbp_v2_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = this; (void)p__root;
    m_elements = nullptr;
    m_key_signature = nullptr;
    _read();
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
    m_elements = std::unique_ptr<std::vector<std::unique_ptr<acbp_element_t>>>(new std::vector<std::unique_ptr<acbp_element_t>>());
    {
        int i = 0;
        acbp_element_t* _;
        do {
            _ = new acbp_element_t(m__io, this, m__root);
            m_elements->push_back(std::move(std::unique_ptr<acbp_element_t>(_)));
            i++;
        } while (!( ((_->header()->total_size() == 0) || (_->header()->structure_id() == intel_acbp_v2_t::STRUCTURE_IDS_PMSG)) ));
    }
    m_key_signature = std::unique_ptr<key_signature_t>(new key_signature_t(m__io, this, m__root));
}

intel_acbp_v2_t::~intel_acbp_v2_t() {
    _clean_up();
}

void intel_acbp_v2_t::_clean_up() {
}

intel_acbp_v2_t::acbp_element_t::acbp_element_t(kaitai::kstream* p__io, intel_acbp_v2_t* p__parent, intel_acbp_v2_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;
    m_header = nullptr;
    m_ibbs_body = nullptr;
    m_pmda_body = nullptr;
    _read();
}

void intel_acbp_v2_t::acbp_element_t::_read() {
    m_header = std::unique_ptr<header_t>(new header_t(m__io, this, m__root));
    n_ibbs_body = true;
    if ( ((header()->structure_id() == intel_acbp_v2_t::STRUCTURE_IDS_IBBS) && (header()->total_size() >= 12)) ) {
        n_ibbs_body = false;
        m_ibbs_body = std::unique_ptr<ibbs_body_t>(new ibbs_body_t(m__io, this, m__root));
    }
    n_pmda_body = true;
    if ( ((header()->structure_id() == intel_acbp_v2_t::STRUCTURE_IDS_PMDA) && (header()->total_size() >= 12)) ) {
        n_pmda_body = false;
        m_pmda_body = std::unique_ptr<pmda_body_t>(new pmda_body_t(m__io, this, m__root));
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
    if (!n_ibbs_body) {
    }
    if (!n_pmda_body) {
    }
    if (!n_generic_body) {
    }
}

intel_acbp_v2_t::key_signature_t::key_signature_t(kaitai::kstream* p__io, intel_acbp_v2_t* p__parent, intel_acbp_v2_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;
    m_public_key = nullptr;
    m_signature = nullptr;
    _read();
}

void intel_acbp_v2_t::key_signature_t::_read() {
    m_version = m__io->read_u1();
    m_key_id = m__io->read_u2le();
    m_public_key = std::unique_ptr<public_key_t>(new public_key_t(m__io, this, m__root));
    m_sig_scheme = m__io->read_u2le();
    m_signature = std::unique_ptr<signature_t>(new signature_t(m__io, this, m__root));
}

intel_acbp_v2_t::key_signature_t::~key_signature_t() {
    _clean_up();
}

void intel_acbp_v2_t::key_signature_t::_clean_up() {
}

intel_acbp_v2_t::signature_t::signature_t(kaitai::kstream* p__io, intel_acbp_v2_t::key_signature_t* p__parent, intel_acbp_v2_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;
    _read();
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
    _read();
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
    _read();
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
    _read();
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
    _read();
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
    m_hash = nullptr;
    _read();
}

void intel_acbp_v2_t::pmda_entry_v3_t::_read() {
    m_entry_id = m__io->read_u4le();
    m_base = m__io->read_u4le();
    m_size = m__io->read_u4le();
    m_total_entry_size = m__io->read_u2le();
    m_version = m__io->read_u2le();
    m_hash = std::unique_ptr<hash_t>(new hash_t(m__io, this, m__root));
}

intel_acbp_v2_t::pmda_entry_v3_t::~pmda_entry_v3_t() {
    _clean_up();
}

void intel_acbp_v2_t::pmda_entry_v3_t::_clean_up() {
}

intel_acbp_v2_t::ibbs_body_t::ibbs_body_t(kaitai::kstream* p__io, intel_acbp_v2_t::acbp_element_t* p__parent, intel_acbp_v2_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;
    m_post_ibb_digest = nullptr;
    m_ibb_digests = nullptr;
    m_obb_digest = nullptr;
    m_reserved2 = nullptr;
    m_ibb_segments = nullptr;
    _read();
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
    m_post_ibb_digest = std::unique_ptr<hash_t>(new hash_t(m__io, this, m__root));
    m_ibb_entry_point = m__io->read_u4le();
    m_ibb_digests_size = m__io->read_u2le();
    m_num_ibb_digests = m__io->read_u2le();
    m_ibb_digests = std::unique_ptr<std::vector<std::unique_ptr<hash_t>>>(new std::vector<std::unique_ptr<hash_t>>());
    const int l_ibb_digests = num_ibb_digests();
    for (int i = 0; i < l_ibb_digests; i++) {
        m_ibb_digests->push_back(std::move(std::unique_ptr<hash_t>(new hash_t(m__io, this, m__root))));
    }
    m_obb_digest = std::unique_ptr<hash_t>(new hash_t(m__io, this, m__root));
    m_reserved2 = std::unique_ptr<std::vector<uint8_t>>(new std::vector<uint8_t>());
    const int l_reserved2 = 3;
    for (int i = 0; i < l_reserved2; i++) {
        m_reserved2->push_back(std::move(m__io->read_u1()));
    }
    m_num_ibb_segments = m__io->read_u1();
    m_ibb_segments = std::unique_ptr<std::vector<std::unique_ptr<ibb_segment_t>>>(new std::vector<std::unique_ptr<ibb_segment_t>>());
    const int l_ibb_segments = num_ibb_segments();
    for (int i = 0; i < l_ibb_segments; i++) {
        m_ibb_segments->push_back(std::move(std::unique_ptr<ibb_segment_t>(new ibb_segment_t(m__io, this, m__root))));
    }
}

intel_acbp_v2_t::ibbs_body_t::~ibbs_body_t() {
    _clean_up();
}

void intel_acbp_v2_t::ibbs_body_t::_clean_up() {
}

intel_acbp_v2_t::pmda_body_t::pmda_body_t(kaitai::kstream* p__io, intel_acbp_v2_t::acbp_element_t* p__parent, intel_acbp_v2_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;
    m_entries = nullptr;
    _read();
}

void intel_acbp_v2_t::pmda_body_t::_read() {
    m_reserved = m__io->read_u2le();
    m_total_size = m__io->read_u2le();
    m_version = m__io->read_u4le();
    if (!(version() == 3)) {
        throw kaitai::validation_not_equal_error<uint32_t>(3, version(), _io(), std::string("/types/pmda_body/seq/2"));
    }
    m_num_entries = m__io->read_u4le();
    m_entries = std::unique_ptr<std::vector<std::unique_ptr<pmda_entry_v3_t>>>(new std::vector<std::unique_ptr<pmda_entry_v3_t>>());
    const int l_entries = num_entries();
    for (int i = 0; i < l_entries; i++) {
        m_entries->push_back(std::move(std::unique_ptr<pmda_entry_v3_t>(new pmda_entry_v3_t(m__io, this, m__root))));
    }
}

intel_acbp_v2_t::pmda_body_t::~pmda_body_t() {
    _clean_up();
}

void intel_acbp_v2_t::pmda_body_t::_clean_up() {
}

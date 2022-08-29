#ifndef INTEL_ACBP_V1_H_
#define INTEL_ACBP_V1_H_

// This is a generated file! Please edit source .ksy file and use kaitai-struct-compiler to rebuild

#include "../kaitai/kaitaistruct.h"
#include <stdint.h>
#include <vector>

#if KAITAI_STRUCT_VERSION < 9000L
#error "Incompatible Kaitai Struct C++/STL API: version 0.9 or later is required"
#endif

class intel_acbp_v1_t : public kaitai::kstruct {

public:
    class pmsg_body_t;
    class acbp_element_t;
    class common_header_t;
    class signature_t;
    class pmda_entry_v1_t;
    class ibb_segment_t;
    class public_key_t;
    class hash_t;
    class pmda_entry_v2_t;
    class ibbs_body_t;
    class pmda_body_t;

    enum ibb_segment_type_t {
        IBB_SEGMENT_TYPE_IBB = 0,
        IBB_SEGMENT_TYPE_NON_IBB = 1
    };

    enum structure_ids_t {
        STRUCTURE_IDS_PMDA = 6872283318001360735LL,
        STRUCTURE_IDS_PMSG = 6872289979495636831LL,
        STRUCTURE_IDS_ACBP = 6872299801917087583LL,
        STRUCTURE_IDS_IBBS = 6872303100435717983LL
    };

    intel_acbp_v1_t(kaitai::kstream* p__io, kaitai::kstruct* p__parent = 0, intel_acbp_v1_t* p__root = 0);

private:
    void _read();
    void _clean_up();

public:
    ~intel_acbp_v1_t();

    class pmsg_body_t : public kaitai::kstruct {

    public:

        pmsg_body_t(kaitai::kstream* p__io, intel_acbp_v1_t::acbp_element_t* p__parent = 0, intel_acbp_v1_t* p__root = 0);

    private:
        void _read();
        void _clean_up();

    public:
        ~pmsg_body_t();

    private:
        uint8_t m_version;
        uint16_t m_key_id;
        public_key_t* m_public_key;
        uint16_t m_sig_scheme;
        signature_t* m_signature;
        intel_acbp_v1_t* m__root;
        intel_acbp_v1_t::acbp_element_t* m__parent;

    public:
        uint8_t version() const { return m_version; }
        uint16_t key_id() const { return m_key_id; }
        public_key_t* public_key() const { return m_public_key; }
        uint16_t sig_scheme() const { return m_sig_scheme; }
        signature_t* signature() const { return m_signature; }
        intel_acbp_v1_t* _root() const { return m__root; }
        intel_acbp_v1_t::acbp_element_t* _parent() const { return m__parent; }
    };

    class acbp_element_t : public kaitai::kstruct {

    public:

        acbp_element_t(kaitai::kstream* p__io, intel_acbp_v1_t* p__parent = 0, intel_acbp_v1_t* p__root = 0);

    private:
        void _read();
        void _clean_up();

    public:
        ~acbp_element_t();

    private:
        common_header_t* m_header;
        ibbs_body_t* m_ibbs_body;
        bool n_ibbs_body;

    public:
        bool _is_null_ibbs_body() { ibbs_body(); return n_ibbs_body; };

    private:
        pmda_body_t* m_pmda_body;
        bool n_pmda_body;

    public:
        bool _is_null_pmda_body() { pmda_body(); return n_pmda_body; };

    private:
        pmsg_body_t* m_pmsg_body;
        bool n_pmsg_body;

    public:
        bool _is_null_pmsg_body() { pmsg_body(); return n_pmsg_body; };

    private:
        std::string m_invalid_body;
        bool n_invalid_body;

    public:
        bool _is_null_invalid_body() { invalid_body(); return n_invalid_body; };

    private:
        intel_acbp_v1_t* m__root;
        intel_acbp_v1_t* m__parent;

    public:
        common_header_t* header() const { return m_header; }
        ibbs_body_t* ibbs_body() const { return m_ibbs_body; }
        pmda_body_t* pmda_body() const { return m_pmda_body; }
        pmsg_body_t* pmsg_body() const { return m_pmsg_body; }
        std::string invalid_body() const { return m_invalid_body; }
        intel_acbp_v1_t* _root() const { return m__root; }
        intel_acbp_v1_t* _parent() const { return m__parent; }
    };

    class common_header_t : public kaitai::kstruct {

    public:

        common_header_t(kaitai::kstream* p__io, intel_acbp_v1_t::acbp_element_t* p__parent = 0, intel_acbp_v1_t* p__root = 0);

    private:
        void _read();
        void _clean_up();

    public:
        ~common_header_t();

    private:
        structure_ids_t m_structure_id;
        uint8_t m_version;
        intel_acbp_v1_t* m__root;
        intel_acbp_v1_t::acbp_element_t* m__parent;

    public:
        structure_ids_t structure_id() const { return m_structure_id; }
        uint8_t version() const { return m_version; }
        intel_acbp_v1_t* _root() const { return m__root; }
        intel_acbp_v1_t::acbp_element_t* _parent() const { return m__parent; }
    };

    class signature_t : public kaitai::kstruct {

    public:

        signature_t(kaitai::kstream* p__io, intel_acbp_v1_t::pmsg_body_t* p__parent = 0, intel_acbp_v1_t* p__root = 0);

    private:
        void _read();
        void _clean_up();

    public:
        ~signature_t();

    private:
        uint8_t m_version;
        uint16_t m_size_bits;
        uint16_t m_hash_algorithm_id;
        std::string m_signature;
        intel_acbp_v1_t* m__root;
        intel_acbp_v1_t::pmsg_body_t* m__parent;

    public:
        uint8_t version() const { return m_version; }
        uint16_t size_bits() const { return m_size_bits; }
        uint16_t hash_algorithm_id() const { return m_hash_algorithm_id; }
        std::string signature() const { return m_signature; }
        intel_acbp_v1_t* _root() const { return m__root; }
        intel_acbp_v1_t::pmsg_body_t* _parent() const { return m__parent; }
    };

    class pmda_entry_v1_t : public kaitai::kstruct {

    public:

        pmda_entry_v1_t(kaitai::kstream* p__io, intel_acbp_v1_t::pmda_body_t* p__parent = 0, intel_acbp_v1_t* p__root = 0);

    private:
        void _read();
        void _clean_up();

    public:
        ~pmda_entry_v1_t();

    private:
        uint32_t m_base;
        uint32_t m_size;
        std::string m_hash;
        intel_acbp_v1_t* m__root;
        intel_acbp_v1_t::pmda_body_t* m__parent;

    public:
        uint32_t base() const { return m_base; }
        uint32_t size() const { return m_size; }
        std::string hash() const { return m_hash; }
        intel_acbp_v1_t* _root() const { return m__root; }
        intel_acbp_v1_t::pmda_body_t* _parent() const { return m__parent; }
    };

    class ibb_segment_t : public kaitai::kstruct {

    public:

        ibb_segment_t(kaitai::kstream* p__io, intel_acbp_v1_t::ibbs_body_t* p__parent = 0, intel_acbp_v1_t* p__root = 0);

    private:
        void _read();
        void _clean_up();

    public:
        ~ibb_segment_t();

    private:
        uint16_t m_reserved;
        uint16_t m_flags;
        uint32_t m_base;
        uint32_t m_size;
        intel_acbp_v1_t* m__root;
        intel_acbp_v1_t::ibbs_body_t* m__parent;

    public:
        uint16_t reserved() const { return m_reserved; }
        uint16_t flags() const { return m_flags; }
        uint32_t base() const { return m_base; }
        uint32_t size() const { return m_size; }
        intel_acbp_v1_t* _root() const { return m__root; }
        intel_acbp_v1_t::ibbs_body_t* _parent() const { return m__parent; }
    };

    class public_key_t : public kaitai::kstruct {

    public:

        public_key_t(kaitai::kstream* p__io, intel_acbp_v1_t::pmsg_body_t* p__parent = 0, intel_acbp_v1_t* p__root = 0);

    private:
        void _read();
        void _clean_up();

    public:
        ~public_key_t();

    private:
        uint8_t m_version;
        uint16_t m_size_bits;
        uint32_t m_exponent;
        std::string m_modulus;
        intel_acbp_v1_t* m__root;
        intel_acbp_v1_t::pmsg_body_t* m__parent;

    public:
        uint8_t version() const { return m_version; }
        uint16_t size_bits() const { return m_size_bits; }
        uint32_t exponent() const { return m_exponent; }
        std::string modulus() const { return m_modulus; }
        intel_acbp_v1_t* _root() const { return m__root; }
        intel_acbp_v1_t::pmsg_body_t* _parent() const { return m__parent; }
    };

    class hash_t : public kaitai::kstruct {

    public:

        hash_t(kaitai::kstream* p__io, kaitai::kstruct* p__parent = 0, intel_acbp_v1_t* p__root = 0);

    private:
        void _read();
        void _clean_up();

    public:
        ~hash_t();

    private:
        uint16_t m_hash_algorithm_id;
        uint16_t m_len_hash;
        std::string m_hash;
        intel_acbp_v1_t* m__root;
        kaitai::kstruct* m__parent;

    public:
        uint16_t hash_algorithm_id() const { return m_hash_algorithm_id; }
        uint16_t len_hash() const { return m_len_hash; }
        std::string hash() const { return m_hash; }
        intel_acbp_v1_t* _root() const { return m__root; }
        kaitai::kstruct* _parent() const { return m__parent; }
    };

    class pmda_entry_v2_t : public kaitai::kstruct {

    public:

        pmda_entry_v2_t(kaitai::kstream* p__io, intel_acbp_v1_t::pmda_body_t* p__parent = 0, intel_acbp_v1_t* p__root = 0);

    private:
        void _read();
        void _clean_up();

    public:
        ~pmda_entry_v2_t();

    private:
        uint32_t m_base;
        uint32_t m_size;
        hash_t* m_hash;
        intel_acbp_v1_t* m__root;
        intel_acbp_v1_t::pmda_body_t* m__parent;

    public:
        uint32_t base() const { return m_base; }
        uint32_t size() const { return m_size; }
        hash_t* hash() const { return m_hash; }
        intel_acbp_v1_t* _root() const { return m__root; }
        intel_acbp_v1_t::pmda_body_t* _parent() const { return m__parent; }
    };

    class ibbs_body_t : public kaitai::kstruct {

    public:

        ibbs_body_t(kaitai::kstream* p__io, intel_acbp_v1_t::acbp_element_t* p__parent = 0, intel_acbp_v1_t* p__root = 0);

    private:
        void _read();
        void _clean_up();

    public:
        ~ibbs_body_t();

    private:
        std::vector<uint8_t>* m_reserved;
        uint32_t m_flags;
        uint64_t m_mch_bar;
        uint64_t m_vtd_bar;
        uint32_t m_dma_protection_base0;
        uint32_t m_dma_protection_limit0;
        uint64_t m_dma_protection_base1;
        uint64_t m_dma_protection_limit1;
        hash_t* m_post_ibb_hash;
        uint32_t m_ibb_entry_point;
        hash_t* m_ibb_hash;
        uint8_t m_num_ibb_segments;
        std::vector<ibb_segment_t*>* m_ibb_segments;
        intel_acbp_v1_t* m__root;
        intel_acbp_v1_t::acbp_element_t* m__parent;

    public:
        std::vector<uint8_t>* reserved() const { return m_reserved; }
        uint32_t flags() const { return m_flags; }
        uint64_t mch_bar() const { return m_mch_bar; }
        uint64_t vtd_bar() const { return m_vtd_bar; }
        uint32_t dma_protection_base0() const { return m_dma_protection_base0; }
        uint32_t dma_protection_limit0() const { return m_dma_protection_limit0; }
        uint64_t dma_protection_base1() const { return m_dma_protection_base1; }
        uint64_t dma_protection_limit1() const { return m_dma_protection_limit1; }
        hash_t* post_ibb_hash() const { return m_post_ibb_hash; }
        uint32_t ibb_entry_point() const { return m_ibb_entry_point; }
        hash_t* ibb_hash() const { return m_ibb_hash; }
        uint8_t num_ibb_segments() const { return m_num_ibb_segments; }
        std::vector<ibb_segment_t*>* ibb_segments() const { return m_ibb_segments; }
        intel_acbp_v1_t* _root() const { return m__root; }
        intel_acbp_v1_t::acbp_element_t* _parent() const { return m__parent; }
    };

    class pmda_body_t : public kaitai::kstruct {

    public:

        pmda_body_t(kaitai::kstream* p__io, intel_acbp_v1_t::acbp_element_t* p__parent = 0, intel_acbp_v1_t* p__root = 0);

    private:
        void _read();
        void _clean_up();

    public:
        ~pmda_body_t();

    private:
        uint16_t m_total_size;
        uint32_t m_version;
        uint32_t m_num_entries;
        std::vector<pmda_entry_v1_t*>* m_entries_v1;
        bool n_entries_v1;

    public:
        bool _is_null_entries_v1() { entries_v1(); return n_entries_v1; };

    private:
        std::vector<pmda_entry_v2_t*>* m_entries_v2;
        bool n_entries_v2;

    public:
        bool _is_null_entries_v2() { entries_v2(); return n_entries_v2; };

    private:
        intel_acbp_v1_t* m__root;
        intel_acbp_v1_t::acbp_element_t* m__parent;

    public:
        uint16_t total_size() const { return m_total_size; }
        uint32_t version() const { return m_version; }
        uint32_t num_entries() const { return m_num_entries; }
        std::vector<pmda_entry_v1_t*>* entries_v1() const { return m_entries_v1; }
        std::vector<pmda_entry_v2_t*>* entries_v2() const { return m_entries_v2; }
        intel_acbp_v1_t* _root() const { return m__root; }
        intel_acbp_v1_t::acbp_element_t* _parent() const { return m__parent; }
    };

private:
    structure_ids_t m_structure_id;
    uint8_t m_version;
    uint8_t m_reserved0;
    uint8_t m_bpm_revision;
    uint8_t m_bp_svn;
    uint8_t m_acm_svn;
    uint8_t m_reserved1;
    uint16_t m_nem_data_size;
    std::vector<acbp_element_t*>* m_elements;
    intel_acbp_v1_t* m__root;
    kaitai::kstruct* m__parent;

public:
    structure_ids_t structure_id() const { return m_structure_id; }
    uint8_t version() const { return m_version; }
    uint8_t reserved0() const { return m_reserved0; }
    uint8_t bpm_revision() const { return m_bpm_revision; }
    uint8_t bp_svn() const { return m_bp_svn; }
    uint8_t acm_svn() const { return m_acm_svn; }
    uint8_t reserved1() const { return m_reserved1; }
    uint16_t nem_data_size() const { return m_nem_data_size; }
    std::vector<acbp_element_t*>* elements() const { return m_elements; }
    intel_acbp_v1_t* _root() const { return m__root; }
    kaitai::kstruct* _parent() const { return m__parent; }
};

#endif  // INTEL_ACBP_V1_H_

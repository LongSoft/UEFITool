#ifndef INTEL_ACM_H_
#define INTEL_ACM_H_

// This is a generated file! Please edit source .ksy file and use kaitai-struct-compiler to rebuild

#include "../kaitai/kaitaistruct.h"
#include <stdint.h>

#if KAITAI_STRUCT_VERSION < 9000L
#error "Incompatible Kaitai Struct C++/STL API: version 0.9 or later is required"
#endif

class intel_acm_t : public kaitai::kstruct {

public:
    class header_t;

    enum module_subtype_t {
        MODULE_SUBTYPE_TXT = 0,
        MODULE_SUBTYPE_STARTUP = 1,
        MODULE_SUBTYPE_BOOT_GUARD = 3
    };

    enum known_header_version_t {
        KNOWN_HEADER_VERSION_V0_0 = 0,
        KNOWN_HEADER_VERSION_V3_0 = 196608
    };

    intel_acm_t(kaitai::kstream* p__io, kaitai::kstruct* p__parent = 0, intel_acm_t* p__root = 0);

private:
    void _read();
    void _clean_up();

public:
    ~intel_acm_t();

    class header_t : public kaitai::kstruct {

    public:

        header_t(kaitai::kstream* p__io, intel_acm_t* p__parent = 0, intel_acm_t* p__root = 0);

    private:
        void _read();
        void _clean_up();

    public:
        ~header_t();

    private:
        uint16_t m_module_type;
        module_subtype_t m_module_subtype;
        uint32_t m_header_size;
        uint32_t m_header_version;
        uint16_t m_chipset_id;
        uint16_t m_flags;
        uint32_t m_module_vendor;
        uint8_t m_date_day;
        uint8_t m_date_month;
        uint16_t m_date_year;
        uint32_t m_module_size;
        uint16_t m_acm_svn;
        uint16_t m_se_svn;
        uint32_t m_code_control_flags;
        uint32_t m_error_entry_point;
        uint32_t m_gdt_max;
        uint32_t m_gdt_base;
        uint32_t m_segment_sel;
        uint32_t m_entry_point;
        std::string m_reserved;
        uint32_t m_key_size;
        uint32_t m_scratch_space_size;
        std::string m_rsa_public_key;
        uint32_t m_rsa_exponent;
        bool n_rsa_exponent;

    public:
        bool _is_null_rsa_exponent() { rsa_exponent(); return n_rsa_exponent; };

    private:
        std::string m_rsa_signature;
        std::string m_scratch_space;
        intel_acm_t* m__root;
        intel_acm_t* m__parent;

    public:
        uint16_t module_type() const { return m_module_type; }
        module_subtype_t module_subtype() const { return m_module_subtype; }

        /**
         * counted in 4 byte increments
         */
        uint32_t header_size() const { return m_header_size; }
        uint32_t header_version() const { return m_header_version; }
        uint16_t chipset_id() const { return m_chipset_id; }
        uint16_t flags() const { return m_flags; }
        uint32_t module_vendor() const { return m_module_vendor; }

        /**
         * BCD
         */
        uint8_t date_day() const { return m_date_day; }

        /**
         * BCD
         */
        uint8_t date_month() const { return m_date_month; }

        /**
         * BCD
         */
        uint16_t date_year() const { return m_date_year; }

        /**
         * counted in 4 byte increments
         */
        uint32_t module_size() const { return m_module_size; }
        uint16_t acm_svn() const { return m_acm_svn; }
        uint16_t se_svn() const { return m_se_svn; }
        uint32_t code_control_flags() const { return m_code_control_flags; }
        uint32_t error_entry_point() const { return m_error_entry_point; }
        uint32_t gdt_max() const { return m_gdt_max; }
        uint32_t gdt_base() const { return m_gdt_base; }
        uint32_t segment_sel() const { return m_segment_sel; }
        uint32_t entry_point() const { return m_entry_point; }
        std::string reserved() const { return m_reserved; }

        /**
         * counted in 4 byte increments
         */
        uint32_t key_size() const { return m_key_size; }

        /**
         * counted in 4 byte increments
         */
        uint32_t scratch_space_size() const { return m_scratch_space_size; }
        std::string rsa_public_key() const { return m_rsa_public_key; }
        uint32_t rsa_exponent() const { return m_rsa_exponent; }
        std::string rsa_signature() const { return m_rsa_signature; }
        std::string scratch_space() const { return m_scratch_space; }
        intel_acm_t* _root() const { return m__root; }
        intel_acm_t* _parent() const { return m__parent; }
    };

private:
    header_t* m_header;
    std::string m_body;
    intel_acm_t* m__root;
    kaitai::kstruct* m__parent;

public:
    header_t* header() const { return m_header; }
    std::string body() const { return m_body; }
    intel_acm_t* _root() const { return m__root; }
    kaitai::kstruct* _parent() const { return m__parent; }
};

#endif  // INTEL_ACM_H_

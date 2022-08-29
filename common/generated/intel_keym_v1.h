#ifndef INTEL_KEYM_V1_H_
#define INTEL_KEYM_V1_H_

// This is a generated file! Please edit source .ksy file and use kaitai-struct-compiler to rebuild

#include "../kaitai/kaitaistruct.h"
#include <stdint.h>

#if KAITAI_STRUCT_VERSION < 9000L
#error "Incompatible Kaitai Struct C++/STL API: version 0.9 or later is required"
#endif

class intel_keym_v1_t : public kaitai::kstruct {

public:
    class km_hash_t;
    class public_key_t;
    class signature_t;
    class key_signature_t;

    enum structure_ids_t {
        STRUCTURE_IDS_KEYM = 6872296602200661855LL
    };

    intel_keym_v1_t(kaitai::kstream* p__io, kaitai::kstruct* p__parent = 0, intel_keym_v1_t* p__root = 0);

private:
    void _read();
    void _clean_up();

public:
    ~intel_keym_v1_t();

    class km_hash_t : public kaitai::kstruct {

    public:

        km_hash_t(kaitai::kstream* p__io, intel_keym_v1_t* p__parent = 0, intel_keym_v1_t* p__root = 0);

    private:
        void _read();
        void _clean_up();

    public:
        ~km_hash_t();

    private:
        uint16_t m_hash_algorithm_id;
        uint16_t m_len_hash;
        std::string m_hash;
        intel_keym_v1_t* m__root;
        intel_keym_v1_t* m__parent;

    public:
        uint16_t hash_algorithm_id() const { return m_hash_algorithm_id; }
        uint16_t len_hash() const { return m_len_hash; }
        std::string hash() const { return m_hash; }
        intel_keym_v1_t* _root() const { return m__root; }
        intel_keym_v1_t* _parent() const { return m__parent; }
    };

    class public_key_t : public kaitai::kstruct {

    public:

        public_key_t(kaitai::kstream* p__io, intel_keym_v1_t::key_signature_t* p__parent = 0, intel_keym_v1_t* p__root = 0);

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
        intel_keym_v1_t* m__root;
        intel_keym_v1_t::key_signature_t* m__parent;

    public:
        uint8_t version() const { return m_version; }
        uint16_t size_bits() const { return m_size_bits; }
        uint32_t exponent() const { return m_exponent; }
        std::string modulus() const { return m_modulus; }
        intel_keym_v1_t* _root() const { return m__root; }
        intel_keym_v1_t::key_signature_t* _parent() const { return m__parent; }
    };

    class signature_t : public kaitai::kstruct {

    public:

        signature_t(kaitai::kstream* p__io, intel_keym_v1_t::key_signature_t* p__parent = 0, intel_keym_v1_t* p__root = 0);

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
        intel_keym_v1_t* m__root;
        intel_keym_v1_t::key_signature_t* m__parent;

    public:
        uint8_t version() const { return m_version; }
        uint16_t size_bits() const { return m_size_bits; }
        uint16_t hash_algorithm_id() const { return m_hash_algorithm_id; }
        std::string signature() const { return m_signature; }
        intel_keym_v1_t* _root() const { return m__root; }
        intel_keym_v1_t::key_signature_t* _parent() const { return m__parent; }
    };

    class key_signature_t : public kaitai::kstruct {

    public:

        key_signature_t(kaitai::kstream* p__io, intel_keym_v1_t* p__parent = 0, intel_keym_v1_t* p__root = 0);

    private:
        void _read();
        void _clean_up();

    public:
        ~key_signature_t();

    private:
        uint8_t m_version;
        uint16_t m_key_id;
        public_key_t* m_public_key;
        uint16_t m_sig_scheme;
        signature_t* m_signature;
        intel_keym_v1_t* m__root;
        intel_keym_v1_t* m__parent;

    public:
        uint8_t version() const { return m_version; }
        uint16_t key_id() const { return m_key_id; }
        public_key_t* public_key() const { return m_public_key; }
        uint16_t sig_scheme() const { return m_sig_scheme; }
        signature_t* signature() const { return m_signature; }
        intel_keym_v1_t* _root() const { return m__root; }
        intel_keym_v1_t* _parent() const { return m__parent; }
    };

private:
    structure_ids_t m_structure_id;
    uint8_t m_version;
    uint8_t m_km_version;
    uint8_t m_km_svn;
    uint8_t m_km_id;
    km_hash_t* m_km_hash;
    key_signature_t* m_key_signature;
    intel_keym_v1_t* m__root;
    kaitai::kstruct* m__parent;

public:
    structure_ids_t structure_id() const { return m_structure_id; }
    uint8_t version() const { return m_version; }
    uint8_t km_version() const { return m_km_version; }
    uint8_t km_svn() const { return m_km_svn; }
    uint8_t km_id() const { return m_km_id; }
    km_hash_t* km_hash() const { return m_km_hash; }
    key_signature_t* key_signature() const { return m_key_signature; }
    intel_keym_v1_t* _root() const { return m__root; }
    kaitai::kstruct* _parent() const { return m__parent; }
};

#endif  // INTEL_KEYM_V1_H_

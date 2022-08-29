meta:
  id: intel_keym_v2
  title: Intel BootGuard Key Manifest v2
  application: Intel x86 firmware
  file-extension: keym_v2
  tags:
    - firmware
  license: CC0-1.0
  ks-version: 0.9
  endian: le
  
enums:
  structure_ids:
    0x5f5f4d59454b5f5f: keym

  km_usage_flags:
    1: boot_policy_manifest
    2: fit_patch_manifest
    4: acm_manifest
    8: sdev
  
seq:
- id: header
  type: header
- id: key_signature_offset
  type: u2
- id: reserved
  type: u1
  repeat: expr
  repeat-expr: 3
- id: km_version
  type: u1
- id: km_svn
  type: u1
- id: km_id
  type: u1
- id: fpf_hash_algorithm_id
  type: u2
- id: num_km_hashes
  type: u2
- id: km_hashes
  type: km_hash
  repeat: expr
  repeat-expr: num_km_hashes
- id: key_signature   
  type: key_signature
    
types:
  header:
    seq:
    - id: structure_id
      type: u8
      enum: structure_ids
      valid: structure_ids::keym
    - id: version
      type: u1
      valid:
        expr: _ >= 0x20
    - id: header_specific
      type: u1
    - id: total_size
      type: u2
      valid: 0x0    
      
  km_hash:
    seq:
    - id: usage_flags
      type: u8
    - id: hash_algorithm_id
      type: u2
    - id: len_hash
      type: u2
    - id: hash
      size: len_hash
  
  public_key:
    seq:
    - id: version
      type: u1
    - id: size_bits
      type: u2
    - id: exponent
      type: u4
    - id: modulus
      size: size_bits / 8
  
  signature:
    seq:
    - id: version
      type: u1
    - id: size_bits
      type: u2
    - id: hash_algorithm_id
      type: u2
    - id: signature
      size: size_bits / 8
  
  key_signature:
    seq:
    - id: version
      type: u1
    - id: key_id
      type: u2
    - id: public_key
      type: public_key
    - id: sig_scheme
      type: u2
    - id: signature
      type: signature
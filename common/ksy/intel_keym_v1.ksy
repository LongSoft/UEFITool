meta:
  id: intel_keym_v1
  title: Intel BootGuard Key Manifest v1
  application: Intel x86 firmware
  file-extension: keym_v1
  tags:
    - firmware
  license: CC0-1.0
  ks-version: 0.9
  endian: le
  
enums:
  structure_ids:
    0x5f5f4d59454b5f5f: keym
    
seq:
- id: structure_id
  type: u8
  enum: structure_ids
  valid: structure_ids::keym
- id: version
  type: u1
  valid:
    expr: _ < 0x20
- id: km_version
  type: u1
- id: km_svn
  type: u1
- id: km_id
  type: u1
- id: km_hash
  type: km_hash
- id: key_signature
  type: key_signature
  
types:
  km_hash:
    seq:
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
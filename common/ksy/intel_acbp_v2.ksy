meta:
  id: intel_acbp_v2
  title: Intel BootGuard Boot Policy v2
  application: Intel x86 firmware
  file-extension: acbp_v2
  tags:
    - firmware
  license: CC0-1.0
  ks-version: 0.9
  endian: le
  
enums:
  ibb_segment_type:
    0: ibb
    1: non_ibb
  
  structure_ids:
    0x5f5f504243415f5f: acbp
    0x5f5f534242495f5f: ibbs
    0x5f5f535458545f5f: txts
    0x5f5f535246505f5f: pfrs
    0x5f5f534443505f5f: pcds
    0x5f5f41444d505f5f: pmda
    0x5f5f47534d505f5f: pmsg
  
seq:
- id: structure_id
  type: u8
  enum: structure_ids
  valid: structure_ids::acbp
- id: version
  type: u1
  valid:
    expr: _ >= 0x20
- id: header_specific
  type: u1
- id: total_size
  type: u2
  valid: 0x14
- id: key_signature_offset
  type: u2
- id: bpm_revision
  type: u1
- id: bp_svn
  type: u1
- id: acm_svn
  type: u1
- id: reserved
  type: u1
- id: nem_data_size
  type: u2
- id: elements
  type: acbp_element
  repeat: until
  repeat-until: _.header.total_size == 0 or _.header.structure_id == structure_ids::pmsg
- id: key_signature
  type: key_signature
    
types:
  header:
    seq:
    - id: structure_id
      type: u8
      enum: structure_ids
    - id: version
      type: u1
    - id: header_specific
      type: u1
    - id: total_size
      type: u2
        
  hash:
    seq:
    - id: hash_algorithm_id
      type: u2
    - id: len_hash
      type: u2
    - id: hash
      size: len_hash
  
  pmda_entry_v3:
    seq:
    - id: entry_id
      type: u4
    - id: base
      type: u4
    - id: size
      type: u4
    - id: total_entry_size
      type: u2
    - id: version
      type: u2
    - id: hash
      type: hash
  
  pmda_body:
    seq:
    - id: reserved
      type: u2
    - id: total_size
      type: u2
    - id: version
      type: u4
      valid: 3
    - id: num_entries
      type: u4
    - id: entries
      type: pmda_entry_v3
      repeat: expr
      repeat-expr: num_entries
  
  ibb_segment:
    seq:
    - id: reserved
      type: u2
    - id: flags
      type: u2
    - id: base
      type: u4
    - id: size
      type: u4
  
  ibbs_body:
    seq:
    - id: reserved0
      type: u1
    - id: set_number
      type: u1
    - id: reserved1
      type: u1
    - id: pbet_value
      type: u1
    - id: flags
      type: u4
    - id: mch_bar
      type: u8
    - id: vtd_bar
      type: u8
    - id: dma_protection_base0
      type: u4
    - id: dma_protection_limit0
      type: u4
    - id: dma_protection_base1
      type: u8
    - id: dma_protection_limit1
      type: u8
    - id: post_ibb_digest
      type: hash
    - id: ibb_entry_point
      type: u4
    - id: ibb_digests_size
      type: u2
    - id: num_ibb_digests
      type: u2
    - id: ibb_digests
      type: hash
      repeat: expr
      repeat-expr: num_ibb_digests
    - id: obb_digest
      type: hash
    - id: reserved2
      type: u1
      repeat: expr
      repeat-expr: 3
    - id: num_ibb_segments
      type: u1
    - id: ibb_segments
      type: ibb_segment
      repeat: expr
      repeat-expr: num_ibb_segments
  
  acbp_element:
    seq:
    - id: header
      type: header
    - id: ibbs_body
      type: ibbs_body
      if:     header.structure_id == structure_ids::ibbs 
          and header.total_size >= sizeof<header> 
    - id: pmda_body
      type: pmda_body
      if:     header.structure_id == structure_ids::pmda 
          and header.total_size >= sizeof<header> 
    - id: generic_body
      size: header.total_size - sizeof<header>
      if:     header.structure_id != structure_ids::ibbs 
          and header.structure_id != structure_ids::pmda 
          and header.total_size >= sizeof<header>

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
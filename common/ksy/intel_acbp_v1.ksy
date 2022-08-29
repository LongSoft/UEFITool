meta:
  id: intel_acbp_v1
  title: Intel BootGuard Boot Policy v1
  application: Intel x86 firmware
  file-extension: acbp_v1
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
    expr: _ < 0x20
- id: reserved0
  type: u1
- id: bpm_revision
  type: u1
- id: bp_svn
  type: u1
- id: acm_svn
  type: u1
- id: reserved1
  type: u1
- id: nem_data_size
  type: u2
- id: elements
  type: acbp_element
  repeat: until
  repeat-until: _.header.structure_id == structure_ids::pmsg or _io.eof 
  
types:
  acbp_element:
    seq:
    - id: header
      type: common_header
    - id: ibbs_body
      type: ibbs_body
      if: header.structure_id == structure_ids::ibbs
    - id: pmda_body
      type: pmda_body
      if: header.structure_id == structure_ids::pmda
    - id: pmsg_body
      type: pmsg_body
      if: header.structure_id == structure_ids::pmsg
    - id: invalid_body
      size: 0
      if:     header.structure_id != structure_ids::pmsg 
          and header.structure_id != structure_ids::pmda 
          and header.structure_id != structure_ids::ibbs
      valid:
        expr: false
      
  common_header:
    seq:
    - id: structure_id
      type: u8
      enum: structure_ids
    - id: version
      type: u1

  hash:
    seq:
    - id: hash_algorithm_id
      type: u2
    - id: len_hash
      type: u2
    - id: hash
      size: 32
 
  ibbs_body:
    seq:
    - id: reserved
      type: u1
      repeat: expr
      repeat-expr: 3
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
    - id: post_ibb_hash
      type: hash
    - id: ibb_entry_point
      type: u4
    - id: ibb_hash
      type: hash
    - id: num_ibb_segments
      type: u1
    - id: ibb_segments
      type: ibb_segment
      repeat: expr
      repeat-expr: num_ibb_segments
  
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
  
  pmda_body:
    seq:
    - id: total_size
      type: u2
    - id: version
      type: u4
    - id: num_entries
      type: u4
    - id: entries_v1
      if: version == 1
      type: pmda_entry_v1
      repeat: expr
      repeat-expr: num_entries
    - id: entries_v2
      if: version == 2
      type: pmda_entry_v2
      repeat: expr
      repeat-expr: num_entries
  
  pmda_entry_v1:
    seq:
    - id: base
      type: u4
    - id: size
      type: u4
    - id: hash
      size: 32
  
  pmda_entry_v2:
    seq:
    - id: base
      type: u4
    - id: size
      type: u4
    - id: hash
      type: hash
     
  pmsg_body:
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
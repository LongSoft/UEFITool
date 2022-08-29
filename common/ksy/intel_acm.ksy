meta:
  id: intel_acm
  title: Intel Authenticated Code Module
  application: Intel x86 firmware
  file-extension: acm
  tags:
    - executable
    - firmware
  license: CC0-1.0
  ks-version: 0.9
  endian: le
  
enums:
  module_subtype:
    0: txt
    1: startup
    3: boot_guard
  
  known_header_version:
    0x00000000: v0_0
    0x00030000: v3_0
    
seq:
- id: header
  type: header
- id: body
  size: 4 * (header.module_size - header.header_size - header.scratch_space_size)
  
types:
  header:
    seq:
    - id: module_type
      type: u2
      valid: 0x0002
    - id: module_subtype
      type: u2
      enum: module_subtype
    - id: header_size
      type: u4
      doc: counted in 4 byte increments
    - id: header_version
      type: u4
    - id: chipset_id
      type: u2
    - id: flags
      type: u2
    - id: module_vendor
      type: u4
      valid: 0x8086
    - id: date_day
      type: u1
      doc: BCD
    - id: date_month
      type: u1
      doc: BCD
    - id: date_year
      type: u2
      doc: BCD
    - id: module_size
      type: u4
      doc: counted in 4 byte increments
    - id: acm_svn
      type: u2
    - id: se_svn
      type: u2
    - id: code_control_flags
      type: u4
    - id: error_entry_point
      type: u4
    - id: gdt_max
      type: u4
    - id: gdt_base
      type: u4
    - id: segment_sel
      type: u4
    - id: entry_point
      type: u4
    - id: reserved
      size: 64
    - id: key_size
      type: u4
      doc: counted in 4 byte increments
    - id: scratch_space_size
      type: u4
      doc: counted in 4 byte increments
    - id: rsa_public_key
      size: (4 * key_size)
    - id: rsa_exponent
      type: u4
      if: header_version == 0
    - id: rsa_signature
      size: (4 * key_size)
    - id: scratch_space
      size: (4 * scratch_space_size)

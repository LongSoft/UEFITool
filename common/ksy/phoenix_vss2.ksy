meta:
  id: phoenix_vss2
  title: Phoenix VSS2 NVRAM variable storage
  application: Phoenix SCT-based UEFI firmware
  file-extension: vss2
  tags:
    - firmware
  license: CC0-1.0
  ks-version: 0.9
  endian: le

seq:
- id: signature
  type: u4
  valid:
   expr: _ == 0xDDCF3617 or _ == 0xAAF32C78 # Beginning of known store GUIDs for VSS2
- id: signature_auth_var_key_db
  contents: [0x7B, 0x94, 0x9A, 0x43, 0xA1, 0x80, 0x2E, 0x14, 0x4E, 0xC3, 0x77, 0x92] # AAF32C78-947B-439A-A180-2E144EC37792
  if: signature == 0xAAF32C78
- id: signature_vss2_store
  contents: [0x75, 0x32, 0x64, 0x41, 0x98, 0xB6, 0xFE, 0x85, 0x70, 0x7F, 0xFE, 0x7D] # DDCF3617-3275-4164-98B6-FE85707FFE7D
  if: signature == 0xDDCF3617
- id: vss2_size
  type: u4
  valid:
   expr: _ > len_vss2_store_header and _ < 0xFFFFFFFF
- id: format
  type: u1
  valid:
    expr: _ == 0x5a # Formatted
- id: state
  type: u1
- id: reserved
  type: u2
- id: reserved1
  type: u4
- id: body
  type: vss2_store_body
  size: vss2_size - len_vss2_store_header
instances:
  len_vss2_store_header:
    value: 7 * sizeof<u4>

types:
 vss2_store_body:
  seq:
  - id: variables
    type: vss2_variable
    repeat: until
    repeat-until: _.signature_first != 0xAA or _io.eof

 vss2_variable_attributes:
  seq:
  - id: non_volatile
    type: b1le
  - id: boot_service
    type: b1le
  - id: runtime
    type: b1le
  - id: hw_error_record
    type: b1le
  - id: auth_write
    type: b1le
  - id: time_based_auth
    type: b1le
  - id: append_write
    type: b1le
  - id: reserved
    type: b25le

# TODO: check if VSS2 stores can have standard VSS variables
 vss2_variable:
  seq:
  - id: invoke_offset
    size: 0
    if: offset >= 0
  - id: signature_first
    type: u1
  - id: signature_last
    type: u1
    valid:
     expr: _ == 0x55
    if: signature_first == 0xAA
  - id: state
    type: u1
    if: signature_first == 0xAA
  - id: reserved
    type: u1
    if: signature_first == 0xAA
  - id: attributes
    type: vss2_variable_attributes
    if: signature_first == 0xAA
  - id: len_name
    type: u4
    if: signature_first == 0xAA
  - id: len_data
    type: u4
    if: signature_first == 0xAA
# vvv Auth variable
  - id: timestamp
    size: 16
    if: signature_first == 0xAA and is_auth
  - id: pubkey_index
    type: u4
    if: signature_first == 0xAA and is_auth
  - id: len_name_auth
    type: u4
    if: signature_first == 0xAA and is_auth
  - id: len_data_auth
    type: u4
    if: signature_first == 0xAA and is_auth
# ^^^ Auth variable
  - id: vendor_guid
    size: 16
    if: signature_first == 0xAA
# vvv Auth variable
  - id: name_auth
    size: len_name_auth
    if: signature_first == 0xAA and is_auth
  - id: data_auth
    size: len_data_auth
    if: signature_first == 0xAA and is_auth
  - id: invoke_end_offset_auth
    size: 0
    if: signature_first == 0xAA and is_auth and end_offset_auth >= 0
  - id: alignment_padding_auth
    size: len_alignment_padding_auth
    if: signature_first == 0xAA and is_auth
# ^^^ Auth variable
  - id: name
    size: len_name
    if: signature_first == 0xAA and not is_auth
  - id: data
    size: len_data
    if: signature_first == 0xAA and not is_auth
  - id: invoke_end_offset
    size: 0
    if: signature_first == 0xAA and not is_auth and end_offset >= 0
  - id: alignment_padding
    size: len_alignment_padding
    if: signature_first == 0xAA and not is_auth
  instances:
    offset:
     value: _io.pos
    end_offset:
     value: _io.pos
    end_offset_auth:
     value: _io.pos
    len_alignment_padding:
     value: (((end_offset - offset)+3) & ~3) - (end_offset - offset)
    len_alignment_padding_auth:
     value: (((end_offset_auth - offset)+3) & ~3) - (end_offset - offset)
    is_valid:
      value: state == 0x7F or state == 0x3F
    is_auth:
     value: (attributes.auth_write or attributes.time_based_auth or attributes.append_write) or (len_name == 0 or len_data == 0)
    len_auth_header:
      value: 60
    len_standard_header:
      value: 32

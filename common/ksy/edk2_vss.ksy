meta:
  id: edk2_vss
  title: EDK2 VSS NVRAM store
  application: EDK2-based UEFI firmware
  file-extension: vss
  tags:
    - firmware
  license: CC0-1.0
  ks-version: 0.9
  endian: le

seq:
- id: signature
  type: u4
  valid:
   expr: _ == 0x53535624 or _ == 0x53565324 or _ == 0x53534E24 # $VSS/$SVS/$NSS
- id: vss_size
  type: u4
  valid:
   expr: _ > len_vss_store_header and _ < 0xFFFFFFFF
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
  type: vss_store_body
  size: vss_size - len_vss_store_header
instances:
  len_vss_store_header:
   value: 16

types:
 vss_store_body:
  seq:
  - id: variables
    type: vss_variable
    repeat: until
    repeat-until: _.signature_first != 0xAA or _io.eof

 vss_variable_attributes:
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
    type: b24le
  - id: apple_data_checksum
    type: b1le

 vss_variable:
  seq:
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
    type: vss_variable_attributes
    if: signature_first == 0xAA
# vvv Intel legacy
  - id: len_total
    type: u4
    if: signature_first == 0xAA and is_intel_legacy
    valid:
     expr: _ >= len_intel_legacy_header + 4 + 1 # Header size + at least one UCS2 character for the name + UCS2 null terminator + at least one byte of data
# ^^^ Intel legacy
# Next 2 fields can be of any value for an authenticated variable due to them being a combined value of MonothonicCounter
  - id: len_name
    type: u4
    if: signature_first == 0xAA and not is_intel_legacy
  - id: len_data
    type: u4
    if: signature_first == 0xAA and not is_intel_legacy
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
    valid:
     expr: (_ >= 4) and (_ % 2 == 0) # UCS2 characters come in byte pairs
  - id: len_data_auth
    type: u4
    if: signature_first == 0xAA and is_auth
    valid:
     expr: _ > 0
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
# ^^^ Auth variable
# vvv Apple MacEFI
  - id: apple_data_crc32
    type: u4
    if: signature_first == 0xAA and not is_intel_legacy and not is_auth and attributes.apple_data_checksum
# ^^^ Apple MacEFI
# vvv Intel legacy
  - id: intel_legacy_data
    size: len_total - len_intel_legacy_header
    if: signature_first == 0xAA and is_intel_legacy
# ^^^ Intel legacy
  - id: name
    size: len_name
    if: signature_first == 0xAA and not is_intel_legacy and not is_auth
    valid:
     expr: (len_name >= 4) and (len_name % 2 == 0)
  - id: data
    size: len_data
    if: signature_first == 0xAA and not is_intel_legacy and not is_auth
    valid:
     expr: len_name > 0
  instances:
    is_valid:
      value: state == 0xFC or state == 0x7F or state == 0x3F
    is_intel_legacy:
      value: (state == 0xF8 or state == 0xFC) # Special states indicating Intel legacy variables
    is_auth:
      value: state != 0xF8 and state != 0xFC and ((attributes.auth_write or attributes.time_based_auth or attributes.append_write) or (len_name == 0 or len_data == 0))
    len_intel_legacy_header:
      value: 28
    len_auth_header:
      value: 60
    len_standard_header:
      value: 32
    len_apple_header:
      value: 36

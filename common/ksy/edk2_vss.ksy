meta:
  id: edk2_vss
  title: EDK2 VSS storage
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
- id: size
  type: u4
  valid:
   expr: _ > 4 * sizeof<u4>
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
  size: size - 4 * sizeof<u4>

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
    #TODO: add Intel legacy total_size variant
  - id: len_name
    type: u4
    if: signature_first == 0xAA
  - id: len_data
    type: u4
    if: signature_first == 0xAA
  - id: vendor_guid
    size: 16
    if: signature_first == 0xAA
  - id: apple_data_crc32
    type: u4
    if: signature_first == 0xAA and attributes.apple_data_checksum
  - id: name
    size: len_name
    if: signature_first == 0xAA
  - id: data
    size: len_data
    if: signature_first == 0xAA

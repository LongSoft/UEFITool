meta:
  id: ms_slic_pubkey
  title: Microsoft SLIC Public Key
  application: Phoenix-based UEFI firmware
  file-extension: slpk
  tags:
    - firmware
  license: CC0-1.0
  ks-version: 0.9
  endian: le

seq:
- id: type
  type: u4
- id: len_pubkey
  type: u4
- id: key_type
  type: u1
- id: version
  type: u1
- id: reserved
  type: u2
- id: algorithm
  type: u4
- id: magic
  type: u4
- id: bit_length
  type: u4
- id: exponent
  type: u4
- id: modulus
  size: 128

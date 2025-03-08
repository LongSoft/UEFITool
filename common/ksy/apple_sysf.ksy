meta:
  id: apple_sysf
  title: Apple System Flags store
  application: Apple MacEFI-based UEFI firmware
  file-extension: sysf
  tags:
    - firmware
  license: CC0-1.0
  ks-version: 0.9
  endian: le
  
seq:
- id: signature
  type: u4
- id: unknown
  type: u1
- id: unknown1
  type: u4
- id: sysf_size
  type: u2
- id: body
  type: sysf_store_body
  size: sysf_size - len_sysf_store_header - sizeof<u4>
- id: crc
  type: u4

instances:
  len_sysf_store_header:
   value: 11

types:
 sysf_store_body:
  seq:
  - id: variables
    type: sysf_variable
    repeat: until
    repeat-until: (_.len_name == 3 and _.name == "EOF") or _io.eof
  - id: zeroes
    type: u1
    repeat: eos
    
 sysf_variable:
  seq:
  - id: len_name
    type: b7le
  - id: invalid_flag
    type: b1le
  - id: name
    type: strz
    encoding: ascii
    size: len_name
  - id: len_data
    type: u2
    if: name != "EOF"
  - id: data
    size: len_data
    if: name != "EOF"
 

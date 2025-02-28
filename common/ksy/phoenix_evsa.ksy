meta:
  id: phoenix_evsa
  title: Phoenix EVSA NVRAM store
  application: Phoenix-based UEFI firmware
  file-extension: evsa
  tags:
    - firmware
  license: CC0-1.0
  ks-version: 0.9
  endian: le
  
seq:
- id: type
  type: u1
  valid: 0xEC
- id: checksum
  type: u1
- id: size
  type: u2
  valid: 20
- id: signature
  type: u4
  valid: 0x41535645
- id: attributes
  type: u4
- id: store_size
  type: u4
- id: reserved
  type: u4
- id: body
  type: evsa_body
  size: store_size - 20
  
types:
  evsa_guid:
   seq:
   - id: guid_id
     type: u2
   - id: guid
     size: 16

  evsa_name:
   seq:
   - id: var_id
     type: u2
   - id: name
     size: _parent.size - 6

  evsa_data:
   seq:
   - id: guid_id
     type: u2
   - id: var_id
     type: u2
   - id: attributes
     type: u4
   - id: data_size
     type: u4
     if: (attributes & 0x10000000) != 0x10000000
   - id: data
     size:  _parent.size - 12
     if: (attributes & 0x10000000) == 0x10000000
   - id: data_ext
     size: data_size
     if: (attributes & 0x10000000) != 0x10000000

  evsa_unknown:
   seq:
   - id: unknown 
     size: 0
     
  evsa_entry:
   seq:
   - id: type
     type: u1
   - id: checksum
     type: u1
   - id: size
     type: u2
   - id: body
     type:
      switch-on: type
      cases:
        0xED: evsa_guid
        0xE1: evsa_guid
        0xEE: evsa_name
        0xE2: evsa_name
        0xEF: evsa_data
        0xE3: evsa_data
        0x83: evsa_data
        _:    evsa_unknown
        
  evsa_body:
   seq:
   - id: entries
     type: evsa_entry
     repeat: eos
        
        
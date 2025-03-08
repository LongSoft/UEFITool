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
- id: checksum
  type: u1
- id: len_evsa_store_header
  type: u2
- id: signature
  type: u4
- id: attributes
  type: u4
- id: len_evsa_store
  type: u4
- id: reserved
  type: u4
- id: body
  type: evsa_body
  size: len_evsa_store - len_evsa_store_header

types:
  evsa_guid:
   seq:
   - id: guid_id
     type: u2
   - id: guid
     size: 16
     valid:
      expr: _parent.len_evsa_entry == 22

  evsa_name:
   seq:
   - id: var_id
     type: u2
   - id: name
     size: _parent.len_evsa_entry - 6

  evsa_variable_attributes:
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
     type: b21le
   - id: extended_header
     type: b1le
   - id: reserved1
     type: b3le
     
  evsa_data:
   seq:
   - id: guid_id
     type: u2
   - id: var_id
     type: u2
   - id: attributes
     type: evsa_variable_attributes
   - id: len_data_ext
     type: u4
     if: attributes.extended_header
   - id: data
     size:  _parent.len_evsa_entry - 12
     if: not attributes.extended_header
   - id: data_ext
     size: len_data_ext
     if: attributes.extended_header

  evsa_unknown:
   seq:
   - id: unknown
     size: 0

  evsa_entry:
   seq:
   - id: entry_type
     type: u1
   - id: checksum
     type: u1
     if: entry_type == 0xE1
      or entry_type == 0xE2
      or entry_type == 0xE3
      or entry_type == 0xED
      or entry_type == 0xEE
      or entry_type == 0xEF
      or entry_type == 0x83
   - id: len_evsa_entry
     type: u2
     if: entry_type == 0xE1
      or entry_type == 0xE2
      or entry_type == 0xE3
      or entry_type == 0xED
      or entry_type == 0xEE
      or entry_type == 0xEF
      or entry_type == 0x83
   - id: body
     type:
      switch-on: entry_type
      cases:
        0xED: evsa_guid
        0xE1: evsa_guid
        0xEE: evsa_name
        0xE2: evsa_name
        0xEF: evsa_data
        0xE3: evsa_data
        0x83: evsa_data
        _: evsa_unknown
        
  evsa_body:
   seq:
   - id: entries
     type: evsa_entry
     repeat: until
     repeat-until: (_.entry_type != 0xED
                and _.entry_type != 0xEE
                and _.entry_type != 0xEF
                and _.entry_type != 0xE1
                and _.entry_type != 0xE2
                and _.entry_type != 0xE3
                and _.entry_type != 0x83)
                or _io.eof
   - id: free_space
     type: u1
     repeat: eos

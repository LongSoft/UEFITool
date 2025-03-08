meta:
  id: phoenix_flm
  title: Phoenix Flash Map
  application: Phoenix-based UEFI firmware
  file-extension: flm
  tags:
    - firmware
  license: CC0-1.0
  ks-version: 0.9
  endian: le
  
seq:
- id: signature
  size: 10
- id: num_entries
  type: u2
- id: reserved
  type: u4
- id: entries
  type: flm_entry
  repeat: expr
  repeat-expr: num_entries
  
instances:
  len_flm_store_header:
    value: 16
  len_flm_entry:
    value: 36

types:
  flm_entry:
   seq:
   - id: guid
     size: 16
   - id: data_type
     type: u2
   - id: entry_type
     type: u2
   - id: physical_address
     type: u8
   - id: size
     type: u4
   - id: offset
     type: u4


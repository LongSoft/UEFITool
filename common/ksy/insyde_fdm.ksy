meta:
  id: insyde_fdm
  title: Insyde Flash Device
  application: Insyde-based UEFI firmware
  file-extension: fdm
  tags:
    - firmware
  license: CC0-1.0
  ks-version: 0.9
  endian: le

seq:
- id: signature
  type: u4
  valid: 0x4D444648 # HFDM
- id: store_size
  type: u4
- id: data_offset
  type: u4
- id: entry_size
  type: u4
- id: entry_format
  type: u1
- id: revision
  type: u1
- id: num_extensions
  type: u1
- id: checksum
  type: u1
- id: fd_base_address
  type: u8
- id: extensions
  type: fdm_extensions
  size: num_extensions * sizeof<fdm_extension>
  if: revision == 3
- id: board_ids
  type: fdm_board_ids
  if: revision == 3
- id: entries
  type: fdm_entries
  size: store_size - data_offset

types:
 fdm_extensions:
  seq:
  - id: extensions
    type: fdm_extension
    repeat: eos

 fdm_extension:
  seq:
  - id: offset
    type: u2
  - id: count
    type: u2
    
 fdm_board_ids:
  seq:
  - id: region_index
    type: u4
  - id: num_board_ids
    type: u4
  - id: board_ids
    type: u8
    repeat: expr
    repeat-expr: num_board_ids
 
 fdm_entries:
  seq:
   - id: entries
     type: fdm_entry
     repeat: eos
     
 fdm_entry:
  seq:
  - id: guid
    size: 16
  - id: region_id
    size: 16
  - id: region_offset
    type: u8
  - id: region_size
    type: u8
  - id: attributes
    type: u4
  - id: hash
    size: _parent._parent.entry_size - 16 - 16 - 8 - 8 - 4
  instances:
   region_base:
     value: _root.fd_base_address.as<u4> + region_offset.as<u4>
     

meta:
  id: phoenix_flm
  title: Phoenix flash map
  application: Phoenix-based UEFI firmware
  file-extension: flm
  tags:
    - firmware
  license: CC0-1.0
  ks-version: 0.9
  endian: le
  
seq:
- id: signature
  contents: [0x5F, 0x46, 0x4C, 0x41, 0x53, 0x48, 0x5F, 0x4D, 0x41, 0x50] # _FLASH_MAP
- id: num_entries
  type: u2
  valid: 
   expr: _ <= 113 # Needs to fit into the last 0x1000 bytes of the NVRAM volume
- id: reserved
  type: u4
- id: entries
  type: flm_entry
  repeat: expr
  repeat-expr: num_entries
- id: free_space
  type: u1
  repeat: expr
  repeat-expr: len_flm_store - len_flm_store_header - len_flm_entry * num_entries
  
instances:
  len_flm_store:
   value: 0x1000
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
    
    

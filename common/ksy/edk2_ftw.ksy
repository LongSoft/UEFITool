meta:
  id: edk2_ftw
  title: EDK2 Fault Tolerant Write NVRAM store
  application: EDK2-based UEFI firmware
  file-extension: ftw
  tags:
    - firmware
  license: CC0-1.0
  ks-version: 0.9
  endian: le

seq:
- id: signature
  size: 16
- id: crc
  type: u4
- id: state
  type: u1
- id: reserved
  size: 3
- id: len_write_queue_32
  type: u4
- id: len_write_queue_64
  type: u4
  if: len_write_queue_32 % 0x10 == 0
- id: write_queue_32
  size: len_write_queue_32
  if: len_write_queue_32 % 0x10 == 0x04
- id: write_queue_64
  size: ((len_write_queue_64.as<u8>) << 32) + len_write_queue_32
  if: len_write_queue_32 % 0x10 == 0
  
instances:
  len_ftw_store_header_32:
   value: 28
  len_ftw_store_header_64:
   value: 32

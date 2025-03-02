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
  valid:
   expr: _ == [0x8D, 0x2B, 0xF1, 0xFF, 0x96, 0x76, 0x8B, 0x4C, 0xA9, 0x85, 0x27, 0x47, 0x07, 0x5B, 0x4F, 0x50]
         or _ == [0x2B, 0x29, 0x58, 0x9E, 0x68, 0x7C, 0x7D, 0x49, 0x0A, 0xCE, 0x65, 0x00, 0xFD, 0x9F, 0x1B, 0x95]
         or _ == [0x2B, 0x29, 0x58, 0x9E, 0x68, 0x7C, 0x7D, 0x49, 0xA0, 0xCE, 0x65, 0x00, 0xFD, 0x9F, 0x1B, 0x95]
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

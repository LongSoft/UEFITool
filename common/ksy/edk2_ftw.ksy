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
  type: u4
  valid:
   expr: _ == 0xFFF12B8D or _ == 0x9E58292B
- id: signature_main
  contents: [0x96, 0x76, 0x8B, 0x4C, 0xA9, 0x85, 0x27, 0x47, 0x07, 0x5B, 0x4F, 0x50] # FF12B8D-7696-4C8B-A985-2747075B4F50
  if: signature == 0xFFF12B8D
- id: signature_edk2_working_block
  contents: [0x68, 0x7C, 0x7D, 0x49, 0x0A, 0xCE, 0x65, 0x00, 0xFD, 0x9F, 0x1B, 0x95] # 9E58292B-7C68-497D-0ACE-6500FD9F1B95
  if: signature ==  0x9E58292B
- id: signature_vss2_working_block
  contents: [0x68, 0x7C, 0x7D, 0x49, 0xA0, 0xCE, 0x65, 0x00, 0xFD, 0x9F, 0x1B, 0x95] # 9E58292B-7C68-497D-A0CE-6500FD9F1B95
  if: signature ==  0x9E58292B
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

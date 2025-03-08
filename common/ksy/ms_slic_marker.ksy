meta:
  id: ms_slic_marker
  title: Microsoft SLIC Marker
  application: Phoenix-based UEFI firmware
  file-extension: slmr
  tags:
    - firmware
  license: CC0-1.0
  ks-version: 0.9
  endian: le

seq:
- id: type
  type: u4
- id: len_marker
  type: u4
- id: version
  type: u4
- id: oem_id
  size: 6
- id: oem_table_id
  size: 8
- id: windows_flag
  type: u8
- id: slic_version
  type: u4
- id: reserved
  size: 16
- id: signature
  size: 128

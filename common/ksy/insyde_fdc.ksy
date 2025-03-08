meta:
  id: insyde_fdc
  title: Insyde Factory Data Copy NVRAM store
  application: Insyde-based UEFI firmware
  file-extension: fdc
  tags:
    - firmware
  license: CC0-1.0
  ks-version: 0.9
  endian: le

seq:
- id: signature
  type: u4
- id: fdc_size
  type: u4
  valid:
   expr: _ > len_fdc_store_header.as<u4> and _ < 0xFFFFFFFF
- id: body
  size: fdc_size - len_fdc_store_header
instances:
  len_fdc_store_header:
   value: 0x50

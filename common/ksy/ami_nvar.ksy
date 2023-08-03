meta:
  id: ami_nvar
  title: AMI Aptio NVRAM Storage
  application: AMI Aptio-based UEFI firmware
  file-extension: nvar
  tags:
    - firmware
  license: CC0-1.0
  ks-version: 0.9
  endian: le

seq:
- id: entries
  type: nvar_entry
  repeat: until
  repeat-until: _.signature_first != 0x4e or _io.eof

types:
  nvar_entry:
   seq:
   - id: invoke_offset
     size: 0
     if: offset >= 0
   - id: signature_first
     type: u1
   - id: signature_rest
     contents: [VAR]
     if: signature_first == 0x4e
   - id: size
     type: u2
     valid:
      expr: _ > sizeof<u4> + sizeof<u2> + sizeof<u4>
     if: signature_first == 0x4e
   - id: next
     type: b24le
     if: signature_first == 0x4e
   - id: attributes
     type: nvar_attributes
     if: signature_first == 0x4e
   - id: body
     type: nvar_entry_body
     size: size - (sizeof<u4> + sizeof<u2> + sizeof<u4>)
     if: signature_first == 0x4e
   - id: invoke_end_offset
     size: 0
     if: signature_first == 0x4e and end_offset >= 0
   instances:
    offset:
     value: _io.pos
    end_offset:
     value: _io.pos

  nvar_attributes:
   seq:
    - id: valid
      type: b1
    - id: auth_write
      type: b1
    - id: hw_error_record
      type: b1
    - id: extended_header
      type: b1
    - id: data_only
      type: b1
    - id: local_guid
      type: b1
    - id: ascii_name
      type: b1
    - id: runtime
      type: b1

  nvar_extended_attributes:
   seq:
    - id: reserved_high
      type: b2
    - id: time_based_auth
      type: b1
    - id: auth_write
      type: b1
    - id: reserved_low
      type: b3
    - id: checksum
      type: b1

  ucs2_string:
   seq:
   - id: ucs2_chars
     type: u2
     repeat: until
     repeat-until: _ == 0

  nvar_entry_body:
   seq:
   - id: guid_index
     type: u1
     if: (not _parent.attributes.local_guid)
      and (not _parent.attributes.data_only)
      and (_parent.attributes.valid)
   - id: guid
     size: 16
     if: (_parent.attributes.local_guid)
      and (not _parent.attributes.data_only)
      and (_parent.attributes.valid)
   - id: ascii_name
     type: strz
     encoding: ASCII
     if: (_parent.attributes.ascii_name)
      and (not _parent.attributes.data_only)
      and (_parent.attributes.valid)
   - id: ucs2_name
     type: ucs2_string
     if: (not _parent.attributes.ascii_name)
      and (not _parent.attributes.data_only)
      and (_parent.attributes.valid)
   - id: invoke_data_start
     size: 0
     if: data_start_offset >= 0
   - id: data
     size-eos: true
   instances:
    extended_header_size_field:
     pos: _io.pos - sizeof<u2>
     type: u2
     if: _parent.attributes.valid
      and _parent.attributes.extended_header
      and _parent.size > sizeof<u4> + sizeof<u2> + sizeof<u4> + sizeof<u2>
    extended_header_size:
     value: '(_parent.attributes.extended_header and _parent.attributes.valid and (_parent.size > sizeof<u4> + sizeof<u2> + sizeof<u4> + sizeof<u2>)) ? (extended_header_size_field >= sizeof<nvar_extended_attributes> + sizeof<u2> ? extended_header_size_field : 0) : 0'
    extended_header_attributes:
     pos: _io.pos - extended_header_size
     type: nvar_extended_attributes
     if: _parent.attributes.valid
      and _parent.attributes.extended_header
      and (extended_header_size >= sizeof<nvar_extended_attributes> + sizeof<u2>)
    extended_header_timestamp:
     pos: _io.pos - extended_header_size + sizeof<nvar_extended_attributes>
     type: u8
     if: _parent.attributes.valid
      and _parent.attributes.extended_header
      and (extended_header_size >= sizeof<nvar_extended_attributes> + sizeof<u8> + sizeof<u2>)
      and extended_header_attributes.time_based_auth
    extended_header_hash:
     pos: _io.pos - extended_header_size + sizeof<nvar_extended_attributes> + sizeof<u8>
     size: 32
     if: _parent.attributes.valid
      and _parent.attributes.extended_header
      and (extended_header_size >= sizeof<nvar_extended_attributes> + sizeof<u8> + 32 + sizeof<u2>)
      and extended_header_attributes.time_based_auth
      and (not _parent.attributes.data_only)
    extended_header_checksum:
     pos: _io.pos - sizeof<u2> - sizeof<u1>
     type: u1
     if: _parent.attributes.valid
      and _parent.attributes.extended_header
      and (extended_header_size >= sizeof<nvar_extended_attributes> + sizeof<u1> + sizeof<u2>)
      and extended_header_attributes.checksum
    data_start_offset:
     value: _io.pos
    data_end_offset:
     value: _io.pos
    data_size:
     value: (data_end_offset - data_start_offset) - extended_header_size

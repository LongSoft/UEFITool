// This is a generated file! Please edit source .ksy file and use kaitai-struct-compiler to rebuild

#include "insyde_fdm.h"
#include "../kaitai/exceptions.h"

insyde_fdm_t::insyde_fdm_t(kaitai::kstream* p__io, kaitai::kstruct* p__parent, insyde_fdm_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = this; (void)p__root;
    m_extensions = nullptr;
    m__io__raw_extensions = nullptr;
    m_board_ids = nullptr;
    m_entries = nullptr;
    m__io__raw_entries = nullptr;
    _read();
}

void insyde_fdm_t::_read() {
    m_signature = m__io->read_u4le();
    if (!(signature() == 1296320072)) {
        throw kaitai::validation_not_equal_error<uint32_t>(1296320072, signature(), _io(), std::string("/seq/0"));
    }
    m_store_size = m__io->read_u4le();
    m_data_offset = m__io->read_u4le();
    m_entry_size = m__io->read_u4le();
    m_entry_format = m__io->read_u1();
    m_revision = m__io->read_u1();
    m_num_extensions = m__io->read_u1();
    m_checksum = m__io->read_u1();
    m_fd_base_address = m__io->read_u8le();
    n_extensions = true;
    if (revision() == 3) {
        n_extensions = false;
        m__raw_extensions = m__io->read_bytes((num_extensions() * 4));
        m__io__raw_extensions = std::unique_ptr<kaitai::kstream>(new kaitai::kstream(m__raw_extensions));
        m_extensions = std::unique_ptr<fdm_extensions_t>(new fdm_extensions_t(m__io__raw_extensions.get(), this, m__root));
    }
    n_board_ids = true;
    if (revision() == 3) {
        n_board_ids = false;
        m_board_ids = std::unique_ptr<fdm_board_ids_t>(new fdm_board_ids_t(m__io, this, m__root));
    }
    m__raw_entries = m__io->read_bytes((store_size() - data_offset()));
    m__io__raw_entries = std::unique_ptr<kaitai::kstream>(new kaitai::kstream(m__raw_entries));
    m_entries = std::unique_ptr<fdm_entries_t>(new fdm_entries_t(m__io__raw_entries.get(), this, m__root));
}

insyde_fdm_t::~insyde_fdm_t() {
    _clean_up();
}

void insyde_fdm_t::_clean_up() {
    if (!n_extensions) {
    }
    if (!n_board_ids) {
    }
}

insyde_fdm_t::fdm_entries_t::fdm_entries_t(kaitai::kstream* p__io, insyde_fdm_t* p__parent, insyde_fdm_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;
    m_entries = nullptr;
    _read();
}

void insyde_fdm_t::fdm_entries_t::_read() {
    m_entries = std::unique_ptr<std::vector<std::unique_ptr<fdm_entry_t>>>(new std::vector<std::unique_ptr<fdm_entry_t>>());
    {
        int i = 0;
        while (!m__io->is_eof()) {
            m_entries->push_back(std::move(std::unique_ptr<fdm_entry_t>(new fdm_entry_t(m__io, this, m__root))));
            i++;
        }
    }
}

insyde_fdm_t::fdm_entries_t::~fdm_entries_t() {
    _clean_up();
}

void insyde_fdm_t::fdm_entries_t::_clean_up() {
}

insyde_fdm_t::fdm_extension_t::fdm_extension_t(kaitai::kstream* p__io, insyde_fdm_t::fdm_extensions_t* p__parent, insyde_fdm_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;
    _read();
}

void insyde_fdm_t::fdm_extension_t::_read() {
    m_offset = m__io->read_u2le();
    m_count = m__io->read_u2le();
}

insyde_fdm_t::fdm_extension_t::~fdm_extension_t() {
    _clean_up();
}

void insyde_fdm_t::fdm_extension_t::_clean_up() {
}

insyde_fdm_t::fdm_board_ids_t::fdm_board_ids_t(kaitai::kstream* p__io, insyde_fdm_t* p__parent, insyde_fdm_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;
    m_board_ids = nullptr;
    _read();
}

void insyde_fdm_t::fdm_board_ids_t::_read() {
    m_region_index = m__io->read_u4le();
    m_num_board_ids = m__io->read_u4le();
    m_board_ids = std::unique_ptr<std::vector<uint64_t>>(new std::vector<uint64_t>());
    const int l_board_ids = num_board_ids();
    for (int i = 0; i < l_board_ids; i++) {
        m_board_ids->push_back(std::move(m__io->read_u8le()));
    }
}

insyde_fdm_t::fdm_board_ids_t::~fdm_board_ids_t() {
    _clean_up();
}

void insyde_fdm_t::fdm_board_ids_t::_clean_up() {
}

insyde_fdm_t::fdm_extensions_t::fdm_extensions_t(kaitai::kstream* p__io, insyde_fdm_t* p__parent, insyde_fdm_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;
    m_extensions = nullptr;
    _read();
}

void insyde_fdm_t::fdm_extensions_t::_read() {
    m_extensions = std::unique_ptr<std::vector<std::unique_ptr<fdm_extension_t>>>(new std::vector<std::unique_ptr<fdm_extension_t>>());
    {
        int i = 0;
        while (!m__io->is_eof()) {
            m_extensions->push_back(std::move(std::unique_ptr<fdm_extension_t>(new fdm_extension_t(m__io, this, m__root))));
            i++;
        }
    }
}

insyde_fdm_t::fdm_extensions_t::~fdm_extensions_t() {
    _clean_up();
}

void insyde_fdm_t::fdm_extensions_t::_clean_up() {
}

insyde_fdm_t::fdm_entry_t::fdm_entry_t(kaitai::kstream* p__io, insyde_fdm_t::fdm_entries_t* p__parent, insyde_fdm_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;
    f_region_base = false;
    _read();
}

void insyde_fdm_t::fdm_entry_t::_read() {
    m_guid = m__io->read_bytes(16);
    m_region_id = m__io->read_bytes(16);
    m_region_offset = m__io->read_u8le();
    m_region_size = m__io->read_u8le();
    m_attributes = m__io->read_u4le();
    m_hash = m__io->read_bytes((((((_parent()->_parent()->entry_size() - 16) - 16) - 8) - 8) - 4));
}

insyde_fdm_t::fdm_entry_t::~fdm_entry_t() {
    _clean_up();
}

void insyde_fdm_t::fdm_entry_t::_clean_up() {
}

int32_t insyde_fdm_t::fdm_entry_t::region_base() {
    if (f_region_base)
        return m_region_base;
    m_region_base = (static_cast<uint32_t>(_root()->fd_base_address()) + static_cast<uint32_t>(region_offset()));
    f_region_base = true;
    return m_region_base;
}

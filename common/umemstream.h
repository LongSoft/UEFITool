/* umemstream.h

 Copyright (c) 2023, Nikolaj Schlej. All rights reserved.
 This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php

 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 */

#ifndef UMEMSTREAM_H
#define UMEMSTREAM_H

#include <sstream>

// NOTE: this implementation is certainly not a valid replacement to std::stringstream
// NOTE: because it only supports getting through the buffer once
// NOTE: however, we already do it this way, so it's enough for practical purposes

class umembuf : public std::streambuf {
public:
    umembuf(const char *p, size_t l) {
        setg((char*)p, (char*)p, (char*)p + l);
    }

    pos_type seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which = std::ios_base::in) override
    {
        (void)which;
        if (dir == std::ios_base::cur)
            gbump((int)off);
        else if (dir == std::ios_base::end)
            setg(eback(), egptr() + off, egptr());
        else if (dir == std::ios_base::beg)
            setg(eback(), eback() + off, egptr());
        return gptr() - eback();
    }

    pos_type seekpos(pos_type sp, std::ios_base::openmode which) override
    {
        return seekoff(sp - pos_type(off_type(0)), std::ios_base::beg, which);
    }
};

class umemstream : public std::istream {
public:
  umemstream(const char *p, size_t l) : std::istream(&buffer_),
    buffer_(p, l) {
    rdbuf(&buffer_);
  }

private:
  umembuf buffer_;
};

#endif // UMEMSTREAM_H

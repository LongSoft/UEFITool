/* filesystem.h

Copyright (c) 2023, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "ustring.h"
#include "ubytearray.h"

bool isExistOnFs(const UString& path);
bool makeDirectory(const UString& dir);
bool changeDirectory(const UString& dir);
bool removeDirectory(const UString& dir);
bool readFileIntoBuffer(const UString& inPath, UByteArray& buf);
UString getAbsPath(const UString& path);

#endif

/* guiddatabase.h

Copyright (c) 2017, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHWARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*/

#ifndef GUID_DATABASE_H
#define GUID_DATABASE_H

#include "basetypes.h"
#include "ustring.h"

UString guidDatabaseLookup(const EFI_GUID & guid);
void initGuidDatabase(const UString & path = "", UINT32* numEntries = NULL);

#endif // GUID_DATABASE_H

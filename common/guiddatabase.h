/* guiddatabase.h

Copyright (c) 2017, LongSoft. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHWARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*/

#ifndef GUID_DATABASE_H
#define GUID_DATABASE_H

#include <map>
#include <string>

#include "basetypes.h"
#include "ustring.h"
#include "ffsparser.h"
#include "ffs.h"
#include "utility.h"

struct OperatorLessForGuids : public std::binary_function<EFI_GUID, EFI_GUID, bool>
{
    bool operator()(const EFI_GUID& lhs, const EFI_GUID& rhs) const
    {
        return (memcmp(&lhs, &rhs, sizeof(EFI_GUID)) < 0);
    }
};

typedef std::map<EFI_GUID, UString, OperatorLessForGuids> GuidDatabase;

UString guidDatabaseLookup(const EFI_GUID & guid);
void initGuidDatabase(const UString & path = "", UINT32* numEntries = NULL);
GuidDatabase guidDatabaseFromTreeRecursive(TreeModel * model, const UModelIndex index);
USTATUS guidDatabaseExportToFile(const UString & outPath, GuidDatabase & db);

#endif // GUID_DATABASE_H

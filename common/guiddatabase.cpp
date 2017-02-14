/* guiddatabase.cpp

Copyright (c) 2017, LongSoft. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHWARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*/

#include "guiddatabase.h"
#include "ubytearray.h"

// TODO: remove Qt dependency
#if defined(U_ENABLE_GUID_DATABASE_SUPPORT) && defined(QT_CORE_LIB)
#include <map>
#include <QFile>
#include <QUuid>

struct OperatorLessForGuids : public std::binary_function<EFI_GUID, EFI_GUID, bool>
{
    bool operator()(const EFI_GUID& lhs, const EFI_GUID& rhs) const
    {
        return (memcmp(&lhs, &rhs, sizeof(EFI_GUID)) < 0);
    }
};

typedef std::map<EFI_GUID, UString, OperatorLessForGuids> GuidToUStringMap;
static GuidToUStringMap gGuidToUStringMap;

void initGuidDatabase(const UString & path, UINT32* numEntries)
{
    gGuidToUStringMap.clear();

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    while (!file.atEnd()) {
        UByteArray line = file.readLine();

        // Use sharp sign as commentary
        if (line.length() == 0 || line[0] == '#')
            continue;

        // GUID and name are comma-separated 
        QList<UByteArray> lineParts = line.split(',');
        if (lineParts.length() < 2)
            continue;

        QUuid uuid(lineParts[0]);
        UString str(lineParts[1].constData());
        gGuidToUStringMap.insert(GuidToUStringMap::value_type(*(const EFI_GUID*)&uuid.data1, str.simplified()));
    }

    if (numEntries)
        *numEntries = gGuidToUStringMap.size();
}

UString guidDatabaseLookup(const EFI_GUID & guid)
{
    return gGuidToUStringMap[guid];
}

#else
void initGuidDatabase(const UString & path, UINT32* numEntries)
{
    U_UNUSED_PARAMETER(path);
    if (numEntries)
        *numEntries = 0;
}

UString guidDatabaseLookup(const EFI_GUID & guid)
{
	U_UNUSED_PARAMETER(guid);
    return UString();
}
#endif

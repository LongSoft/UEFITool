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
#include "ffs.h"

#if defined(U_ENABLE_GUID_DATABASE_SUPPORT)
#include <map>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdio>

struct OperatorLessForGuids : public std::binary_function<EFI_GUID, EFI_GUID, bool>
{
    bool operator()(const EFI_GUID& lhs, const EFI_GUID& rhs) const
    {
        return (memcmp(&lhs, &rhs, sizeof(EFI_GUID)) < 0);
    }
};

typedef std::map<EFI_GUID, UString, OperatorLessForGuids> GuidToUStringMap;
static GuidToUStringMap gGuidToUStringMap;

#ifdef QT_CORE_LIB

#include <QFile>
#include <QTextStream>

// This is required to be able to read Qt-embedded paths

static std::string readGuidDatabase(const UString &path) {
    QFile guids(path);
    if (guids.open(QFile::ReadOnly | QFile::Text))
        return QTextStream(&guids).readAll().toStdString();
    return std::string {};
}

#else

static std::string readGuidDatabase(const UString &path) {
    std::ifstream guids(path.toLocal8Bit());
    std::stringstream ret;
    if (ret)
        ret << guids.rdbuf();
    return ret.str();
}

#endif

void initGuidDatabase(const UString & path, UINT32* numEntries)
{
    gGuidToUStringMap.clear();

    std::stringstream file(readGuidDatabase(path));

    while (!file.eof()) {
        std::string line;
        std::getline(file, line);

        // Use sharp symbol as commentary
        if (line.size() == 0 || line[0] == '#')
            continue;

        // GUID and name are comma-separated 
        std::vector<UString> lineParts;
        std::string::size_type prev = 0, curr = 0;
        while ((curr = line.find(',', curr)) != std::string::npos) {
            std::string substring( line.substr(prev, curr-prev) );
            lineParts.push_back(UString(substring.c_str()));
            prev = ++curr;
        }
        lineParts.push_back(UString(line.substr(prev, curr-prev).c_str()));

        if (lineParts.size() < 2)
            continue;

        EFI_GUID guid;
        if (!ustringToGuid(lineParts[0], guid))
            continue;

        gGuidToUStringMap.insert(GuidToUStringMap::value_type(guid, lineParts[1]));
    }

    if (numEntries)
        *numEntries = (UINT32)gGuidToUStringMap.size();
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

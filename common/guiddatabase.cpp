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

#if defined(U_ENABLE_GUID_DATABASE_SUPPORT)
#include <map>
#include <fstream>
#include <sstream>
#include <string>
#include <cstdio>
#include <vector>

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

        unsigned long p0;
        int p1, p2, p3, p4, p5, p6, p7, p8, p9, p10;

        int err = std::sscanf(lineParts[0].toLocal8Bit(), "%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
            &p0, &p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10);
        if (err == 0)
            continue;

        guid.Data1 = p0;
        guid.Data2 = p1;
        guid.Data3 = p2;
        guid.Data4[0] = p3;
        guid.Data4[1] = p4;
        guid.Data4[2] = p5;
        guid.Data4[3] = p6;
        guid.Data4[4] = p7;
        guid.Data4[5] = p8;
        guid.Data4[6] = p9;
        guid.Data4[7] = p10;

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

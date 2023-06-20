/* filesystem.c

Copyright (c) 2023, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#include "filesystem.h"
#include <sys/stat.h>
#include <fstream>

bool readFileIntoBuffer(const UString& inPath, UByteArray& buf) 
{
    if (!isExistOnFs(inPath))
        return false;

    std::ifstream inputFile(inPath.toLocal8Bit(), std::ios::in | std::ios::binary);
    if (!inputFile)
        return false;
    std::vector<char> buffer(std::istreambuf_iterator<char>(inputFile),
        (std::istreambuf_iterator<char>()));
    inputFile.close();

    buf = buffer;

    return true;
}

#if defined(_WIN32) || defined(__MINGW32__)
#include <direct.h>
#include <stdlib.h>
bool isExistOnFs(const UString & path) 
{
    struct _stat buf;
    return (_stat(path.toLocal8Bit(), &buf) == 0);
}

bool makeDirectory(const UString & dir) 
{
    return (_mkdir(dir.toLocal8Bit()) == 0);
}

bool changeDirectory(const UString & dir) 
{
    return (_chdir(dir.toLocal8Bit()) == 0);
}

bool removeDirectory(const UString & dir) 
{
    int r = _rmdir(dir.toLocal8Bit());
    // Hack: unlike *nix, Windows does not permit deleting current directories
    if (r < 0 && errno == EACCES && changeDirectory(dir + UString("/../"))) {
        return (_rmdir(dir.toLocal8Bit()) == 0);
    }
    return (r == 0);
}

UString getAbsPath(const UString & path) 
{
    char * abs = (char*)calloc(0x8000, 1);
    UString new_path;
    if (_fullpath(abs, path.toLocal8Bit(), 0x8000))
        new_path = UString(abs);
    else
        new_path = path;
    free(abs);
    return new_path;
}
#else
#include <unistd.h>
#include <stdlib.h>
bool isExistOnFs(const UString & path) 
{
    struct stat buf;
    return (stat(path.toLocal8Bit(), &buf) == 0);
}

bool makeDirectory(const UString & dir) 
{
    return (mkdir(dir.toLocal8Bit(), ACCESSPERMS) == 0);
}

bool removeDirectory(const UString & dir) 
{
    return (rmdir(dir.toLocal8Bit()) == 0);
}

bool changeDirectory(const UString & dir) 
{
    return (chdir(dir.toLocal8Bit()) == 0);
}

UString getAbsPath(const UString & path) {
    char abs[PATH_MAX] = {};
    // Last is a non-standard extension for non-existent files
    if (realpath(path.toLocal8Bit(), abs) || abs[0] != '\0')
        return UString(abs);
    return path;
}
#endif
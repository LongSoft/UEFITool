/* ffsfinder.cpp

 Copyright (c) 2015, Nikolaj Schlej. All rights reserved.
 This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 */

#include "ffsfinder.h"

#if QT_VERSION_MAJOR >= 6
#include <QRegularExpression>
#else
#include <QRegExp>
#endif

USTATUS FfsFinder::findHexPattern(const UByteArray & hexPattern, const UINT8 mode) {
    const UModelIndex rootIndex = model->index(0, 0);
    USTATUS ret = findHexPattern(rootIndex, hexPattern, mode);
    if (ret != U_SUCCESS)
        msg(UString("Hex pattern \"") + UString(hexPattern) + UString("\" could not be found"), rootIndex);
    return ret;
}

USTATUS FfsFinder::findHexPattern(const UModelIndex & index, const UByteArray & hexPattern, const UINT8 mode)
{
    if (!index.isValid())
        return U_SUCCESS;
    
    if (hexPattern.isEmpty())
        return U_INVALID_PARAMETER;
    
    // Check for "all substrings" pattern
    if (hexPattern.count('.') == hexPattern.length())
        return U_SUCCESS;
    
    USTATUS ret = U_ITEM_NOT_FOUND;
    bool hasChildren = (model->rowCount(index) > 0);
    for (int i = 0; i < model->rowCount(index); i++) {
        if (U_SUCCESS == findHexPattern(index.model()->index(i, index.column(), index), hexPattern, mode))
            ret = U_SUCCESS;
    }
    
    UByteArray data;
    if (hasChildren) {
        if (mode == SEARCH_MODE_HEADER)
            data = model->header(index);
        else if (mode == SEARCH_MODE_ALL)
            data = model->header(index) + model->body(index);
    }
    else {
        if (mode == SEARCH_MODE_HEADER)
            data = model->header(index);
        else if (mode == SEARCH_MODE_BODY)
            data = model->body(index);
        else
            data = model->header(index) + model->body(index);
    }
    
    UString hexBody = UString(data.toHex());
#if QT_VERSION_MAJOR >= 6
    QRegularExpression regexp = QRegularExpression(UString(hexPattern));
    regexp.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch regexpmatch;
    
    INT32 offset = (INT32)hexBody.indexOf(regexp, 0, &regexpmatch);
#else
    QRegExp regexp = QRegExp(UString(hexPattern), Qt::CaseInsensitive);

    INT32 offset = regexp.indexIn(hexBody);
#endif
    while (offset >= 0) {
        if (offset % 2 == 0) {
            // For patterns that cross header|body boundary, skip patterns entirely located in body, since
            // children search above has already found them.
            if (!(hasChildren && mode == SEARCH_MODE_ALL && offset/2 >= model->header(index).size())) {
                UModelIndex parentFileIndex = model->findParentOfType(index, Types::File);
                UString name = model->name(index);
                if (model->parent(index) == parentFileIndex) {
                    name = model->name(parentFileIndex) + UString("/") + name;
                }
                else if (parentFileIndex.isValid()) {
                    name = model->name(parentFileIndex) + UString("/.../") + name;
                }

                msg(UString("Hex pattern \"") + UString(hexPattern)
                    + UString("\" found as \"") + hexBody.mid(offset, hexPattern.length()).toUpper()
                    + UString("\" in ") + name
                    + usprintf(" at %s-offset %02Xh", mode == SEARCH_MODE_BODY ? "body" : "header", offset / 2),
                    index);
                ret = U_SUCCESS;
            }
        }

#if QT_VERSION_MAJOR >= 6
        offset = (INT32)hexBody.indexOf(regexp, (qsizetype)offset + 1, &regexpmatch);
#else
        offset = regexp.indexIn(hexBody, offset + 1);
#endif
    }

    return ret;
}

USTATUS FfsFinder::findGuidPattern(const UByteArray & guidPattern, const UINT8 mode) {
    const UModelIndex rootIndex = model->index(0, 0);
    USTATUS ret = findGuidPattern(rootIndex, guidPattern, mode);
    if (ret != U_SUCCESS)
        msg(UString("GUID pattern \"") + UString(guidPattern) + UString("\" could not be found"), rootIndex);
    return ret;
}

USTATUS FfsFinder::findGuidPattern(const UModelIndex & index, const UByteArray & guidPattern, const UINT8 mode)
{
    if (guidPattern.isEmpty())
        return U_INVALID_PARAMETER;

    if (!index.isValid())
        return U_SUCCESS;

    USTATUS ret = U_ITEM_NOT_FOUND;
    bool hasChildren = (model->rowCount(index) > 0);
    for (int i = 0; i < model->rowCount(index); i++) {
        if (U_SUCCESS == findGuidPattern(index.model()->index(i, index.column(), index), guidPattern, mode))
            ret = U_SUCCESS;
    }

    UByteArray data;
    if (hasChildren) {
        if (mode != SEARCH_MODE_BODY)
            data = model->header(index);
    }
    else {
        if (mode == SEARCH_MODE_HEADER)
            data.append(model->header(index));
        else if (mode == SEARCH_MODE_BODY)
            data.append(model->body(index));
        else
            data.append(model->header(index)).append(model->body(index));
    }

    UString hexBody = UString(data.toHex());
    QList<UByteArray> list = guidPattern.split('-');
    if (list.count() != 5)
        return U_INVALID_PARAMETER;

    UByteArray hexPattern;
    // Reverse first GUID block
    hexPattern.append(list.at(0).mid(6, 2));
    hexPattern.append(list.at(0).mid(4, 2));
    hexPattern.append(list.at(0).mid(2, 2));
    hexPattern.append(list.at(0).mid(0, 2));
    // Reverse second GUID block
    hexPattern.append(list.at(1).mid(2, 2));
    hexPattern.append(list.at(1).mid(0, 2));
    // Reverse third GUID block
    hexPattern.append(list.at(2).mid(2, 2));
    hexPattern.append(list.at(2).mid(0, 2));
    // Append fourth and fifth GUID blocks as is
    hexPattern.append(list.at(3)).append(list.at(4));

    // Check for "all substrings" pattern
    if (hexPattern.count('.') == hexPattern.length())
        return U_SUCCESS;

#if QT_VERSION_MAJOR >= 6
    QRegularExpression regexp((QString)UString(hexPattern));
    regexp.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch regexpmatch;

    INT32 offset = (INT32)hexBody.indexOf(regexp, 0, &regexpmatch);
#else
    QRegExp regexp(UString(hexPattern), Qt::CaseInsensitive);

    INT32 offset = regexp.indexIn(hexBody);
#endif
    while (offset >= 0) {
        if (offset % 2 == 0) {
            UModelIndex parentFileIndex = model->findParentOfType(index, Types::File);
            UString name = model->name(index);
            if (model->parent(index) == parentFileIndex) {
                name = model->name(parentFileIndex) + UString("/") + name;
            }
            else if (parentFileIndex.isValid()) {
                name = model->name(parentFileIndex) + UString("/.../") + name;
            }

            msg(UString("GUID pattern \"") + UString(guidPattern)
                + UString("\" found as \"") + hexBody.mid(offset, hexPattern.length()).toUpper()
                + UString("\" in ") + name
                + usprintf(" at %s-offset %02Xh", mode == SEARCH_MODE_BODY ? "body" : "header", offset / 2),
                index);
            ret = U_SUCCESS;
        }

#if QT_VERSION_MAJOR >= 6
        offset = (INT32)hexBody.indexOf(regexp, (qsizetype)offset + 1, &regexpmatch);
#else
        offset = regexp.indexIn(hexBody, offset + 1);
#endif
    }

    return ret;
}

USTATUS FfsFinder::findTextPattern(const UString & pattern, const UINT8 mode, const bool unicode, const Qt::CaseSensitivity caseSensitive) {
    const UModelIndex rootIndex = model->index(0, 0);
    USTATUS ret = findTextPattern(rootIndex, pattern, mode, unicode, caseSensitive);
    if (ret != U_SUCCESS)
        msg((unicode ? UString("Unicode") : UString("ASCII")) + UString(" text \"")
            + UString(pattern) + UString("\" could not be found"), rootIndex);
    return ret;
}

USTATUS FfsFinder::findTextPattern(const UModelIndex & index, const UString & pattern, const UINT8 mode, const bool unicode, const Qt::CaseSensitivity caseSensitive)
{
    if (pattern.isEmpty())
        return U_INVALID_PARAMETER;

    if (!index.isValid())
        return U_SUCCESS;

    USTATUS ret = U_ITEM_NOT_FOUND;
    bool hasChildren = (model->rowCount(index) > 0);
    for (int i = 0; i < model->rowCount(index); i++) {
        if (U_SUCCESS == findTextPattern(index.model()->index(i, index.column(), index), pattern, mode, unicode, caseSensitive))
            ret = U_SUCCESS;
    }

    UByteArray body;
    if (hasChildren) {
        if (mode != SEARCH_MODE_BODY)
            body = model->header(index);
    }
    else {
        if (mode == SEARCH_MODE_HEADER)
            body.append(model->header(index));
        else if (mode == SEARCH_MODE_BODY)
            body.append(model->body(index));
        else
            body.append(model->header(index)).append(model->body(index));
    }

    UString data = UString::fromLatin1((const char*)body.constData(), body.length());

    UString searchPattern;
    if (unicode)
        searchPattern = UString::fromLatin1((const char*)pattern.utf16(), pattern.length() * 2);
    else
        searchPattern = pattern;

    int offset = -1;
    while ((offset = (int)data.indexOf(searchPattern, (int)(offset + 1), caseSensitive)) >= 0) {
        UModelIndex parentFileIndex = model->findParentOfType(index, Types::File);
        UString name = model->name(index);
        if (model->parent(index) == parentFileIndex) {
            name = model->name(parentFileIndex) + UString("/") + name;
        }
        else if (parentFileIndex.isValid()) {
            name = model->name(parentFileIndex) + UString("/.../") + name;
        }

        msg((unicode ? UString("Unicode") : UString("ASCII")) + UString(" text \"") + UString(pattern)
            + UString("\" found in ") + name
            + usprintf(" at %s-offset %02Xh", mode == SEARCH_MODE_BODY ? "body" : "header", offset),
            index);
        ret = U_SUCCESS;
    }

    return ret;
}

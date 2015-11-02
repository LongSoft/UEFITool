/* fssfinder.cpp

Copyright (c) 2015, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#include "ffsfinder.h"

FfsFinder::FfsFinder(const TreeModel* treeModel, QObject *parent)
    : QObject(parent), model(treeModel)
{
}

FfsFinder::~FfsFinder()
{
}

void FfsFinder::msg(const QString & message, const QModelIndex & index)
{
    messagesVector.push_back(QPair<QString, QModelIndex>(message, index));
}

QVector<QPair<QString, QModelIndex> > FfsFinder::getMessages() const
{
    return messagesVector;
}

void FfsFinder::clearMessages()
{
    messagesVector.clear();
}

STATUS FfsFinder::findHexPattern(const QModelIndex & index, const QByteArray & hexPattern, const UINT8 mode)
{
    if (!index.isValid())
        return ERR_SUCCESS;

    if (hexPattern.isEmpty())
        return ERR_INVALID_PARAMETER;

    // Check for "all substrings" pattern
    if (hexPattern.count('.') == hexPattern.length())
        return ERR_SUCCESS;

    bool hasChildren = (model->rowCount(index) > 0);
    for (int i = 0; i < model->rowCount(index); i++) {
        findHexPattern(index.child(i, index.column()), hexPattern, mode);
    }

    QByteArray data;
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

    QString hexBody = QString(data.toHex());
    QRegExp regexp = QRegExp(QString(hexPattern), Qt::CaseInsensitive);
    INT32 offset = regexp.indexIn(hexBody);
    while (offset >= 0) {
        if (offset % 2 == 0) {
            msg(tr("Hex pattern \"%1\" found as \"%2\" in %3 at %4-offset %5h")
                .arg(QString(hexPattern))
                .arg(hexBody.mid(offset, hexPattern.length()).toUpper())
                .arg(model->name(index))
                .arg(mode == SEARCH_MODE_BODY ? tr("body") : tr("header"))
                .hexarg(offset / 2),
                index);
        }
        offset = regexp.indexIn(hexBody, offset + 1);
    }

    return ERR_SUCCESS;
}

STATUS FfsFinder::findGuidPattern(const QModelIndex & index, const QByteArray & guidPattern, const UINT8 mode)
{
    if (guidPattern.isEmpty())
        return ERR_INVALID_PARAMETER;

    if (!index.isValid())
        return ERR_SUCCESS;

    bool hasChildren = (model->rowCount(index) > 0);
    for (int i = 0; i < model->rowCount(index); i++) {
        findGuidPattern(index.child(i, index.column()), guidPattern, mode);
    }

    QByteArray data;
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

    QString hexBody = QString(data.toHex());
    QList<QByteArray> list = guidPattern.split('-');
    if (list.count() != 5)
        return ERR_INVALID_PARAMETER;

    QByteArray hexPattern;
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
        return ERR_SUCCESS;

    QRegExp regexp(QString(hexPattern), Qt::CaseInsensitive);
    INT32 offset = regexp.indexIn(hexBody);
    while (offset >= 0) {
        if (offset % 2 == 0) {
            msg(tr("GUID pattern \"%1\" found as \"%2\" in %3 at %4-offset %5h")
                .arg(QString(guidPattern))
                .arg(hexBody.mid(offset, hexPattern.length()).toUpper())
                .arg(model->name(index))
                .arg(mode == SEARCH_MODE_BODY ? tr("body") : tr("header"))
                .hexarg(offset / 2),
                index);
        }
        offset = regexp.indexIn(hexBody, offset + 1);
    }

    return ERR_SUCCESS;
}

STATUS FfsFinder::findTextPattern(const QModelIndex & index, const QString & pattern, const bool unicode, const Qt::CaseSensitivity caseSensitive)
{
    if (pattern.isEmpty())
        return ERR_INVALID_PARAMETER;

    if (!index.isValid())
        return ERR_SUCCESS;

    bool hasChildren = (model->rowCount(index) > 0);
    for (int i = 0; i < model->rowCount(index); i++) {
        findTextPattern(index.child(i, index.column()), pattern, unicode, caseSensitive);
    }

    if (hasChildren)
        return ERR_SUCCESS;

    QString data;
    if (unicode)
        data = QString::fromUtf16((const ushort*)model->body(index).data(), model->body(index).length() / 2);
    else
        data = QString::fromLatin1((const char*)model->body(index).data(), model->body(index).length());

    int offset = -1;
    while ((offset = data.indexOf(pattern, offset + 1, caseSensitive)) >= 0) {
        msg(tr("%1 text \"%2\" found in %3 at offset %4h")
            .arg(unicode ? "Unicode" : "ASCII")
            .arg(pattern)
            .arg(model->name(index))
            .hexarg(unicode ? offset * 2 : offset),
            index);
    }

    return ERR_SUCCESS;
}

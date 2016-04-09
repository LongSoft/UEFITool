/* fitparser.h

Copyright (c) 2015, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHWARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*/

#ifndef FITPARSER_H
#define FITPARSER_H

#include <vector>

#include <QObject>
#include <QModelIndex>
#include <QByteArray>

#include "treemodel.h"
#include "utility.h"
#include "parsingdata.h"
#include "fit.h"
#include "types.h"
#include "treemodel.h"

class TreeModel;

class FitParser
{
public:
    // Default constructor and destructor
    FitParser(TreeModel* treeModel);
    ~FitParser();

    // Returns messages
    std::vector<std::pair<QString, QModelIndex> > getMessages() const;
    // Clears messages
    void clearMessages();

    STATUS parse(const QModelIndex & index, const QModelIndex & lastVtf);
    std::vector<std::vector<QString> > getFitTable() const { return fitTable; }

private:
    TreeModel *model;
    std::vector<std::pair<QString, QModelIndex> > messagesVector;
    QModelIndex lastVtf;
    std::vector<std::vector<QString> > fitTable;
    
    STATUS findFitRecursive(const QModelIndex & index, QModelIndex & found, UINT32 & fitOffset);
    QString fitEntryTypeToQString(UINT8 type);

    // Message helper
    void msg(const QString & message, const QModelIndex &index = QModelIndex());
};

#endif // FITPARSER_H

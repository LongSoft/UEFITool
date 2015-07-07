/* fitparser.h

Copyright (c) 2015, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHWARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*/

#ifndef __FITPARSER_H__
#define __FITPARSER_H__

#include <QObject>
#include <QModelIndex>
#include <QByteArray>
#include <QStringList>
#include <QVector>

#include "basetypes.h"
#include "treemodel.h"
#include "utility.h"
#include "parsingdata.h"
#include "fit.h"

class TreeModel;

class FitParser : public QObject
{
    Q_OBJECT

public:
    // Default constructor and destructor
    FitParser(TreeModel* treeModel, QObject *parent = 0);
    ~FitParser();

    STATUS parse(const QModelIndex & index, const QModelIndex & lastVtf);
    QVector<QPair<FIT_ENTRY, QString> > getFitEntries() const { return fitEntries; }

private:
    TreeModel *model;
    QModelIndex lastVtf;
    QVector<QPair<FIT_ENTRY, QString> > fitEntries;
    
    STATUS findFitRecursive(const QModelIndex & index, QModelIndex & found, UINT32 & fitOffset);
};

#endif

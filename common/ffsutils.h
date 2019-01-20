/* fssreport.h

Copyright (c) 2019, LongSoft. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#ifndef FFSUTILS_H
#define FFSUTILS_H

#include <set>

#include "basetypes.h"
#include "ubytearray.h"
#include "ustring.h"
#include "treemodel.h"

namespace FfsUtils {

USTATUS findFileRecursive(TreeModel *model, const UModelIndex index, const UString & hexPattern, const UINT8 mode, std::set<std::pair<UModelIndex, UModelIndex> > & files);

};

#endif // FFSUTILS_H

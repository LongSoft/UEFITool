/* umodelindex.h

Copyright (c) 2016, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*/

#ifndef UMODELINDEX_H
#define UMODELINDEX_H

#ifdef QT_CORE_LIB
// Use Qt class, if Qt is available
#include <QModelIndex>
#define UModelIndex QModelIndex
#else
// Use own implementation
#endif // QT_CORE_LIB
#endif // UMODELINDEX_H

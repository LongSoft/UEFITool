/* uefiextract_main.cpp

Copyright (c) 2015, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/
#include <QCoreApplication>
#include <QString>
#include <QStringList>
#include <iostream>
#include "uefiextract.h"

int main(int argc, char *argv[])
{
  QCoreApplication a(argc, argv);
  a.setOrganizationName("CodeRush");
  a.setOrganizationDomain("coderush.me");
  a.setApplicationName("UEFIExtract");

  UEFIExtract w;
  UINT8 result = ERR_SUCCESS;
  UINT32 returned = 0;

  if (a.arguments().length() > 33) {
    std::cout << "Too many arguments" << std::endl;
    return 1;
  }

  if (a.arguments().length() > 2 ) {
    if (w.init(a.arguments().at(1)))
      return 1;

    if (a.arguments().length() == 3) {
      result = w.extract(a.arguments().at(2));
      if (result)
        return 2;
    }
    else {
      for (int i = 3; i < a.arguments().length(); i++) {
        result = w.extract(a.arguments().at(2), a.arguments().at(i));
        if (result)
          returned |= (1 << (i - 1));
      }
      return returned;
    }
    
  }
  else {
    std::cout << "UEFIExtract 0.4.4" << std::endl << std::endl <<
    "Usage: uefiextract imagefile dumpdir [FileGUID_1 FileGUID_2 ... FileGUID_31]" << std::endl <<
    "Returned value is a bit mask where 0 on position N meant File with GUID_N was found and unpacked, 1 otherwise" << std::endl;
    return 1;
  }
}

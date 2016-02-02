/* uefiextract_main.cpp

Copyright (c) 2016, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/
#include <QCoreApplication>
#include <QVector>
#include <QPair>
#include <QString>
#include <QFileInfo>

#include <iostream>

#include "../common/ffsparser.h"
#include "ffsdumper.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    a.setOrganizationName("CodeRush");
    a.setOrganizationDomain("coderush.me");
    a.setApplicationName("UEFIExtract");

    if (a.arguments().length() > 32) {
        std::cout << "Too many arguments" << std::endl;
        return 1;
    }

    if (a.arguments().length() > 1) {
        QString path = a.arguments().at(1);
        QFileInfo fileInfo(path);
        if (!fileInfo.exists())
            return ERR_FILE_OPEN;

        QFile inputFile;
        inputFile.setFileName(path);
        if (!inputFile.open(QFile::ReadOnly))
            return ERR_FILE_OPEN;

        QByteArray buffer = inputFile.readAll();
        inputFile.close();

        TreeModel model;
        FfsParser ffsParser(&model);
        STATUS result = ffsParser.parse(buffer);
        if (result)
            return result;

        QVector<QPair<QString, QModelIndex> > messages = ffsParser.getMessages();
        QPair<QString, QModelIndex> msg;
        foreach(msg, messages) {
            std::cout << msg.first.toLatin1().constData() << std::endl;
        }

        FfsDumper ffsDumper(&model);

        if (a.arguments().length() == 2) {
            return (ffsDumper.dump(model.index(0, 0), fileInfo.fileName().append(".dump")) != ERR_SUCCESS);
        }
        else {
            UINT32 returned = 0;
            for (int i = 2; i < a.arguments().length(); i++) {
                result = ffsDumper.dump(model.index(0, 0), fileInfo.fileName().append(".dump"), a.arguments().at(i));
                if (result)
                    returned |= (1 << (i - 1));
            }
            return returned;
        }
    }
    else {
        std::cout << "UEFIExtract 0.10.7" << std::endl << std::endl
                  << "Usage: UEFIExtract imagefile [FileGUID_1 FileGUID_2 ... FileGUID_31]" << std::endl
                  << "Return value is a bit mask where 0 at position N means that file with GUID_N was found and unpacked, 1 otherwise" << std::endl;
        return 1;
    }
}

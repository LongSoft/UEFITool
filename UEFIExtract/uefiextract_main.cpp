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
        STATUS result = ffsParser.parseImageFile(buffer, model.index(0, 0));
        if (result)
            return result;

        QVector<QPair<QString, QModelIndex> > messages = ffsParser.getMessages();
        QPair<QString, QModelIndex> msg;
        foreach(msg, messages) {
            std::cout << msg.first.toLatin1().constData() << std::endl;
        }

        FfsDumper ffsDumper(&model);
        return ffsDumper.dump(model.index(0, 0), fileInfo.fileName().append(".dump"));
    }
    else {
        std::cout << "UEFIExtract 0.10.0" << std::endl << std::endl
                  << "Usage: uefiextract imagefile" << std::endl;
        return 1;
    }
}

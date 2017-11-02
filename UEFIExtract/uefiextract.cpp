/* uefiextract.cpp

Copyright (c) 2014, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#include "uefiextract.h"

UEFIExtract::UEFIExtract(QObject *parent) :
    QObject(parent)
{
    ffsEngine = new FfsEngine(this);
}

UEFIExtract::~UEFIExtract()
{
    delete ffsEngine;
}

UINT8 UEFIExtract::init(const QString & path)
{
	fileInfo = QFileInfo(path);

	if (!fileInfo.exists())
		return ERR_FILE_OPEN;

	QFile inputFile;
	inputFile.setFileName(path);

	if (!inputFile.open(QFile::ReadOnly))
		return ERR_FILE_OPEN;

	QByteArray buffer = inputFile.readAll();
	inputFile.close();

	return ffsEngine->parseImageFile(buffer);
}

UINT8 UEFIExtract::extract(QString path, QString guid)
{
    return ffsEngine->dump(ffsEngine->treeModel()->index(0, 0), path, guid);
}

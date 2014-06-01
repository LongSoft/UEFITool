#include "wrapper.h"

Wrapper::Wrapper(void)
{
    ffsEngine = new FfsEngine();
}

Wrapper::~Wrapper(void)
{
    delete ffsEngine;
}

/* General stuff */

UINT8 Wrapper::fileOpen(QString path, QByteArray & buf)
{
    QFileInfo fileInfo = QFileInfo(path);

    if (!fileInfo.exists())
        return ERR_FILE_NOT_FOUND;

    QFile inputFile;
    inputFile.setFileName(path);

    if (!inputFile.open(QFile::ReadOnly))
        return ERR_FILE_OPEN;

    buf.clear();

    buf.append(inputFile.readAll());
    inputFile.close();

    return ERR_SUCCESS;
}

UINT8 Wrapper::fileWrite(QString path, QByteArray & buf)
{
    QFileInfo fileInfo = QFileInfo(path);

    if (fileInfo.exists())
        return ERR_FILE_EXISTS;

    QFile writeFile;
    writeFile.setFileName(path);

    if (!writeFile.open(QFile::WriteOnly))
        return ERR_FILE_OPEN;

    if(writeFile.write(buf) != buf.size())
        return ERR_FILE_WRITE;

    writeFile.close();

    return ERR_SUCCESS;
}

BOOLEAN Wrapper::fileExists(QString path)
{
    QFileInfo fileInfo = QFileInfo(path);

    return fileInfo.exists();
}

UINT8 Wrapper::dirCreate(QString path)
{
    QDir dir;
    if (dir.cd(path))
        return ERR_DIR_ALREADY_EXIST;

    if (!dir.mkpath(path))
        return ERR_DIR_CREATE;

    return ERR_SUCCESS;
}

BOOLEAN Wrapper::dirExists(QString path)
{
    QDir dir(path);

    return dir.exists();
}

QString Wrapper::pathConcatenate(QString path, QString filename)
{
    return QDir(path).filePath(filename);
}

UINT8 Wrapper::getFolderListByExt(QStringList & list, QString path, QString stringEnd)
{
    QDirIterator di(path);

    while (di.hasNext()) {
        if(di.next().endsWith(stringEnd))
            list.append(di.filePath());
    }

    if(!list.size())
        return ERR_ITEM_NOT_FOUND;

    return ERR_SUCCESS;
}

BOOLEAN Wrapper::isValidKextDir(QString path)
{
    QDir dir;
    QFileInfo plist;

    dir.setPath(path);
    dir = dir.filePath("Contents");
    plist.setFile(dir,"Info.plist");
    dir = dir.filePath("MacOS");

    if (!dir.exists())
        return FALSE;

    if (!plist.exists())
        return FALSE;

    return TRUE;
}

UINT32 Wrapper::getUInt32(QByteArray & buf, UINT32 start, bool fromBE)
{
    int i;
    UINT32 tmp = 0;
    for(i = start; i < (start+4); i++)
    {
         tmp = (tmp << 8) + buf.at(i);
    }
    if (fromBE)
        return qFromBigEndian(tmp);
    return tmp;
}

UINT32 Wrapper::getDateTime()
{
    QDateTime dateTime = QDateTime::currentDateTime();
    return dateTime.toTime_t();
}

/* Specific stuff */

UINT8 Wrapper::dumpSectionByName(QString name, QByteArray & buf, UINT8 mode)
{
    QModelIndex index;
    QModelIndex rootIndex = ffsEngine->treeModel()->index(0, 0);

    index = ffsEngine->findIndexByName(rootIndex, name);
    if(!index.isValid())
        return ERR_ITEM_NOT_FOUND;

    ffsEngine->extract(index, buf, mode);

    return ERR_SUCCESS;
}

UINT8 Wrapper::dumpSectionByGUID(QString guid, QByteArray & buf, UINT8 mode)
{
    QModelIndex index;
    QModelIndex rootIndex = ffsEngine->treeModel()->index(0, 0);

    index = ffsEngine->findIndexByName(rootIndex, guid);
    if(!index.isValid())
        return ERR_ITEM_NOT_FOUND;

    ffsEngine->extract(index, buf, mode);

    return ERR_SUCCESS;
}

UINT8 Wrapper::parseBIOSFile(QByteArray & buf)
{
    UINT8 ret = ffsEngine->parseImageFile(buf);
    if (ret)
        return ret;

    return ERR_SUCCESS;
}

UINT8 Wrapper::getDSDTfromAMI(QByteArray & in, QByteArray & out)
{
    INT32 tmp = 0;
    UINT32 size_start = 0;

    UINT32 start = 0;
    UINT32 size = 0;
    static const QString MAGIC = "DSDT";

    tmp = in.indexOf(MAGIC, start);
    if (tmp < 0)
        return ERR_ITEM_NOT_FOUND;

    start = tmp;
    size_start = start + MAGIC.size();

    size = getUInt32(in, size_start, true);

    if((start+size) > in.size())
        return ERR_INVALID_SECTION;

    out.clear();
    /* ToDo: Why is DSDT one byte too long */
    out = in.mid(start, size+1);

    if(out.size() != size+1)
        return ERR_OUT_OF_MEMORY;

    return ERR_SUCCESS;
}

UINT8 Wrapper::kext2ffs(QString basename, QString GUID, QByteArray plist, QByteArray inputbinary, QByteArray & output)
{
    QString name;
    QString version;
    QByteArray input;
    UINT8 null = 0;
    KextConvert *kext = new KextConvert();

    input.clear();

    if(!plist.isEmpty()) {
        kext->getInfoFromPlist(plist, name, version);
        name = basename;
        version = "0.12a";
        name.sprintf("%s.Rev-%s", qPrintable(name), qPrintable(version));

        //convertPlistToBinary();
        input.append(plist);
        input.append(null);
        input.append(inputbinary);
    }
    else {
        name = basename;
        input = inputbinary;
    }

    return kext->createFFS(name, GUID, input, output);
}


UINT8 Wrapper::efi2ffs(QString basename, QString GUID, QByteArray inputbinary, QByteArray & output)
{
    KextConvert *kext = new KextConvert();
    return ERR_SUCCESS;
}

UINT8 Wrapper::ozm2ffs(QByteArray inputbinary, QByteArray & output)
{
    QString fixedGUID = "99F2839C-57C3-411E-ABC3-ADE5267D960D"; //OzmosisDefaults.plist
    QString fixedName = "OzmosisDefaults";
    KextConvert *kext = new KextConvert();

    return kext->createFFS(fixedName, fixedGUID, inputbinary, output);
}


#include <QtEndian>
#include <QDirIterator>
#include <QDateTime>
#include <plist/Plist.hpp>
#include "ffs/kextconvert.h"
#include "dsdt2bios/Dsdt2Bios.h"
#include "../ffs.h"
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
    QFileInfo fileInfo(path);

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
    QFileInfo fileInfo(path);

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

UINT32 Wrapper::getDateTime()
{
    QDateTime dateTime = QDateTime::currentDateTime();
    return dateTime.toTime_t();
}

UINT16 Wrapper::getUInt16(QByteArray & buf, UINT32 start, bool fromBE)
{
    UINT16 tmp = 0;

    tmp = (tmp << 8) + buf.at(start+0);
    tmp = (tmp << 8) + buf.at(start+1);

    if(fromBE)
        return qFromBigEndian(tmp);
    else
        return tmp;
}

UINT32 Wrapper::getUInt32(QByteArray & buf, UINT32 start, bool fromBE)
{
    UINT32 tmp = 0;

    tmp = (tmp << 8) + buf.at(start+0);
    tmp = (tmp << 8) + buf.at(start+1);
    tmp = (tmp << 8) + buf.at(start+2);
    tmp = (tmp << 8) + buf.at(start+3);

    if(fromBE)
        return qFromBigEndian(tmp);
    else
        return tmp;
}

/* Specific stuff */

QModelIndex Wrapper::getRootIndex() {
    return ffsEngine->treeModel()->index(0, 0);
}

UINT8 Wrapper::insert(QModelIndex & index, QByteArray & object, UINT8 mode) {
    return ffsEngine->insert(index, object, mode);
}

UINT8 Wrapper::replace(QModelIndex & index, QByteArray & object, UINT8 mode) {
    return ffsEngine->replace(index, object, mode);
}

UINT8 Wrapper::reconstructImageFile(QByteArray & out) {
    return ffsEngine->reconstructImageFile(out);
}

UINT8 Wrapper::getGUIDfromFile(QByteArray object, QString & name)
{
    QByteArray header;
    EFI_GUID* guid;
    header = object.left(sizeof(EFI_GUID));
    guid = (EFI_GUID*)(header.constData());

    // Get info
    name = guidToQString(*guid);
    return ERR_SUCCESS;
}

UINT8 Wrapper::findFileByGUID(const QModelIndex index, const QString guid, QModelIndex & result)
{
    UINT8 ret;

    if (!index.isValid()) {
        return ERR_INVALID_SECTION;
    }

    if(!ffsEngine->treeModel()->nameString(index).compare(guid)) {
        result = index;
        return ERR_SUCCESS;
    }

    for (int i = 0; i < ffsEngine->treeModel()->rowCount(index); i++) {
        QModelIndex childIndex = index.child(i, 0);
        ret = findFileByGUID(childIndex, guid, result);
        if(result.isValid() && (ret == ERR_SUCCESS))
            return ERR_SUCCESS;
    }
    return ERR_ITEM_NOT_FOUND;
}

UINT8 Wrapper::findSectionByIndex(const QModelIndex index, UINT8 type, QModelIndex & result)
{
    UINT8 ret;

    if (!index.isValid()) {
        return ERR_INVALID_SECTION;
    }

    if(ffsEngine->treeModel()->subtype(index) == type) {
        result = index;
        return ERR_SUCCESS;
    }

    for (int i = 0; i < ffsEngine->treeModel()->rowCount(index); i++) {
        QModelIndex childIndex = index.child(i, 0);
        ret = findSectionByIndex(childIndex, type, result);
        if(result.isValid() && (ret == ERR_SUCCESS))
            return ERR_SUCCESS;
    }
    return ERR_ITEM_NOT_FOUND;
}

UINT8 Wrapper::dumpFileByGUID(QString guid, QByteArray & buf, UINT8 mode)
{
    UINT8 ret;
    QModelIndex result;
    QModelIndex rootIndex = ffsEngine->treeModel()->index(0, 0);

    ret = findFileByGUID(rootIndex, guid, result);
    if(ret)
        return ERR_ITEM_NOT_FOUND;

    ret = ffsEngine->extract(result, buf, mode);
    if(ret)
        return ERR_ERROR;

    return ERR_SUCCESS;
}

UINT8 Wrapper::dumpSectionByGUID(QString guid, UINT8 type, QByteArray & buf, UINT8 mode)
{
    UINT8 ret;
    QModelIndex resultFile;
    QModelIndex resultSection;
    QModelIndex rootIndex = ffsEngine->treeModel()->index(0, 0);

    ret = findFileByGUID(rootIndex, guid, resultFile);
    if(ret)
        return ERR_ITEM_NOT_FOUND;

    ret = findSectionByIndex(resultFile, type, resultSection);
    if(ret)
        return ERR_ITEM_NOT_FOUND;

    ret = ffsEngine->extract(resultSection, buf, mode);
    if(ret)
        return ERR_ERROR;

    return ERR_SUCCESS;
}

UINT8 Wrapper::getLastSibling(QModelIndex index, QModelIndex & result)
{
    int lastRow, column;

    column = 0;
    lastRow = ffsEngine->treeModel()->rowCount(index.parent());
    lastRow--;

    result = index.sibling(lastRow, column);
    return ERR_SUCCESS;
}

UINT8 Wrapper::parseBIOSFile(QByteArray & buf)
{
    UINT8 ret = ffsEngine->parseImageFile(buf);
    if (ret)
        return ret;

    return ERR_SUCCESS;
}

UINT8 Wrapper::getDSDTfromAMI(QByteArray in, QByteArray & out)
{
    UINT8 ret;
    UINT32 start = 0;
    UINT32 size = 0;

    Dsdt2Bios d2b;

    ret = d2b.getDSDTFromAmi(in, start, size);
    if(ret)
        return ret;

    out.clear();

    out = in.mid(start, size);

    if(out.size() != size)
        return ERR_OUT_OF_MEMORY;

    return ERR_SUCCESS;
}

UINT8 Wrapper::dsdt2bios(QByteArray amiboardinfo, QByteArray dsdt, QByteArray & out)
{
    UINT8 ret;
    UINT32 reloc_padding;
    UINT32 offset, size;

    Dsdt2Bios d2b;

    reloc_padding = 0;

    ret = d2b.getDSDTFromAmi(amiboardinfo, offset, size);
    if(ret)
        return ret;

    ret = d2b.injectDSDTIntoAmi(amiboardinfo, dsdt, offset, size, out, reloc_padding);
    if(ret == ERR_RELOCATION && (reloc_padding != 0)) {
        /* Re-running with other reloc_padding */
        ret = d2b.injectDSDTIntoAmi(amiboardinfo, dsdt, offset, size, out, reloc_padding);
    }

    return ret;
}

UINT8 Wrapper::getInfoFromPlist(QByteArray plist, QString & name, QByteArray & out)
{
    std::vector<char> data;
    static const std::string nameIdentifier = "CFBundleName";
    static const std::string versionIdentifier = "CFBundleShortVersionString";

    QString plistName;
    QString plistVersion;

    std::map<std::string, boost::any> dict;
    Plist::readPlist(plist.data(), plist.size(), dict);

    /* Just assumes the entry's there, otherwise => CRASH */
    plistName = boost::any_cast<const std::string&>(dict.find(nameIdentifier)->second).c_str();
    plistVersion = boost::any_cast<const std::string&>(dict.find(versionIdentifier)->second).c_str();

    if(plistName.isEmpty()) {
        printf("ERROR: CFBundleName in plist is blank. Aborting!\n");
        return ERR_ERROR;
    }

    if(plistVersion.isEmpty()) {
        plistVersion = "nA"; // set to NonAvailable
    }

    name.sprintf("%s.Rev-%s", qPrintable(plistName), qPrintable(plistVersion));

    // Assign new value for CFBundleName
    dict[nameIdentifier] = std::string(name.toUtf8().constData());

    Plist::writePlistBinary(data, dict);

    out.clear();
    out.append(data.data(), data.size());

    return ERR_SUCCESS;
}

UINT8 Wrapper::parseKextDirectory(QString input, QList<kextEntry> & kextList)
{
    UINT32 GUIDindex;
    UINT32 GUIDindexCount;

    kextEntry mKextEntry;
    QString basename;
    QDir dir;

    QFileInfo binaryPath;
    QFileInfo plistPath;

    QDirIterator di(input);

    const static QString kextExtension = ".kext";
    const static QString ozmDefaultsFilename = "OzmosisDefaults.plist";
    const static QString ozmPlistGUID = "99F2839C-57C3-411E-ABC3-ADE5267D960D";
    const static QString ozmPlistPath = pathConcatenate(input, ozmDefaultsFilename);
    const static QString kextGUID = "DADE100%1-1B31-4FE4-8557-26FCEFC78275"; // %1 placeholder

    GUIDindexCount = 6;

    if(fileExists(ozmPlistPath)) {
        mKextEntry.basename = "OzmosisDefaults";
        mKextEntry.binaryPath = ozmPlistPath; // yes, Plist handled as binary
        mKextEntry.plistPath = "";
        mKextEntry.GUID = ozmPlistGUID;
        mKextEntry.filename = "OzmosisDefaults.ffs";

        kextList.append(mKextEntry);
    }

    // Check all folders in input-dir
    while (di.hasNext()) {

        if(!di.next().endsWith(kextExtension))
            continue;

        basename = di.fileName().left(di.fileName().size()-kextExtension.size());

        dir.setPath(di.filePath());
        dir = dir.filePath("Contents");
        plistPath.setFile(dir,"Info.plist");
        dir = dir.filePath("MacOS");
        binaryPath.setFile(dir, basename);

        if (!dir.exists()) {
            printf("ERROR: Kext-dir invalid: %s/Contents/MacOS/ missing!\n", qPrintable(di.fileName()));
            return ERR_ERROR;
        }

        if (!plistPath.exists()) {
            printf("ERROR: Kext-dir invalid: %s/Contents/Info.plist missing!\n", qPrintable(di.fileName()));
            return ERR_ERROR;
        }

        if (!binaryPath.exists()) {
            printf("ERROR: Kext-dir invalid: %s/Contents/MacOS/%s missing!\n",
                   qPrintable(di.fileName()), qPrintable(basename));
            return ERR_ERROR;
        }

        if(GUIDindexCount > 0xF) {
            printf("ERROR: Reached maximum Kext-Count! Ignoring the rest...\n");
            break;
        }

        if(!di.fileName().compare("FakeSMC.kext"))
            GUIDindex = 1;
        else if(!di.fileName().compare("Disabler.kext"))
            GUIDindex = 2;
        else if(!di.fileName().compare("Injector.kext"))
            GUIDindex = 3;
        else
            GUIDindex = GUIDindexCount++;

        mKextEntry.basename = basename;
        mKextEntry.binaryPath = binaryPath.filePath();
        mKextEntry.plistPath = plistPath.filePath();
        mKextEntry.GUID = kextGUID.arg(GUIDindex, 0, 16);
        mKextEntry.filename = basename + ".ffs";
        kextList.append(mKextEntry);
    }

    return ERR_SUCCESS;
}

UINT8 Wrapper::convertKexts(kextEntry entry, QByteArray & out)
{
    UINT8 ret;
    UINT8 nullterminator = 0;

    QString sectionName;

    QByteArray plist;
    QByteArray plistBinary;
    QByteArray kextbinary;
    QByteArray inputBinary;

    KextConvert *kext = new KextConvert();

    // Convert Kext to FFS
    inputBinary.clear();

    if(!entry.basename.compare("OzmosisDefaults")) {
        sectionName = entry.basename;
    }
    else {
        // If NOT OzmosisDefaults => check plist path
        ret = fileOpen(entry.plistPath, plist);
        if(ret) {
            printf("ERROR: Open failed: %s\n", qPrintable(entry.plistPath));
            return ERR_ERROR;
        }

        ret = getInfoFromPlist(plist, sectionName, plistBinary);
        if(ret) {
            printf("ERROR: Failed to get values/convert Info.plist\n");
            return ERR_ERROR;
        }

        inputBinary.append(plistBinary);
        inputBinary.append(nullterminator); // need between binary plist + kextbinary
    }

    ret = fileOpen(entry.binaryPath, kextbinary);
    if(ret) {
        printf("ERROR: Open failed: %s\n", qPrintable(entry.binaryPath));
        return ERR_ERROR;
    }
    inputBinary.append(kextbinary);

    out.clear();
    ret = kext->createFFS(sectionName, entry.GUID, inputBinary, out);
    if(ret) {
        printf("ERROR: KEXT2FFS failed on '%s'\n", qPrintable(entry.basename));
        return ERR_ERROR;
    }

    return ERR_SUCCESS;
}

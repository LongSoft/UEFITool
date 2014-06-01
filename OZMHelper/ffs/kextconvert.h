#include <QString>
#include <QFileInfo>
#include <QByteArray>

#ifndef KEXTCONVERT_H
#define KEXTCONVERT_H

/* Kext

FakeSMC.kext 1
Disabler.kext 2
Injector.kext 3
Other.kext 6 - infinite

BASE GUID "DADE100$2-1B31-4FE4-8557-26FCEFC78275"

-- EFI --

from 20 (0x14) ++
BASE GUID "DADE10$2-1B31-4FE4-8557-26FCEFC78275"

-- OZM PLIST --

STATIC GUID "99F2839C-57C3-411E-ABC3-ADE5267D960D"

*/

class KextConvert
{
public:
    KextConvert();
    ~KextConvert();

    UINT8 createFFS(QString name, QString GUID, QByteArray inputbinary, QByteArray & output);
    BOOLEAN getInfoFromPlist(QByteArray plist, QString & CFBundleName, QString & CFBundleShortVersionString);

private:
};

#endif // KEXTCONVERT_H

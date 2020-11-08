#ifndef BGMANIFESTPARSER_H
#define BGMANIFESTPARSER_H


#include "utility.h"
#include "ffsparser.h"


enum MANIFEST_TYPE
{
    UNKNOWN,
    KEYM,
    IBBM
};

class BGManifestParser
{
public:
    BGManifestParser();

    virtual ~BGManifestParser()
    {

    }
    //pure virtual
    virtual USTATUS  ParseManifest(const UByteArray & keyManifest, const UINT32 localOffset, const UModelIndex & parent,
                  UString & info, UINT32 &realSize, UString &SecurityInfo, TreeModel* TreeMod,UByteArray bgKMHash) = 0;
    virtual MANIFEST_TYPE GetManifestType();
protected:
    MANIFEST_TYPE manifestType;
    UINT8         manifestVersion;
    FfsParser*    ffsParser;
};

class BGKeyManifestParser : public BGManifestParser
{
public:

    BGKeyManifestParser();
    ~BGKeyManifestParser() { }

    virtual USTATUS ParseManifest(const UByteArray & keyManifest, const UINT32 localOffset, const UModelIndex & parent,
                  UString & info, UINT32 &realSize, UString &SecurityInfo, TreeModel* TreeMod, UByteArray bgKMHash);

private:

};

class BGKeyManifestParserIcelake : public BGManifestParser
{
public:
    BGKeyManifestParserIcelake();

    ~BGKeyManifestParserIcelake() { }
    virtual USTATUS ParseManifest(const UByteArray & keyManifest, const UINT32 localOffset, const UModelIndex & parent,
                  UString & info, UINT32 &realSize, UString &SecurityInfo, TreeModel* TreeMod, UByteArray bgKMHash);

private:
};

class BGIBBManifestParser : public BGManifestParser
{
public:
    BGIBBManifestParser(FfsParser* parser);

    ~BGIBBManifestParser() { }
    virtual USTATUS ParseManifest(const UByteArray & bootPolicy, const UINT32 localOffset, const UModelIndex & parent,
                  UString & info, UINT32 &realSize, UString &SecurityInfo, TreeModel* TreeMod, UByteArray bgKMHash);

private:

};

class BGIBBManifestParserIcelake : public BGManifestParser
{
public:
    BGIBBManifestParserIcelake(FfsParser* parser);

    ~BGIBBManifestParserIcelake() { }
    virtual USTATUS ParseManifest(const UByteArray & bootPolicy, const UINT32 localOffset, const UModelIndex & parent,
                  UString & info, UINT32 &realSize, UString &SecurityInfo, TreeModel* TreeMod, UByteArray bgKMHash);

private:

};

#endif // BGMANIFESTPARSER_H

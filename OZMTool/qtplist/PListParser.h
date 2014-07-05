#ifndef PLIST_PARSER_H
#define PLIST_PARSER_H

#include <QtCore>
#include <QDomElement>

class PListParser {
public:
	static QVariant parsePList(QIODevice *device);
    static QVariant parsePList(QByteArray & file);
private:
	static QVariant parseElement(const QDomElement &e);
	static QVariantList parseArrayElement(const QDomElement& node);
	static QVariantMap parseDictElement(const QDomElement& element);
};
#endif

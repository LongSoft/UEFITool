#ifndef PLIST_SERIALIZER_H
#define PLIST_SERIALIZER_H

#include <QtCore>
#include <QDomElement>
#include <QDomDocument>

class PListSerializer {
public:
	static QString toPList(QVariant &variant);
private:
	static QDomElement serializeElement(QDomDocument &doc, const QVariant &variant);
	static QDomElement serializeMap(QDomDocument &doc, const QVariantMap &map);
	static QDomElement serializeList(QDomDocument &doc, const QVariantList &list);
};
#endif


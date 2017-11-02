This is a PList (Mac OS X property list) parser for Qt.  See http://en.wikipedia.org/wiki/Property_list for more information about the format.

Basically, it takes a QIODevice (generally a file), and returns you a QVariant with all the values in the PList.

Suppose this is your plist file:

<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>SomeKey</key>
	<string>Value1</string>
	<key>MyOtherKey</key>
	string>Value2</string>
</dict>
</plist>

This would get converted into a QVariantMap, with keys SomeKey and MyOtherKey, with respective values Value1 and Value2.  If your PList has arrays or dictionaries inside, they are converted to QVariantLists and QVariantMaps.

It can also do the conversion the other way: if you have a QVariant, QVariantList, or QVariantMap, it can generate a PList file out of it, using PListSerializer.

That's it!

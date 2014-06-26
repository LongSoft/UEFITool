//
//   PlistCpp Property List (plist) serialization and parsing library.
//
//   https://github.com/animetrics/PlistCpp
//   
//   Copyright (c) 2011 Animetrics Inc. (marc@animetrics.com)
//   
//   Permission is hereby granted, free of charge, to any person obtaining a copy
//   of this software and associated documentation files (the "Software"), to deal
//   in the Software without restriction, including without limitation the rights
//   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//   copies of the Software, and to permit persons to whom the Software is
//   furnished to do so, subject to the following conditions:
//   
//   The above copyright notice and this permission notice shall be included in
//   all copies or substantial portions of the Software.
//   
//   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//   THE SOFTWARE.

#include "Plist.hpp"
#include <boost/locale/encoding_utf.hpp>
#include <list>
#include <sstream>
#include "base64.hpp"
#include "pugixml.hpp"

namespace Plist {

		struct PlistHelperData
		{
			public:

				// binary helper data
				std::vector<int32_t> _offsetTable;
				std::vector<unsigned char> _objectTable;
				int32_t _offsetByteSize;
				int64_t _offsetTableOffset;

				int32_t _objRefSize;
				int32_t _refCount;
		};

		void writePlistBinary(
				PlistHelperData& d,
				const boost::any& message);

		void writePlistXML(
				pugi::xml_document& doc,
				const boost::any& message);

		// helper functions

		// msvc <= 2005 doesn't have std::vector::data() method

		template<typename T>
		T* vecData(std::vector<T>& vec)
		{
			return (vec.size() > 0) ? &vec[0] : 0;
			// if(vec.size() > 0)
			//		return &vec[0];
			// else
			//		throw Error("vecData trying to get pointer to empty std::vector");
		}

		template<typename T>
		const T* vecData(const std::vector<T>& vec)
		{
			return (vec.size() > 0) ? &vec[0] : 0;
			// if(vec.size() > 0)
			//		return &vec[0];
			// else
			//		throw Error("vecData trying to get pointer to empty std::vector");
		}

		// xml helper functions

		template<typename T>
			std::string stringFromValue(const T& value);
		template<typename T>
			void writeXMLSimpleNode(pugi::xml_node& node, const char* name, const boost::any& obj);

		// xml parsing

		dictionary_type parseDictionary(pugi::xml_node& node);
		array_type parseArray(pugi::xml_node& node);
		std::vector<char> base64Decode(const char* data);
		void base64Encode(std::string& dataEncoded, const std::vector<char>& data);
		Date parseDate(pugi::xml_node& node);
		boost::any parse(pugi::xml_node& doc);

		// xml writing

		void writeXMLArray(pugi::xml_node& node, const array_type& array);
		void writeXMLDictionary(pugi::xml_node& node, const dictionary_type& message);
		void writeXMLNode(pugi::xml_node& node, const boost::any& obj);

		// binary helper functions

		template <typename IntegerType>
			IntegerType bytesToInt(const unsigned char* bytes, bool littleEndian);
		double bytesToDouble(const unsigned char* bytes, bool littleEndian);
		std::vector<unsigned char> doubleToBytes(double val, bool littleEndian);
		template<typename IntegerType>
			std::vector<unsigned char> intToBytes(IntegerType val, bool littleEndian);
		std::vector<unsigned char> getRange(const unsigned char* origBytes, int64_t index, int64_t size);
		std::vector<unsigned char> getRange(const std::vector<unsigned char>& origBytes, int64_t index, int64_t size);
		std::vector<char> getRange(const char* origBytes, int64_t index, int64_t size);

		// binary parsing

		boost::any parseBinary(const PlistHelperData& d, int objRef);
		dictionary_type  parseBinaryDictionary(const PlistHelperData& d, int objRef);
		array_type  parseBinaryArray(const PlistHelperData& d, int objRef);
		std::vector<int32_t> getRefsForContainers(const PlistHelperData& d, int objRef);
		int64_t parseBinaryInt(const PlistHelperData& d, int headerPosition, int& intByteCount);
		double parseBinaryReal(const PlistHelperData& d, int headerPosition);
		Date parseBinaryDate(const PlistHelperData& d, int headerPosition);
		bool parseBinaryBool(const PlistHelperData& d, int headerPosition);
		std::string parseBinaryString(const PlistHelperData& d, int objRef);
		std::string parseBinaryUnicode(const PlistHelperData& d, int headerPosition);
		data_type parseBinaryByteArray(const PlistHelperData& d, int headerPosition);
		std::vector<unsigned char> regulateNullBytes(const std::vector<unsigned char>& origBytes, unsigned int minBytes);
		void parseTrailer(PlistHelperData& d, const std::vector<unsigned char>& trailer);
		void parseOffsetTable(PlistHelperData& d, const std::vector<unsigned char>& offsetTableBytes);
		int32_t getCount(const PlistHelperData& d, int bytePosition, unsigned char headerByte, int& startOffset);

		// binary writing

		int countAny(const boost::any& object);
		int countDictionary(const dictionary_type& dictionary);
		int countArray(const array_type& array);
		std::vector<unsigned char> writeBinaryDictionary(PlistHelperData& d, const dictionary_type& dictionary);
		std::vector<unsigned char> writeBinaryArray(PlistHelperData& d, const array_type& array);
		std::vector<unsigned char> writeBinaryByteArray(PlistHelperData& d, const data_type& byteArray);
		std::vector<unsigned char> writeBinaryInteger(PlistHelperData& d, int64_t value, bool write);
		std::vector<unsigned char> writeBinaryBool(PlistHelperData& d, bool value);
		std::vector<unsigned char> writeBinaryDate(PlistHelperData& d, const Date& date);
		std::vector<unsigned char> writeBinaryDouble(PlistHelperData& d, double value);
		std::vector<unsigned char> writeBinary(PlistHelperData& d, const boost::any& obj);
		std::vector<unsigned char> writeBinaryString(PlistHelperData& d, const std::string& value, bool head);

		inline bool hostLittleEndian()
		{
			union { uint32_t x; uint8_t c[4]; } u;
			u.x = 0xab0000cd;
			return u.c[0] == 0xcd;
		}

} // namespace Plist

namespace Plist {

template<typename T>
void writeXMLSimpleNode(pugi::xml_node& node, const char* name, const boost::any& obj)
{
	pugi::xml_node newNode;
	newNode = node.append_child(name);
	newNode.append_child(pugi::node_pcdata).set_value(stringFromValue(boost::any_cast<const T&>(obj)).c_str());
}

void writeXMLNode(pugi::xml_node& node, const boost::any& obj)
{
	using namespace std;

	const std::type_info &objType = obj.type();

	if(objType == typeid(int32_t))
		writeXMLSimpleNode<int32_t>(node, "integer", obj);
	else if(objType == typeid(int64_t))
		writeXMLSimpleNode<int64_t>(node, "integer", obj);
	else if(objType == typeid(long))
		writeXMLSimpleNode<long>(node, "integer", obj);
	else if(objType == typeid(short))
		writeXMLSimpleNode<short>(node, "integer", obj);
	else if(objType == typeid(dictionary_type))
		writeXMLDictionary(node, boost::any_cast<const dictionary_type&>(obj));
	else if(objType == typeid(string_type))
		writeXMLSimpleNode<string_type>(node, "string", obj);
	else if(objType == typeid(array_type))
		writeXMLArray(node, boost::any_cast<const array_type&>(obj));
	else if(objType == typeid(data_type))
	{
		string dataEncoded;
		base64Encode(dataEncoded, boost::any_cast<const data_type&>(obj));
		writeXMLSimpleNode<string>(node, "data", dataEncoded);
	}
	else if(objType == typeid(double))
		writeXMLSimpleNode<double>(node, "real", obj);
	else if(objType == typeid(float))
		writeXMLSimpleNode<float>(node, "real", obj);
	else if(objType == typeid(Date))
		writeXMLSimpleNode<string>(node, "date", boost::any_cast<const Date&>(obj).timeAsXMLConvention());
	else if(objType == typeid(bool))
	{
		bool value = boost::any_cast<const bool&>(obj);
		node.append_child(value ? "true" : "false");
	}
	else
		throw Error((string("Plist Error: Can't serialize type ") + objType.name()).c_str());
}

void writeXMLArray(
		pugi::xml_node& node,
		const array_type& array)
{
	using namespace std;
	pugi::xml_node newNode = node.append_child("array");
	for(array_type::const_iterator it = array.begin();
			it != array.end();
			++it)
		writeXMLNode(newNode, *it);
}

void writeXMLDictionary(
		pugi::xml_node& node,
		const dictionary_type& message)
{
	using namespace std;
	pugi::xml_node newNode = node.append_child("dict");
	for(dictionary_type::const_iterator it = message.begin();
			it != message.end();
			++it)
	{
		pugi::xml_node keyNode = newNode.append_child("key");
		keyNode.append_child(pugi::node_pcdata).set_value(it->first.c_str());
		writeXMLNode(newNode, it->second);
	}

}

void writePlistXML(
		pugi::xml_document& doc,
		const boost::any& message)
//		const dictionary_type& message)
{

	// declaration node
	pugi::xml_node decNode = doc.append_child(pugi::node_declaration);
	decNode.append_attribute("version") = "1.0";
	decNode.append_attribute("encoding") = "UTF-8";

	// doctype node
	doc.append_child(pugi::node_doctype).set_value("plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\"");

	// root node
	pugi::xml_node plistNode = doc.append_child("plist");
	plistNode.append_attribute("version") = "1.0";

	writeXMLNode(plistNode, message);
}

void writePlistBinary(
		PlistHelperData& d,
		const boost::any& message)
{
	using namespace std;
	//int totalRefs =  countDictionary(message);
	int totalRefs =  countAny(message) - 1;
	d._refCount = totalRefs;

	d._objRefSize = regulateNullBytes(intToBytes<int32_t>(d._refCount, hostLittleEndian()), 1).size();

	//writeBinaryDictionary(d, message);
	writeBinary(d, message);
	writeBinaryString(d, "bplist00", false);
	d._offsetTableOffset = (int64_t) d._objectTable.size();
	d._offsetTable.push_back(d._objectTable.size() - 8);
	d._offsetByteSize = regulateNullBytes(intToBytes<int>(d._offsetTable.back(), hostLittleEndian()), 1).size();

	vector<unsigned char> offsetBytes;

	reverse(d._offsetTable.begin(), d._offsetTable.end());

	for(unsigned int i = 0; i < d._offsetTable.size(); ++i)
	{
		d._offsetTable[i] = d._objectTable.size() - d._offsetTable[i];
		vector<unsigned char> buffer = regulateNullBytes(intToBytes<int>(d._offsetTable[i], hostLittleEndian()), d._offsetByteSize);
		//reverse(buffer.begin(), buffer.end());

		offsetBytes.insert(offsetBytes.end(), buffer.rbegin(), buffer.rend());
	}

	d._objectTable.insert(d._objectTable.end(), offsetBytes.begin(), offsetBytes.end());

	vector<unsigned char> dummy(6, 0);
	d._objectTable.insert(d._objectTable.end(), dummy.begin(), dummy.end());
	d._objectTable.push_back((unsigned char) (d._offsetByteSize));
	d._objectTable.push_back((unsigned char) (d._objRefSize));
	vector<unsigned char> temp = intToBytes<int64_t>((int64_t) totalRefs + 1, hostLittleEndian());
	d._objectTable.insert(d._objectTable.end(), temp.rbegin(), temp.rend());
	temp = intToBytes<int64_t>(0, hostLittleEndian());
	d._objectTable.insert(d._objectTable.end(), temp.begin(), temp.end());
	temp = intToBytes<int64_t>(d._offsetTableOffset, hostLittleEndian());
	d._objectTable.insert(d._objectTable.end(), temp.rbegin(), temp.rend());
}

void writePlistBinary(std::vector<char>& plist, const boost::any& message)
{
	PlistHelperData d;
	writePlistBinary(d, message);
	plist.resize(d._objectTable.size());
	std::copy((const char*) vecData(d._objectTable), (const char*) vecData(d._objectTable) + d._objectTable.size(), plist.begin());
}

void writePlistBinary(
		std::ostream& stream,
		const boost::any& message)
{
	PlistHelperData d;
	writePlistBinary(d, message);
	stream.write((const char*) vecData(d._objectTable), d._objectTable.size());
}

void writePlistBinary(
				const char* filename,
				const boost::any& message)
{
	std::ofstream stream(filename, std::ios::binary);
	writePlistBinary(stream, message);
	stream.close();
}

#if defined(_MSC_VER)
void writePlistBinary(
				const wchar_t* filename,
				const boost::any& message)
{
	std::ofstream stream(filename, std::ios::binary);
	writePlistBinary(stream, message);
	stream.close();
}
#endif

void writePlistXML(std::vector<char>& plist, const boost::any& message)
{
	std::stringstream ss;
	writePlistXML(ss, message);

	std::istreambuf_iterator<char> beg(ss);
	std::istreambuf_iterator<char> end;
	plist.clear();
	plist.insert(plist.begin(), beg, end);
}

void writePlistXML(
		std::ostream& stream,
		const boost::any& message)
{
	pugi::xml_document doc;
	writePlistXML(doc, message);
	doc.save(stream);
}

void writePlistXML(
		const char* filename,
		const boost::any& message)
{

	std::ofstream stream(filename, std::ios::binary);
	writePlistXML(stream, message);
	stream.close();
}

#if defined(_MSC_VER)
void writePlistXML(
		const wchar_t* filename,
		const boost::any& message)
{
	std::ofstream stream(filename, std::ios::binary);
	writePlistXML(stream, message);
	stream.close();
}
#endif

int countAny(const boost::any& object)
{
	using namespace std;
	static boost::any dict = dictionary_type();
	static boost::any array = array_type();

	int count = 0;
	if(object.type() == dict.type())
		count += countDictionary(boost::any_cast<dictionary_type >(object)) + 1;
	else if (object.type() == array.type())
		count += countArray(boost::any_cast<array_type >(object)) + 1;
	else
		++count;

	return count;
}

std::vector<unsigned char> writeBinary(PlistHelperData& d, const boost::any& obj)
{
	using namespace std;

	const std::type_info &objType = obj.type();

	std::vector<unsigned char> value;
	if(objType == typeid(int32_t))
		value = writeBinaryInteger(d, boost::any_cast<const int32_t&>(obj), true);
	else if(objType == typeid(int64_t))
		value = writeBinaryInteger(d, boost::any_cast<const int64_t&>(obj), true);
	else if(objType == typeid(long))
		value = writeBinaryInteger(d, boost::any_cast<const long&>(obj), true);
	else if(objType == typeid(short))
		value = writeBinaryInteger(d, boost::any_cast<const short&>(obj), true);
	else if(objType == typeid(dictionary_type))
		value = writeBinaryDictionary(d, boost::any_cast<const dictionary_type& >(obj));
	else if(objType == typeid(string))
		value = writeBinaryString(d, boost::any_cast<const string&>(obj), true);
	else if(objType == typeid(array_type))
		value = writeBinaryArray(d, boost::any_cast<const array_type& >(obj));
	else if(objType == typeid(data_type))
		value = writeBinaryByteArray(d, boost::any_cast<const data_type& >(obj));
	else if(objType == typeid(double))
		value = writeBinaryDouble(d, boost::any_cast<const double&>(obj));
	else if(objType == typeid(float))
		value = writeBinaryDouble(d, boost::any_cast<const float&>(obj));
	else if(objType == typeid(Date))
		value = writeBinaryDate(d, boost::any_cast<const Date&>(obj));
	else if(objType == typeid(bool))
		value = writeBinaryBool(d, boost::any_cast<const bool&>(obj));
	else
		throw Error((string("Plist Error: Can't serialize type ") + objType.name()).c_str());

	return value;
}

static uint32_t nextpow2(uint32_t x)
{
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return x + 1;
}

static uint32_t ilog2(uint32_t x)
{
	uint32_t r = 0;
	while (x >>= 1)
		++r;
	return r;
}

std::vector<unsigned char> writeBinaryByteArray(PlistHelperData& d, const data_type& byteArray)
{
	using namespace std;
	vector<unsigned char> header;
	if(byteArray.size() < 15)
		header.push_back(0x40 | ((unsigned char) byteArray.size()));
	else
	{
		header.push_back(0x40 | 0xf);
		vector<unsigned char> theSize = writeBinaryInteger(d, byteArray.size(), false);
		header.insert(header.end(), theSize.begin(), theSize.end());
	}

	vector<unsigned char> buffer(header);
	buffer.insert(buffer.end(), (unsigned char*) vecData(byteArray), (unsigned char*) vecData(byteArray) + byteArray.size());
	d._objectTable.insert(d._objectTable.begin(), buffer.begin(), buffer.end());

	return buffer;
}

std::vector<unsigned char> writeBinaryArray(PlistHelperData& d, const array_type& array)
{
	using namespace std;

	vector<int32_t> refs;
	for(array_type::const_reverse_iterator it = array.rbegin();
			it != array.rend();
			++it)
	{
		writeBinary(d, *it);
		d._offsetTable.push_back(d._objectTable.size());
		refs.push_back(d._refCount);
		d._refCount--;
	}

	vector<unsigned char> header;
	if (array.size() < 15)
	{
		header.push_back(0xA0 | ((unsigned char) array.size()));
	}
	else
	{
		header.push_back(0xA0 | 0xf);
		vector<unsigned char> theSize = writeBinaryInteger(d, array.size(), false);
		header.insert(header.end(), theSize.begin(), theSize.end());
	}

	// try to do this more efficiently.  Not good to insert at the begining of buffer.

	vector<unsigned char> buffer;
	for(vector<int32_t>::const_iterator it = refs.begin();
			it != refs.end();
			++it)
	{
		vector<unsigned char> refBuffer = regulateNullBytes(intToBytes<int32_t>(*it, hostLittleEndian()), d._objRefSize);
//		reverse(refBuffer.begin(), refBuffer.end());
		buffer.insert(buffer.begin(), refBuffer.rbegin(), refBuffer.rend());
	}

	buffer.insert(buffer.begin(), header.begin(), header.end());

	d._objectTable.insert(d._objectTable.begin(), buffer.begin(), buffer.end());

	return buffer;
}

std::vector<unsigned char> writeBinaryDictionary(PlistHelperData& d, const dictionary_type& dictionary)
{
	using namespace std;

	vector<int32_t> refs;
	for(dictionary_type::const_reverse_iterator it = dictionary.rbegin();
			it != dictionary.rend();
			++it)
	{
		writeBinary(d, it->second);
		d._offsetTable.push_back(d._objectTable.size());
		refs.push_back(d._refCount);
		d._refCount--;
	}

	for(dictionary_type::const_reverse_iterator it = dictionary.rbegin();
			it != dictionary.rend();
			++it)
	{
		writeBinary(d, it->first);
		d._offsetTable.push_back(d._objectTable.size());
		refs.push_back(d._refCount);
		d._refCount--;
	}

	vector<unsigned char> header;
	if (dictionary.size() < 15)
	{
		header.push_back(0xD0 | ((unsigned char) dictionary.size()));
	}
	else
	{
		header.push_back(0xD0 | 0xf);
		vector<unsigned char> theSize = writeBinaryInteger(d, dictionary.size(), false);
		header.insert(header.end(), theSize.begin(), theSize.end());
	}

	// try to do this more efficiently.  Not good to insert at the begining of buffer.

	vector<unsigned char> buffer;
	for(vector<int32_t>::const_iterator it = refs.begin();
			it != refs.end();
			++it)
	{
		vector<unsigned char> refBuffer = regulateNullBytes(intToBytes<int32_t>(*it, hostLittleEndian()), d._objRefSize);
//		reverse(refBuffer.begin(), refBuffer.end());
		buffer.insert(buffer.begin(), refBuffer.rbegin(), refBuffer.rend());
	}

	buffer.insert(buffer.begin(), header.begin(), header.end());

	d._objectTable.insert(d._objectTable.begin(), buffer.begin(), buffer.end());

	return buffer;
}

std::vector<unsigned char> writeBinaryDouble(PlistHelperData& d, double value)
{
	using namespace std;
	vector<unsigned char> buffer = regulateNullBytes(doubleToBytes(value, hostLittleEndian()), 4);
	buffer.resize(nextpow2(buffer.size()), 0);

	unsigned char header = 0x20 | ilog2(buffer.size());
	buffer.push_back(header);
	reverse(buffer.begin(), buffer.end());

	d._objectTable.insert(d._objectTable.begin(), buffer.begin(), buffer.end());

	return buffer;
}

std::vector<unsigned char> writeBinaryBool(PlistHelperData& d, bool value)
{
	std::vector<unsigned char> buffer;
	if(value)
		buffer.push_back(0x09);
	else
		buffer.push_back(0x08);

	d._objectTable.insert(d._objectTable.begin(), buffer.begin(), buffer.end());
	return buffer;
}

std::vector<unsigned char> writeBinaryDate(PlistHelperData& d, const Date& date)
{
	std::vector<unsigned char> buffer;

	// need to serialize as Apple epoch.

	double macTime = date.timeAsAppleEpoch();

	buffer = doubleToBytes(macTime, false);
	buffer.insert(buffer.begin(), 0x33);

	d._objectTable.insert(d._objectTable.begin(), buffer.begin(), buffer.end());
	return buffer;
}

std::vector<unsigned char> writeBinaryInteger(PlistHelperData& d, int64_t value, bool write)
{
	using namespace std;

	// The integer is initially forced to be 64 bit because it must be serialized
	// as 8 bytes if it is negative.   If it is not negative, the
	// regulateNullBytes step will reduce the representation down to the min
	// power base 2 bytes needed to store it.

	vector<unsigned char> buffer = intToBytes<int64_t>(value, hostLittleEndian());
	buffer = regulateNullBytes(intToBytes<int64_t>(value, hostLittleEndian()), 1);
	buffer.resize(nextpow2(buffer.size()), 0);

	unsigned char header = 0x10 | ilog2(buffer.size());
	buffer.push_back(header);
	reverse(buffer.begin(), buffer.end());

	if(write)
		d._objectTable.insert(d._objectTable.begin(), buffer.begin(), buffer.end());

	return buffer;
}

std::vector<unsigned char> writeBinaryString(PlistHelperData& d, const std::string& value, bool head)
{
	using namespace std;
	vector<unsigned char> buffer;
	buffer.reserve(value.size());

	for(std::string::const_iterator it = value.begin();
			it != value.end();
			++it)
		buffer.push_back((unsigned char) *it);

	if(head)
	{
		vector<unsigned char> header;
		if (value.length() < 15)
			header.push_back(0x50 | ((unsigned char) value.length()));
		else
		{
			header.push_back(0x50 | 0xf);
			vector<unsigned char> theSize = writeBinaryInteger(d, buffer.size(), false);
			header.insert(header.end(), theSize.begin(), theSize.end());
		}
		buffer.insert(buffer.begin(), header.begin(), header.end());
	}

	d._objectTable.insert(d._objectTable.begin(), buffer.begin(), buffer.end());

	return buffer;
}

int countDictionary(const dictionary_type& dictionary)
{
	using namespace std;

	int count = 0;
	for(dictionary_type::const_iterator it = dictionary.begin();
			it != dictionary.end();
			++it)
	{
		++count;
		count += countAny(it->second);
	}

	return count;
}

int countArray(const array_type& array)
{
	using namespace std;
	int count = 0;
	for(array_type::const_iterator it = array.begin();
			it != array.end();
			++it)
		count += countAny(*it);

	return count;
}

void readPlist(std::istream& stream, boost::any& message)
{
	int start = stream.tellg();
	stream.seekg(0, std::ifstream::end);
	int size = ((int) stream.tellg()) - start;
	if(size > 0)
	{
		stream.seekg(0, std::ifstream::beg);
		std::vector<char> buffer(size);
		stream.read( (char *)&buffer[0], size );

		readPlist(&buffer[0], size, message);
	}
	else
	{
		throw Error("Can't read zero length data");
	}
}

void readPlist(const char* byteArrayTemp, int64_t size, boost::any& message)
{
	using namespace std;
	const unsigned char* byteArray = (const unsigned char*) byteArrayTemp;
	if (!byteArray || (size == 0))
		throw Error("Plist: Empty plist data");

	// infer plist type from header.  If it has the bplist00 header as first 8
	// bytes, then it's a binary plist.  Otherwise, assume it's XML

	std::string magicHeader((const char*) byteArray, 8);
	if(magicHeader == "bplist00")
	{
		PlistHelperData d;
		parseTrailer(d, getRange(byteArray, size - 32, 32));

		d._objectTable = getRange(byteArray, 0, d._offsetTableOffset);
		std::vector<unsigned char> offsetTableBytes = getRange(byteArray, d._offsetTableOffset, size - d._offsetTableOffset - 32);

		parseOffsetTable(d, offsetTableBytes);

		message = parseBinary(d, 0);
	}
	else
	{
		pugi::xml_document doc;
		pugi::xml_parse_result result = doc.load_buffer(byteArray, (size_t)size);
		if(!result)
			throw Error((string("Plist: XML parsed with error ") + result.description()).c_str());

		pugi::xml_node rootNode = doc.child("plist").first_child();
		message = parse(rootNode);
	}

}

dictionary_type parseDictionary(pugi::xml_node& node)
{
	using namespace std;

	dictionary_type dict;
	for(pugi::xml_node_iterator it = node.begin(); it != node.end(); ++it)
	{
		if(string("key") != it->name())
			throw Error("Plist: XML dictionary key expected but not found");

		string key(it->first_child().value());
		++it;

		if(it == node.end())
			throw Error("Plist: XML dictionary value expected for key " + key + "but not found");
		else if(string("key") == it->name())
			throw Error("Plist: XML dictionary value expected for key " + key + "but found another key node");

		dict[key] = parse(*it);
	}

	return dict;
}

array_type parseArray(pugi::xml_node& node)
{
	using namespace std;

	array_type array;
	for(pugi::xml_node_iterator it = node.begin(); it != node.end(); ++it)
		array.push_back(parse(*it));

	return array;
}

Date parseDate(pugi::xml_node& node)
{
	Date date;
	date.setTimeFromXMLConvention(node.first_child().value());

	return date;
}

std::vector<char> base64Decode(const char* encodedData)
{
	using namespace std;

	vector<char> data;
	insert_iterator<vector<char> > ii(data, data.begin());
	base64<char> b64;
	int state = 0;
	b64.get(encodedData, encodedData + strlen(encodedData), ii, state);

	return data;
}

void base64Encode(std::string& dataEncoded, const std::vector<char>& data)
{
	using namespace std;
	dataEncoded.clear();
	insert_iterator<string> ii(dataEncoded, dataEncoded.begin());
	base64<char> b64;
	int state = 0;

#if defined(_WIN32) || defined(_WIN64)
	b64.put(data.begin(), data.end(), ii, state , base64<>::crlf());
#else
	b64.put(data.begin(), data.end(), ii, state , base64<>::lf());
#endif

}

boost::any parse(pugi::xml_node& node)
{
	using namespace std;

	string nodeName = node.name();

	boost::any result;
	if("dict" == nodeName)
		result = parseDictionary(node);
	else if("array" == nodeName)
		result = parseArray(node);
	else if("string" == nodeName)
		result = string(node.first_child().value());
	else if("integer" == nodeName)
		result = (int64_t) atoll(node.first_child().value());
	else if("real" == nodeName)
		result = atof(node.first_child().value());
	else if("false" == nodeName)
		result = bool(false);
	else if("true" == nodeName)
		result = bool(true);
	else if("data" == nodeName)
		result = base64Decode(node.first_child().value());
	else if("date" == nodeName)
		result = parseDate(node);
	else
		throw Error(string("Plist: XML unknown node type " + nodeName));

	return result;
}

void parseOffsetTable(PlistHelperData& d, const std::vector<unsigned char>& offsetTableBytes)
{
	for (unsigned int i = 0; i < offsetTableBytes.size(); i += d._offsetByteSize)
	{
		std::vector<unsigned char> temp = getRange(offsetTableBytes, i, d._offsetByteSize);
		std::reverse(temp.begin(), temp.end());
		d._offsetTable.push_back(
				bytesToInt<int32_t>(
					vecData(regulateNullBytes(temp, 4)), hostLittleEndian()));
	}
}

void parseTrailer(PlistHelperData& d, const std::vector<unsigned char>& trailer)
{
	d._offsetByteSize = bytesToInt<int32_t>(vecData(regulateNullBytes(getRange(trailer, 6, 1), 4)), hostLittleEndian());
	d._objRefSize = bytesToInt<int32_t>(vecData(regulateNullBytes(getRange(trailer, 7, 1), 4)), hostLittleEndian());

	std::vector<unsigned char> refCountBytes = getRange(trailer, 12, 4);
//	std::reverse(refCountBytes.begin(), refCountBytes.end());
	d._refCount = bytesToInt<int32_t>(vecData(refCountBytes), false);

	std::vector<unsigned char> offsetTableOffsetBytes = getRange(trailer, 24, 8);
//	std::reverse(offsetTableOffsetBytes.begin(), offsetTableOffsetBytes.end());
	d._offsetTableOffset = bytesToInt<int64_t>(vecData(offsetTableOffsetBytes), false);
}


std::vector<unsigned char> regulateNullBytes(const std::vector<unsigned char>& origBytes, unsigned int minBytes)
{

	std::vector<unsigned char> bytes(origBytes);
	while((bytes.back() == 0) && (bytes.size() > minBytes))
		bytes.pop_back();

	while(bytes.size() < minBytes)
		bytes.push_back(0);

	return bytes;
}

boost::any parseBinary(const PlistHelperData& d, int objRef)
{
	unsigned char header = d._objectTable[d._offsetTable[objRef]];
	switch (header & 0xF0)
	{
		case 0x00:
			{
				return parseBinaryBool(d, d._offsetTable[objRef]);
			}
		case 0x10:
			{
				int intByteCount;
				return parseBinaryInt(d, d._offsetTable[objRef], intByteCount);
			}
		case 0x20:
			{
				return parseBinaryReal(d, d._offsetTable[objRef]);
			}
		case 0x30:
			{
				return parseBinaryDate(d, d._offsetTable[objRef]);
			}
		case 0x40:
			{
				return parseBinaryByteArray(d, d._offsetTable[objRef]);
			}
		case 0x50:
			{
				return parseBinaryString(d, d._offsetTable[objRef]);
			}
		case 0x60:
			{
				return parseBinaryUnicode(d, d._offsetTable[objRef]);
			}
		case 0xD0:
			{
				return parseBinaryDictionary(d, objRef);
			}
		case 0xA0:
			{
				return parseBinaryArray(d, objRef);
			}
	}
	throw Error("This type is not supported");
}

std::vector<int32_t> getRefsForContainers(const PlistHelperData& d, int objRef)
{
	using namespace std;
	int32_t refCount = 0;
	int refStartPosition;
	refCount = getCount(d, d._offsetTable[objRef], d._objectTable[d._offsetTable[objRef]], refStartPosition);
	refStartPosition += d._offsetTable[objRef];

	vector<int32_t> refs;
	int mult = 1;
	if((((unsigned char) d._objectTable[d._offsetTable[objRef]]) & 0xF0) == 0xD0)
		mult = 2;
	for (int i = refStartPosition; i < refStartPosition + refCount * mult * d._objRefSize; i += d._objRefSize)
	{
		std::vector<unsigned char> refBuffer = getRange(d._objectTable, i, d._objRefSize);
		reverse(refBuffer.begin(), refBuffer.end());
		refs.push_back(bytesToInt<int32_t>(vecData(regulateNullBytes(refBuffer, 4)), hostLittleEndian()));
	}

	return refs;
}

array_type  parseBinaryArray(const PlistHelperData& d, int objRef)
{
	using namespace std;
	vector<int32_t> refs = getRefsForContainers(d, objRef);
	int32_t refCount = refs.size();

	array_type array;
	for(int i = 0; i < refCount; ++i)
		array.push_back(parseBinary(d, refs[i]));

	return array;
}

dictionary_type parseBinaryDictionary(const PlistHelperData& d, int objRef)
{
	using namespace std;
	vector<int32_t> refs = getRefsForContainers(d, objRef);
	int32_t refCount = refs.size() / 2;

	dictionary_type dict;
	for (int i = 0; i < refCount; i++)
	{
		boost::any keyAny = parseBinary(d, refs[i]);

		try
		{
			std::string& key = boost::any_cast<std::string&>(keyAny);
			dict[key] =  parseBinary(d, refs[i + refCount]);
		}
		catch(boost::bad_any_cast& )
		{
			throw Error("Error parsing dictionary.  Key can't be parsed as a string");
		}
	}

	return dict;
}

std::string parseBinaryString(const PlistHelperData& d, int headerPosition)
{
	unsigned char headerByte = d._objectTable[headerPosition];
	int charStartPosition;
	int32_t charCount = getCount(d, headerPosition, headerByte, charStartPosition);
	charStartPosition += headerPosition;

	std::vector<unsigned char> characterBytes = getRange(d._objectTable, charStartPosition, charCount);
	std::string buffer = std::string((char*) vecData(characterBytes), characterBytes.size());
	return buffer;
}

std::string parseBinaryUnicode(const PlistHelperData& d, int headerPosition)
{
	unsigned char headerByte = d._objectTable[headerPosition];
	int charStartPosition;
	int32_t charCount = getCount(d, headerPosition, headerByte, charStartPosition);
	charStartPosition += headerPosition;

	std::vector<unsigned char> characterBytes = getRange(d._objectTable, charStartPosition, charCount * 2);
	if (hostLittleEndian()) {
		if (! characterBytes.empty()) {
			for (std::size_t i = 0, n = characterBytes.size(); i < n - 1; i += 2)
				std::swap(characterBytes[i], characterBytes[i + 1]);
		}
	}

	int16_t *u16chars = (int16_t*) vecData(characterBytes);
	std::size_t u16len = characterBytes.size() / 2;
	std::string result = boost::locale::conv::utf_to_utf<char, int16_t>(u16chars, u16chars + u16len, boost::locale::conv::stop);
	return result;
}

int64_t parseBinaryInt(const PlistHelperData& d, int headerPosition, int& intByteCount)
{
	unsigned char header = d._objectTable[headerPosition];
	intByteCount = 1 << (header & 0xf);
	std::vector<unsigned char> buffer = getRange(d._objectTable, headerPosition + 1, intByteCount);
	reverse(buffer.begin(), buffer.end());

	return bytesToInt<int64_t>(vecData(regulateNullBytes(buffer, 8)), hostLittleEndian());
}

double parseBinaryReal(const PlistHelperData& d, int headerPosition)
{
	unsigned char header = d._objectTable[headerPosition];
	int byteCount = 1 << (header & 0xf);
	std::vector<unsigned char> buffer = getRange(d._objectTable, headerPosition + 1, byteCount);
	reverse(buffer.begin(), buffer.end());

	return bytesToDouble(vecData(regulateNullBytes(buffer, 8)), hostLittleEndian());
}

bool parseBinaryBool(const PlistHelperData& d, int headerPosition)
{
	unsigned char header = d._objectTable[headerPosition];
	bool value;
	if(header == 0x09)
		value = true;
	else if (header == 0x08)
		value = false;
	else if (header == 0x00)
	{
		// null byte, not sure yet what to do with this.  It's in the spec but we
		// have never encountered it.

		throw Error("Plist: null byte encountered, unsure how to parse");
	}
	else if (header == 0x0F)
	{
		// fill byte, not sure yet what to do with this.  It's in the spec but we
		// have never encountered it.

		throw Error("Plist: fill byte encountered, unsure how to parse");
	}
	else
	{
		std::stringstream ss;
		ss<<"Plist: unknown header "<<header;
		throw Error(ss.str().c_str());
	}

	return value;
}

Date parseBinaryDate(const PlistHelperData& d, int headerPosition)
{
	// date always an 8 byte float starting after full byte header
	std::vector<unsigned char> buffer = getRange(d._objectTable, headerPosition + 1, 8);

	Date date;

	// Date is stored as Apple Epoch and big endian.
	date.setTimeFromAppleEpoch(bytesToDouble(vecData(buffer), false));

	return date;
}

data_type parseBinaryByteArray(const PlistHelperData& d, int headerPosition)
{
	unsigned char headerByte = d._objectTable[headerPosition];
	int byteStartPosition;
	int32_t byteCount = getCount(d, headerPosition, headerByte, byteStartPosition);
	byteStartPosition += headerPosition;

	return getRange((const char*) vecData(d._objectTable), byteStartPosition, byteCount);
}

int32_t getCount(const PlistHelperData& d, int bytePosition, unsigned char headerByte, int& startOffset)
{
	unsigned char headerByteTrail = headerByte & 0xf;
	if (headerByteTrail < 15)
	{
		startOffset = 1;
		return headerByteTrail;
	}
	else
	{
		int32_t count = (int32_t)parseBinaryInt(d, bytePosition + 1, startOffset);
		startOffset += 2;
		return count;
	}
}

template<typename T>
std::string stringFromValue(const T& value)
{
	std::stringstream ss;
	ss<<value;
	return ss.str();
}

template <typename IntegerType>
IntegerType bytesToInt(const unsigned char* bytes, bool littleEndian)
{
	IntegerType result = 0;
	if (littleEndian)
		for (int n = sizeof( result ) - 1; n >= 0; n--)
			result = (result << 8) + bytes[n];
	else
		for (unsigned n = 0; n < sizeof( result ); n++)
			result = (result << 8) + bytes[n];
	return result;
}

double bytesToDouble(const unsigned char* bytes, bool littleEndian)
{
	double result;
	int numBytes = sizeof(double);
	if(littleEndian)
		memcpy( &result, bytes, numBytes);
	else
	{
		std::vector<unsigned char> bytesReverse(numBytes);
		std::reverse_copy(bytes, bytes + numBytes, bytesReverse.begin());
		memcpy( &result, vecData(bytesReverse), numBytes);
	}
	return result;
}

std::vector<unsigned char> doubleToBytes(double val, bool littleEndian)
{
	std::vector<unsigned char> result(sizeof(double));
	memcpy(vecData(result), &val, sizeof(double));
	if(!littleEndian)
		std::reverse(result.begin(), result.end());

	return result;
}

template<typename IntegerType>
std::vector<unsigned char> intToBytes(IntegerType val, bool littleEndian)
{
	unsigned int numBytes = sizeof(val);
	std::vector<unsigned char> bytes(numBytes);

	for(unsigned n = 0; n < numBytes; ++n)
		if(littleEndian)
			bytes[n] = (val >> 8 * n) & 0xff;
		else
			bytes[numBytes - 1 - n] = (val >> 8 * n) & 0xff;

	return bytes;
}

std::vector<unsigned char> getRange(const unsigned char* origBytes, int64_t index, int64_t size)
{
	std::vector<unsigned char> result((std::vector<unsigned char>::size_type)size);
	std::copy(origBytes + index, origBytes + index + size, result.begin());
	return result;
}

std::vector<char> getRange(const char* origBytes, int64_t index, int64_t size)
{
	std::vector<char> result((std::vector<char>::size_type)size);
	std::copy(origBytes + index, origBytes + index + size, result.begin());
	return result;
}

std::vector<unsigned char> getRange(const std::vector<unsigned char>& origBytes, int64_t index, int64_t size)
{
	if((index + size) > (int64_t) origBytes.size())
		throw Error("Out of bounds getRange");
	return getRange(vecData(origBytes), index, size);
}

} // namespace Plist

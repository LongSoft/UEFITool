/*
 * This source file is part of the bstring string library.  This code was
 * written by Paul Hsieh in 2002-2015, and is covered by the BSD open source
 * license and the GPL. Refer to the accompanying documentation for details
 * on usage and license.
 */

/*
 * bstrwrap.h
 *
 * This file is the C++ wrapper for the bstring functions.
 */

#ifndef BSTRWRAP_INCLUDE
#define BSTRWRAP_INCLUDE

/////////////////// Configuration defines //////////////////////////////

// WATCOM C/C++ has broken STL and std::iostream support.  If you have
// ported over STLport, then you can #define BSTRLIB_CAN_USE_STL to use
// the CBStringList class.
#if defined(__WATCOMC__)
#  if !defined (BSTRLIB_CAN_USE_STL) && !defined (BSTRLIB_CANNOT_USE_STL)
#    define BSTRLIB_CANNOT_USE_STL
#  endif
#  if !defined (BSTRLIB_CAN_USE_IOSTREAM) && !defined (BSTRLIB_CANNOT_USE_IOSTREAM)
#    define BSTRLIB_CANNOT_USE_IOSTREAM
#  endif
#endif

// By default it assumed that STL has been installed and works for your
// compiler.  If this is not the case, then #define BSTRLIB_CANNOT_USE_STL
#if !defined (BSTRLIB_CANNOT_USE_STL) && !defined (BSTRLIB_CAN_USE_STL)
#define BSTRLIB_CAN_USE_STL
#endif

// By default it assumed that std::iostream works well with your compiler.  
// If this is not the case, then #define BSTRLIB_CAN_USE_IOSTREAM
#if !defined (BSTRLIB_CANNOT_USE_IOSTREAM) && !defined (BSTRLIB_CAN_USE_IOSTREAM)
#define BSTRLIB_CAN_USE_IOSTREAM
#endif

// By default it is assumed that your compiler can deal with and has enabled
// exception handlling.  If this is not the case then you will need to 
// #define BSTRLIB_DOESNT_THROW_EXCEPTIONS
#if !defined (BSTRLIB_THROWS_EXCEPTIONS) && !defined (BSTRLIB_DOESNT_THROW_EXCEPTIONS)
#define BSTRLIB_THROWS_EXCEPTIONS
#endif

////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include "bstrlib.h"
#include "../ubytearray.h"

#ifdef __cplusplus

#if defined(BSTRLIB_CAN_USE_STL)

#if defined(__WATCOMC__)
#pragma warning 604 10
#pragma warning 595 10
#pragma warning 594 10
#pragma warning 549 10
#endif

#include <vector>
#include <string>

#if defined(__WATCOMC__)
#pragma warning 604 9
#pragma warning 595 9
#pragma warning 594 9
#endif

#endif

namespace Bstrlib {

#ifdef BSTRLIB_THROWS_EXCEPTIONS
#if defined(BSTRLIB_CAN_USE_STL)
struct CBStringException : public std::exception {
private:
	std::string msg;
public:
	CBStringException (const std::string inmsg) : msg(inmsg) {}
	virtual ~CBStringException () throw () {}
	virtual const char *what () const throw () { return msg.c_str(); }
};
#else
struct CBStringException {
private:
	char * msg;
	int needToFree;
public:
	CBStringException (const char * inmsg) : needToFree(0) {
		if (inmsg) {
			msg = (char *) malloc (1 + strlen (inmsg));
			if (NULL == msg) msg = "Out of memory";
			else {
				strcpy (msg, inmsg);
				needToFree = 1;
			}
		} else {
			msg = "NULL exception message";
		}
	}
	virtual ~CBStringException () throw () {
		if (needToFree) {
			free (msg);
			needToFree = 0;
			msg = NULL;
		}
	}
	virtual const char *what () const throw () { return msg; }
};
#endif
#define bstringThrow(er) {\
	CBStringException bstr__cppwrapper_exception ("CBString::" er "");\
	throw bstr__cppwrapper_exception;\
}
#else
#define bstringThrow(er) {}
#endif

struct CBString;

#ifdef _MSC_VER
#pragma warning(disable:4512)
#endif

class CBCharWriteProtected {
friend struct CBString;
	private:
	const struct tagbstring& s;
	unsigned int idx;
	CBCharWriteProtected (const struct tagbstring& c, int i) : s(c), idx((unsigned int)i) {
		if (idx >= (unsigned) s.slen) {
			bstringThrow ("character index out of bounds");
		}
	}

	public:

	inline char operator = (char c) {
		if (s.mlen <= 0) {
			bstringThrow ("Write protection error");
		} else {
#ifndef BSTRLIB_THROWS_EXCEPTIONS
			if (idx >= (unsigned) s.slen) return '\0';
#endif
			s.data[idx] = (unsigned char) c;
		}
		return (char) s.data[idx];
	}
	inline unsigned char operator = (unsigned char c) {
		if (s.mlen <= 0) {
			bstringThrow ("Write protection error");
		} else {
#ifndef BSTRLIB_THROWS_EXCEPTIONS
			if (idx >= (unsigned) s.slen) return '\0';
#endif
			s.data[idx] = c;
		}
		return s.data[idx];
	}
	inline operator unsigned char () const {
#ifndef BSTRLIB_THROWS_EXCEPTIONS
		if (idx >= (unsigned) s.slen) return (unsigned char) '\0';
#endif
		return s.data[idx];
	}
};

struct CBString : public tagbstring {

	// Constructors
	CBString ();
	CBString (char c);
	CBString (unsigned char c);
	CBString (const char *s);
	CBString (int len, const char *s);
	CBString (const CBString& b);
	CBString (const tagbstring& x);
	CBString (char c, int len);
	CBString (const void * blk, int len);

#if defined(BSTRLIB_CAN_USE_STL)
	CBString (const struct CBStringList& l);
	CBString (const struct CBStringList& l, const CBString& sep);
	CBString (const struct CBStringList& l, char sep);
	CBString (const struct CBStringList& l, unsigned char sep);
#endif

	// Destructor
#if !defined(BSTRLIB_DONT_USE_VIRTUAL_DESTRUCTOR)
	virtual 
#endif
	~CBString ();

	// = operator
	const CBString& operator = (char c);
	const CBString& operator = (unsigned char c);
	const CBString& operator = (const char *s);
	const CBString& operator = (const CBString& b);
	const CBString& operator = (const tagbstring& x);

	// += operator
	const CBString& operator += (char c);
	const CBString& operator += (unsigned char c);
	const CBString& operator += (const char *s);
	const CBString& operator += (const CBString& b);
	const CBString& operator += (const tagbstring& x);

	// *= operator
	inline const CBString& operator *= (int count) {
		this->repeat (count);
		return *this;
	}

	// + operator
	const CBString operator + (char c) const;
	const CBString operator + (unsigned char c) const;
	const CBString operator + (const unsigned char *s) const;
	const CBString operator + (const char *s) const;
	const CBString operator + (const CBString& b) const;
	const CBString operator + (const tagbstring& x) const;

	// * operator
	inline const CBString operator * (int count) const {
		CBString retval (*this);
		retval.repeat (count);
		return retval;
	}

	// Comparison operators
	bool operator == (const CBString& b) const;
	bool operator == (const char * s) const;
	bool operator == (const unsigned char * s) const;
	bool operator != (const CBString& b) const;
	bool operator != (const char * s) const;
	bool operator != (const unsigned char * s) const;
	bool operator <  (const CBString& b) const;
	bool operator <  (const char * s) const;
	bool operator <  (const unsigned char * s) const;
	bool operator <= (const CBString& b) const;
	bool operator <= (const char * s) const;
	bool operator <= (const unsigned char * s) const;
	bool operator >  (const CBString& b) const;
	bool operator >  (const char * s) const;
	bool operator >  (const unsigned char * s) const;
	bool operator >= (const CBString& b) const;
	bool operator >= (const char * s) const;
	bool operator >= (const unsigned char * s) const;

	// Casts
	inline operator const char* () const { return (const char *)data; }
	inline operator const unsigned char* () const { return (const unsigned char *)data; }
	operator double () const;
	operator float () const;
	operator int () const;
	operator unsigned int () const;

	// Accessors
	inline int length () const {return slen;}

	inline unsigned char character (int i) const {
		if (((unsigned) i) >= (unsigned) slen) {
#ifdef BSTRLIB_THROWS_EXCEPTIONS
			bstringThrow ("character idx out of bounds");
#else
			return '\0';
#endif
		}
		return data[i];
	}
	inline unsigned char operator [] (int i) const { return character(i); }

	inline CBCharWriteProtected character (int i) {
		return CBCharWriteProtected (*this, i);
	}
	inline CBCharWriteProtected operator [] (int i) { return character(i); }

	// Space allocation hint method.
	void alloc (int length);

	// Search methods.
	int caselessEqual (const CBString& b) const;
	int caselessCmp (const CBString& b) const;
	int find (const CBString& b, int pos = 0) const;
	int find (const char * b, int pos = 0) const;
	int caselessfind (const CBString& b, int pos = 0) const;
	int caselessfind (const char * b, int pos = 0) const;
	int find (char c, int pos = 0) const;
	int reversefind (const CBString& b, int pos) const;
	int reversefind (const char * b, int pos) const;
	int caselessreversefind (const CBString& b, int pos) const;
	int caselessreversefind (const char * b, int pos) const;
	int reversefind (char c, int pos) const;
	int findchr (const CBString& b, int pos = 0) const;
	int findchr (const char * s, int pos = 0) const;
	int reversefindchr (const CBString& b, int pos) const;
	int reversefindchr (const char * s, int pos) const;
	int nfindchr (const CBString& b, int pos = 0) const;
	int nfindchr (const char * b, int pos = 0) const;
	int nreversefindchr (const CBString& b, int pos) const;
	int nreversefindchr (const char * b, int pos) const;

	// Search and substitute methods.
	void findreplace (const CBString& find, const CBString& repl, int pos = 0);
	void findreplace (const CBString& find, const char * repl, int pos = 0);
	void findreplace (const char * find, const CBString& repl, int pos = 0);
	void findreplace (const char * find, const char * repl, int pos = 0);
	void findreplacecaseless (const CBString& find, const CBString& repl, int pos = 0);
	void findreplacecaseless (const CBString& find, const char * repl, int pos = 0);
	void findreplacecaseless (const char * find, const CBString& repl, int pos = 0);
	void findreplacecaseless (const char * find, const char * repl, int pos = 0);

	// Extraction method.
	CBString midstr (int left, int len) const;

	// Standard manipulation methods.
	void setsubstr (int pos, const CBString& b, unsigned char fill = ' ');
	void setsubstr (int pos, const char * b, unsigned char fill = ' ');
	void insert (int pos, const CBString& b, unsigned char fill = ' ');
	void insert (int pos, const char * b, unsigned char fill = ' ');
	void insertchrs (int pos, int len, unsigned char fill = ' ');
	void replace (int pos, int len, const CBString& b, unsigned char fill = ' ');
	void replace (int pos, int len, const char * s, unsigned char fill = ' ');
	void remove (int pos, int len);
	void trunc (int len);

	// Miscellaneous methods.
	void format (const char * fmt, ...);
	void formata (const char * fmt, ...);
	void fill (int length, unsigned char fill = ' ');
	void repeat (int count);
	void ltrim (const CBString& b = CBString (bsStaticBlkParms (" \t\v\f\r\n")));
	void rtrim (const CBString& b = CBString (bsStaticBlkParms (" \t\v\f\r\n")));
	inline void trim (const CBString& b = CBString (bsStaticBlkParms (" \t\v\f\r\n"))) {
		rtrim (b);
		ltrim (b);
	}
	void toupper ();
	void tolower ();

	// Write protection methods.
	void writeprotect ();
	void writeallow ();
	inline bool iswriteprotected () const { return mlen <= 0; }

	// Join methods.
#if defined(BSTRLIB_CAN_USE_STL)
	void join (const struct CBStringList& l);
	void join (const struct CBStringList& l, const CBString& sep);
	void join (const struct CBStringList& l, char sep);
	void join (const struct CBStringList& l, unsigned char sep);
#endif

	// CBStream methods
	int gets (bNgetc getcPtr, void * parm, char terminator = '\n');
	int read (bNread readPtr, void * parm);

	// QString compatibility methods
	const char *toLocal8Bit() const { return *this; }
	bool contains(const char *str) { return find(str) >= 0; }
	bool isEmpty() const { return slen == 0; }
	void clear() { *this = ""; }
	CBString left(int len) const { return midstr(0, len); }
	CBString mid(int pos, int len) const { return midstr(pos, len); }
	static CBString fromUtf16(const unsigned short* str) {
		// Naive implementation assuming that only ASCII LE part of UCS2 is used, str may not be aligned.
		CBString msg;
		const char *str8 = reinterpret_cast<const char *>(str);
		while (str8[0]) {
			msg += str8[0];
			str8 += 2;
		}
		return msg;
	}
	CBString leftJustified(int length) { if (length > slen) { return *this + CBString(' ', length - slen); } return *this; }
};
extern const CBString operator + (const char *a, const CBString& b);
extern const CBString operator + (const unsigned char *a, const CBString& b);
extern const CBString operator + (char c, const CBString& b);
extern const CBString operator + (unsigned char c, const CBString& b);
extern const CBString operator + (const tagbstring& x, const CBString& b);
inline const CBString operator * (int count, const CBString& b) {
	CBString retval (b);
	retval.repeat (count);
	return retval;
}

#if defined(BSTRLIB_CAN_USE_IOSTREAM)
extern std::ostream& operator << (std::ostream& sout, CBString b);
extern std::istream& operator >> (std::istream& sin, CBString& b);
extern std::istream& getline (std::istream& sin, CBString& b, char terminator='\n');
#endif

struct CBStream {
friend struct CBStringList;
private:
	struct bStream * m_s;
public:
	CBStream (bNread readPtr, void * parm);
	~CBStream ();
	int buffLengthSet (int sz);
	int buffLengthGet ();
	int eof () const;

	CBString readLine (char terminator);
	CBString readLine (const CBString& terminator);
	void readLine (CBString& s, char terminator);
	void readLine (CBString& s, const CBString& terminator);
	void readLineAppend (CBString& s, char terminator);
	void readLineAppend (CBString& s, const CBString& terminator);

	CBString read ();
	CBString& operator >> (CBString& s);

	CBString read (int n);
	void read (CBString& s);
	void read (CBString& s, int n);
	void readAppend (CBString& s);
	void readAppend (CBString& s, int n);

	void unread (const CBString& s);
	inline CBStream& operator << (const CBString& s) {
		this->unread (s);
		return *this;
	}

	CBString peek () const;
	void peek (CBString& s) const;
	void peekAppend (CBString& s) const;
};

#if defined(BSTRLIB_CAN_USE_STL)
struct CBStringList : public std::vector<CBString> {
	// split a string into a vector of strings.
	void split (const CBString& b, unsigned char splitChar);
	void split (const CBString& b, const CBString& s);
	void splitstr (const CBString& b, const CBString& s);
	void split (const CBStream& b, unsigned char splitChar);
	void split (const CBStream& b, const CBString& s);
	void splitstr (const CBStream& b, const CBString& s);
};
#endif

} // namespace Bstrlib

#if !defined (BSTRLIB_DONT_ASSUME_NAMESPACE)
using namespace Bstrlib;
#endif

#endif
#endif

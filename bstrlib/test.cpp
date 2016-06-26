//
// This source file is part of the bstring string library.  This code was
// written by Paul Hsieh in 2002-2015, and is covered by the BSD open source
// license. Refer to the accompanying documentation for details on usage and
// license.
//

//
// test.cpp
//
// This file is the C++ unit test for Bstrlib
//

#if defined (_MSC_VER)
# define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <stdarg.h>
#include "bstrlib.h"
#include "bstrwrap.h"

// Exceptions must be turned on in the compiler to successfully run
// this test.  The compiler must also support STL.

#define dumpOutQty (32)
static bstring dumpOut[dumpOutQty];
static unsigned int rot = 0;

const char * dumpBstring (const bstring b) {
	rot = (rot + 1) % (unsigned) dumpOutQty;
	if (dumpOut[rot] == NULL) {
		dumpOut[rot] = bfromcstr ("");
		if (dumpOut[rot] == NULL) return "FATAL INTERNAL ERROR";
	}
	dumpOut[rot]->slen = 0;
	if (b == NULL) {
		bcatcstr (dumpOut[rot], "NULL");
	} else {
		char msg[32];
		sprintf (msg, "%p", (void *)b);
		bcatcstr (dumpOut[rot], msg);

		if (b->slen < 0) {
			sprintf (msg, ":[err:slen=%d<0]", b->slen);
			bcatcstr (dumpOut[rot], msg);
		} else {
			if (b->mlen > 0 && b->mlen < b->slen) {
				sprintf (msg, ":[err:mlen=%d<slen=%d]", b->mlen, b->slen);
				bcatcstr (dumpOut[rot], msg);
			} else {
				if (b->mlen == -1) {
					bcatcstr (dumpOut[rot], "[p]");
				} else if (b->mlen < 0) {
					bcatcstr (dumpOut[rot], "[c]");
				}
				bcatcstr (dumpOut[rot], ":");
				if (b->data == NULL) {
					bcatcstr (dumpOut[rot], "[err:data=NULL]");
				} else {
					bcatcstr (dumpOut[rot], "\"");
					bcatcstr (dumpOut[rot], (const char *) b->data);
					bcatcstr (dumpOut[rot], "\"");
				}
			}
		}
	}
	return (const char *) dumpOut[rot]->data;
}

int test0 (void) {
int ret = 0;

	printf ("TEST: CBString constructor\n");

	try {
		printf ("\tCBString c;\n");
		CBString c0;
		ret += (0 != c0.length());
		ret += '\0' != ((const char *)c0)[c0.length()];

		printf ("\tCBString c(\"test\");\n");
		CBString c1 ("test");
		ret += (c1 != "test");
		ret += '\0' != ((const char *)c1)[c1.length()];

		printf ("\tCBString c(25, \"test\");\n");
		CBString c8 (25, "test");
		ret += (c8 != "test");
		ret += c8.mlen < 25;
		ret += '\0' != ((const char *)c8)[c8.length()];

		printf ("\tCBString c('t');\n");
		CBString c2 ('t');
		ret += (c2 != "t");
		ret += '\0' != ((const char *)c2)[c2.length()];

		printf ("\tCBString c('\\0');\n");
		CBString c3 ('\0');
		ret += (1 != c3.length()) || ('\0' != c3[0]);
		ret += '\0' != ((const char *)c3)[c3.length()];

		printf ("\tCBString c(bstr[\"test\"]);\n");
		struct tagbstring t = bsStatic ("test");
		CBString c4 (t);
		ret += (c4 != t.data);
		ret += '\0' != ((const char *)c4)[c4.length()];

		printf ("\tCBString c(CBstr[\"test\"]);\n");
		CBString c5 (c1);
		ret += (c1 != c5);
		ret += '\0' != ((const char *)c5)[c5.length()];

		printf ("\tCBString c('x',5);\n");
		CBString c6 ('x',5);
		ret += (c6 != "xxxxx");
		ret += '\0' != ((const char *)c6)[c6.length()];

		printf ("\tCBString c(\"123456\",4);\n");
		CBString c7 ((void *)"123456",4);
		ret += (c7 != "1234");
		ret += '\0' != ((const char *)c7)[c7.length()];
	}

	catch (struct CBStringException err) {
		printf ("Exception thrown [%d]: %s\n", __LINE__, err.what());
		ret ++;
	}

	printf ("\t# failures: %d\n", ret);
	return ret;
}

#define EXCEPTION_EXPECTED(line) 			\
	try {						\
		line;					\
		ret++;					\
		printf ("\tException was expected\n");	\
	}						\
	catch (struct CBStringException) { }

int test1 (void) {
int ret = 0;

	printf ("TEST: CBString = operator\n");

	try {
		CBString c0;
		struct tagbstring t = bsStatic ("test");

		ret += c0.iswriteprotected();
		c0.writeprotect ();
		ret += 1 != c0.iswriteprotected();
		EXCEPTION_EXPECTED (c0 = 'x');
		EXCEPTION_EXPECTED (c0 = (unsigned char) 'x');
		EXCEPTION_EXPECTED (c0 = "test");
		EXCEPTION_EXPECTED (c0 = CBString ("test"));
		EXCEPTION_EXPECTED (c0 = t);
	}
	catch (struct CBStringException err) {
		printf ("Exception thrown [%d]: %s\n", __LINE__, err.what());
		ret ++;
	}

	try {
		CBString c0, c1;
		struct tagbstring t = bsStatic ("test");

		printf ("\tc = 'x';\n");
		c0 = 'x';
		ret += (c0 != "x");
		ret += '\0' != ((const char *)c0)[c0.length()];
		printf ("\tc = (unsigned char)'x';\n");
		c0 = (unsigned char) 'x';
		ret += (c0 != "x");
		ret += '\0' != ((const char *)c0)[c0.length()];
		printf ("\tc = \"test\";\n");
		c0 = "test";
		ret += (c0 != "test");
		ret += '\0' != ((const char *)c0)[c0.length()];
		printf ("\tc = CBStr[\"test\"];\n");
		c1 = c0;
		ret += (c0 != c1);
		ret += '\0' != ((const char *)c1)[c1.length()];
		printf ("\tc = tbstr[\"test\"];\n");
		c0 = t;
		ret += (c0 != "test");
		ret += '\0' != ((const char *)c0)[c0.length()];
	}

	catch (struct CBStringException err) {
		printf ("Exception thrown [%d]: %s\n", __LINE__, err.what());
		ret ++;
	}

	printf ("\t# failures: %d\n", ret);
	return ret;
}

int test2 (void) {
int ret = 0;

	printf ("TEST: CBString += operator\n");

	try {
		CBString c0;
		struct tagbstring t = bsStatic ("test");

		c0.writeprotect ();
		EXCEPTION_EXPECTED (c0 += 'x');
		EXCEPTION_EXPECTED (c0 += (unsigned char) 'x');
		EXCEPTION_EXPECTED (c0 += "test");
		EXCEPTION_EXPECTED (c0 += CBString ("test"));
		EXCEPTION_EXPECTED (c0 += t);
	}
	catch (struct CBStringException err) {
		printf ("Exception thrown [%d]: %s\n", __LINE__, err.what());
		ret ++;
	}

	try {
		CBString c0;
		struct tagbstring t = bsStatic ("extra");

		c0 = "test";
		printf ("\tc += 'x';\n");
		c0 += 'x';
		ret += (c0 != "testx");
		ret += '\0' != ((const char *)c0)[c0.length()];
		printf ("\tc += (unsigned char)'x';\n");
		c0 += (unsigned char) 'y';
		ret += (c0 != "testxy");
		ret += '\0' != ((const char *)c0)[c0.length()];
		printf ("\tc += \"test\";\n");
		c0 += "test";
		ret += (c0 != "testxytest");
		ret += '\0' != ((const char *)c0)[c0.length()];
		printf ("\tc += CBStr[\"test\"];\n");
		c0 += CBString (c0);
		ret += (c0 != "testxytesttestxytest");
		ret += '\0' != ((const char *)c0)[c0.length()];
		printf ("\tc += tbstr[\"test\"];\n");
		c0 += t;
		ret += (c0 != "testxytesttestxytestextra");
		ret += '\0' != ((const char *)c0)[c0.length()];
	}

	catch (struct CBStringException err) {
		printf ("Exception thrown [%d]: %s\n", __LINE__, err.what());
		ret ++;
	}

	printf ("\t# failures: %d\n", ret);
	return ret;
}

int test3 (void) {
int ret = 0;

	try {
		CBString c0, c1;
		struct tagbstring t = bsStatic ("extra");

		printf ("TEST: CBString + operator\n");

		c1 = "test";
		printf ("\tc + 'x';\n");
		c0 = c1 + 'x';
		ret += (c0 != "testx");
		ret += '\0' != ((const char *)c0)[c0.length()];
		printf ("\tc + (unsigned char)'x';\n");
		c0 = c1 + (unsigned char) 'y';
		ret += (c0 != "testy");
		ret += '\0' != ((const char *)c0)[c0.length()];
		printf ("\tc + \"test\";\n");
		c0 = c1 + (const char *) "stuff";
		ret += (c0 != "teststuff");
		ret += '\0' != ((const char *)c0)[c0.length()];
		printf ("\tc + (unsigned char *) \"test\";\n");
		c0 = c1 + (const unsigned char *) "stuff";
		ret += (c0 != "teststuff");
		ret += '\0' != ((const char *)c0)[c0.length()];
		printf ("\tc + CBStr[\"test\"];\n");
		c0 = c1 + CBString ("other");
		ret += (c0 != "testother");
		ret += '\0' != ((const char *)c0)[c0.length()];
		printf ("\tc + tbstr[\"test\"];\n");
		c0 = c1 + t;
		ret += (c0 != "testextra");
		ret += '\0' != ((const char *)c0)[c0.length()];

        	printf ("TEST: + CBString operator\n");

		printf ("\t'x' + c;\n");
		c0 = 'x' + c1;
		ret += (c0 != "xtest");
		ret += '\0' != ((const char *)c0)[c0.length()];
		printf ("\t(unsigned char)'y' + c;\n");
		c0 = (unsigned char) 'y' + c1;
		ret += (c0 != "ytest");
		ret += '\0' != ((const char *)c0)[c0.length()];
		printf ("\t\"test\" + c;\n");
		c0 = (const char *) "stuff" + c1;
		ret += (c0 != "stufftest");
		ret += '\0' != ((const char *)c0)[c0.length()];
		printf ("\t(unsigned char *) \"test\" + c;\n");
		c0 = (const unsigned char *) "stuff" + c1;
		ret += (c0 != "stufftest");
		ret += '\0' != ((const char *)c0)[c0.length()];
		printf ("\ttbstr[\"extra\"] + c;\n");
		c0 = t + c1;
		ret += (c0 != "extratest");
		ret += '\0' != ((const char *)c0)[c0.length()];
	}

	catch (struct CBStringException err) {
		printf ("Exception thrown [%d]: %s\n", __LINE__, err.what());
		ret ++;
	}

	printf ("\t# failures: %d\n", ret);
	return ret;
}

int test4 (void) {
int ret = 0;

	try {
		printf ("TEST: CBString == operator\n");

		CBString c0, c1, c2;

		c0 = c1 = "test";
		c2 = "other";

		printf ("\tc == d;\n");
		ret += !(c0 == c1);
		ret +=  (c0 == c2);

		printf ("\tc == \"test\";\n");
		ret += !(c0 == "test");
		ret +=  (c2 == "test");

		printf ("\tc == (unsigned char *) \"test\";\n");
		ret += !(c0 == (unsigned char *) "test");
		ret +=  (c2 == (unsigned char *) "test");
	}

	catch (struct CBStringException err) {
		printf ("Exception thrown [%d]: %s\n", __LINE__, err.what());
		ret ++;
	}

	printf ("\t# failures: %d\n", ret);
	return ret;
}

int test5 (void) {
int ret = 0;

	try {
		printf ("TEST: CBString != operator\n");

		CBString c0, c1, c2;

		c0 = c1 = "test";
		c2 = "other";

		printf ("\tc != d;\n");
		ret +=  (c0 != c1);
		ret += !(c0 != c2);

		printf ("\tc != \"test\";\n");
		ret +=  (c0 != "test");
		ret += !(c2 != "test");

		printf ("\tc != (unsigned char *) \"test\";\n");
		ret +=  (c0 != (unsigned char *) "test");
		ret += !(c2 != (unsigned char *) "test");
	}

	catch (struct CBStringException err) {
		printf ("Exception thrown [%d]: %s\n", __LINE__, err.what());
		ret ++;
	}

	printf ("\t# failures: %d\n", ret);
	return ret;
}

int test6 (void) {
int ret = 0;

	try {
		printf ("TEST: CBString <, <= operators\n");

		CBString c0, c1, c2;

		c0 = c1 = "test";
		c2 = "other";

		printf ("\tc < d;\n");
		ret +=  (c0 < c1);
		ret +=  (c0 < c2);
		ret +=  (c1 < c0);
		ret += !(c2 < c0);

		printf ("\tc <= d;\n");
		ret += !(c0 <= c1);
		ret +=  (c0 <= c2);
		ret += !(c1 <= c0);
		ret += !(c2 <= c0);

		printf ("\tc < \"test\";\n");
		ret +=  (c0 < "test");
		ret +=  (c1 < "test");
		ret += !(c2 < "test");
		ret +=  (c0 < "other");
		ret +=  (c1 < "other");
		ret +=  (c2 < "other");

		printf ("\tc <= \"test\";\n");
		ret += !(c0 <= "test");
		ret += !(c1 <= "test");
		ret += !(c2 <= "test");
		ret +=  (c0 <= "other");
		ret +=  (c1 <= "other");
		ret += !(c2 <= "other");

		printf ("\tc < (unsigned char *) \"test\";\n");
		ret +=  (c0 < (const char *) "test");
		ret +=  (c1 < (const char *) "test");
		ret += !(c2 < (const char *) "test");
		ret +=  (c0 < (const char *) "other");
		ret +=  (c1 < (const char *) "other");
		ret +=  (c2 < (const char *) "other");

		printf ("\tc <= (unsigned char *) \"test\";\n");
		ret += !(c0 <= (const char *) "test");
		ret += !(c1 <= (const char *) "test");
		ret += !(c2 <= (const char *) "test");
		ret +=  (c0 <= (const char *) "other");
		ret +=  (c1 <= (const char *) "other");
		ret += !(c2 <= (const char *) "other");
	}

	catch (struct CBStringException err) {
		printf ("Exception thrown [%d]: %s\n", __LINE__, err.what());
		ret ++;
	}

	printf ("\t# failures: %d\n", ret);
	return ret;
}

int test7 (void) {
int ret = 0;

	try {
		printf ("TEST: CBString >, >= operators\n");

		CBString c0, c1, c2;

		c0 = c1 = "test";
		c2 = "other";

		printf ("\tc >= d;\n");
		ret += !(c0 >= c1);
		ret += !(c0 >= c2);
		ret += !(c1 >= c0);
		ret +=  (c2 >= c0);

		printf ("\tc > d;\n");
		ret +=  (c0 >  c1);
		ret += !(c0 >  c2);
		ret +=  (c1 >  c0);
		ret +=  (c2 >  c0);

		printf ("\tc >= \"test\";\n");
		ret += !(c0 >= "test");
		ret += !(c1 >= "test");
		ret +=  (c2 >= "test");
		ret += !(c0 >= "other");
		ret += !(c1 >= "other");
		ret += !(c2 >= "other");

		printf ("\tc > \"test\";\n");
		ret +=  (c0 >  "test");
		ret +=  (c1 >  "test");
		ret +=  (c2 >  "test");
		ret += !(c0 >  "other");
		ret += !(c1 >  "other");
		ret +=  (c2 >  "other");

		printf ("\tc >= (unsigned char *) \"test\";\n");
		ret += !(c0 >= (const char *) "test");
		ret += !(c1 >= (const char *) "test");
		ret +=  (c2 >= (const char *) "test");
		ret += !(c0 >= (const char *) "other");
		ret += !(c1 >= (const char *) "other");
		ret += !(c2 >= (const char *) "other");

		printf ("\tc > (unsigned char *) \"test\";\n");
		ret +=  (c0 >  (const char *) "test");
		ret +=  (c1 >  (const char *) "test");
		ret +=  (c2 >  (const char *) "test");
		ret += !(c0 >  (const char *) "other");
		ret += !(c1 >  (const char *) "other");
		ret +=  (c2 >  (const char *) "other");
	}

	catch (struct CBStringException err) {
		printf ("Exception thrown [%d]: %s\n", __LINE__, err.what());
		ret ++;
	}

	printf ("\t# failures: %d\n", ret);
	return ret;
}

int test8 (void) {
int ret = 0;

	try {
		printf ("TEST: (const char *) CBString operator\n");

		CBString c0 ("test"), c1 ("other");

		printf ("\t(const char *) CBString\n");
		ret += 0 != memcmp ((const char *) c0,  "test", 5);
		ret += 0 != memcmp ((const char *) c1, "other", 6);

		printf ("\t(const unsigned char *) CBString\n");
		ret += 0 != memcmp ((const unsigned char *) c0,  "test", 5);
		ret += 0 != memcmp ((const unsigned char *) c1, "other", 6);
	}

	catch (struct CBStringException err) {
		printf ("Exception thrown [%d]: %s\n", __LINE__, err.what());
		ret ++;
	}

	printf ("\t# failures: %d\n", ret);
	return ret;
}

int test9 (void) {
int ret = 0;

	try {
		printf ("TEST: (double), (float), (int) CBString operators\n");
		CBString c0 ("1.2e3"), c1("100"), c2("100.55");
		printf ("\t(double) \"%s\"\n", (const char *) c0);
		ret += 1.2e3 != (double) c0;
		printf ("\t(float) \"%s\"\n", (const char *) c0);
		ret += 1.2e3 != (float) c0;
		printf ("\t(int) \"%s\"\n", (const char *) c1);
		ret += 100 != (float) c1;
		printf ("\t(int) \"%s\"\n", (const char *) c2);
		ret += 100 != (int) c2;
		printf ("\t(unsigned int) \"%s\"\n", (const char *) c2);
		ret += 100 != (unsigned int) c2;
	}
	catch (struct CBStringException err) {
		printf ("Exception thrown [%d]: %s\n", __LINE__, err.what());
		ret ++;
	}

	try {
		CBString c0 ("xxxxx");
		printf ("\t(double) \"%s\"\n", (const char *) c0);
		ret += -1.2e3 != (double) c0;
	}
	catch (struct CBStringException err) {
		printf ("\tException (%s) correctly thrown\n", err.what());
	}

	try {
		CBString c0 ("xxxxx");
		printf ("\t(float) \"%s\"\n", (const char *) c0);
		ret += -1.2e3 != (float) c0;
	}
	catch (struct CBStringException err) {
		printf ("\tException (%s) correctly thrown\n", err.what());
	}

	try {
		CBString c0 ("xxxxx");
		printf ("\t(int) \"%s\"\n", (const char *) c0);
		ret += -100 != (int) c0;
	}
	catch (struct CBStringException err) {
		printf ("\tException (%s) correctly thrown\n", err.what());
	}

	try {
		CBString c0 ("xxxxx");
		printf ("\t(unsigned int) \"%s\"\n", (const char *) c0);
		ret += 1000 != (unsigned int) c0;
	}
	catch (struct CBStringException err) {
		printf ("\tException (%s) correctly thrown\n", err.what());
	}

	printf ("\t# failures: %d\n", ret);
	return ret;
}

int test10 (void) {
int ret = 0;

	try {
		printf ("TEST: length() method\n");
		CBString c0, c1("Test");

		printf ("\t\"%s\".length();\n", (const char *) c0);
		ret += 0 != c0.length();
		printf ("\t\"%s\".length();\n", (const char *) c1);
		ret += 4 != c1.length();
	}

	catch (struct CBStringException err) {
		printf ("Exception thrown [%d]: %s\n", __LINE__, err.what());
		ret ++;
	}

	printf ("\t# failures: %d\n", ret);
	return ret;
}

int test11 (void) {
int ret = 0;

	printf ("TEST: character() method, [] operator\n");

	try {
		CBString c0("test");
		c0.writeprotect ();
		ret += c0[0] != 't';
		ret += (1 + c0[0]) != 'u';
		ret += ((unsigned char) c0[0] + 1) != 'u';
		ret += c0.character(0) != 't';
		EXCEPTION_EXPECTED (c0[0] = 'x');
		EXCEPTION_EXPECTED (c0.character(0) = 'x');
	}
	catch (struct CBStringException err) {
		printf ("Exception thrown [%d]: %s\n", __LINE__, err.what());
		ret ++;
	}

	try {
		CBString c0("Test");

		printf ("\t\"%s\".character ();\n", (const char *) c0);
		ret += 's' != c0.character (2);
		c0.character (2) = 'x';
		ret += c0 != "Text";

		printf ("\t\"%s\"[];\n", (const char *) c0);
		ret += 'T' != c0[0];
		c0[0] = 't';
		ret += c0 != "text";
	}
	catch (struct CBStringException err) {
		printf ("Exception thrown [%d]: %s\n", __LINE__, err.what());
		ret ++;
	}

	try {
		CBString c0;
		printf ("\t\"%s\".character ();\n", (const char *) c0);
		ret += '?' != c0.character (0);
	}
	catch (struct CBStringException err) {
		printf ("\tException (%s) correctly thrown\n", err.what());
	}

	try {
		CBString c0;
		printf ("\t\"%s\"[];\n", (const char *) c0);
		ret += '?' != c0[0];
	}
	catch (struct CBStringException err) {
		printf ("\tException (%s) correctly thrown\n", err.what());
	}

	printf ("\t# failures: %d\n", ret);
	return ret;
}

int test12 (void) {
int ret = 0;

#ifndef BSTRLIB_NOVSNP
	printf ("TEST: format(), formata() methods\n");

	try {
		CBString c0;

		c0.writeprotect ();
		EXCEPTION_EXPECTED (c0.format  ("%s(%d)", "extra", 4));
		EXCEPTION_EXPECTED (c0.formata ("%s(%d)", "extra", 4));
	}
	catch (struct CBStringException err) {
		printf ("Exception thrown [%d]: %s\n", __LINE__, err.what());
		ret ++;
	}

	try {
		CBString c0, c1("Test"), c2, c3;

		printf ("\tc.format (...);\n");
		c0.format ("%s(%d)", "extra", 4);
		ret += c0 != "extra(4)";

		c2  = c0 + c0 + c0 + c0;
		c2 += c2;
		c2.insert (0, "x");
		c3.format ("x%s%s%s%s%s%s%s%s", (const char *) c0, (const char *) c0
		                              , (const char *) c0, (const char *) c0
		                              , (const char *) c0, (const char *) c0
		                              , (const char *) c0, (const char *) c0);
		ret += c2 != c3;

		printf ("\t\"%s\".formata (...);\n", (const char *) c1);
		c1.formata ("%s(%d)", "extra", 4);
		ret += c1 != "Testextra(4)";

		c2  = c0 + c0 + c0 + c0;
		c2 += c2;
		c2.insert (0, "x");
		c3 = "x";
		c3.formata ("%s%s%s%s%s%s%s%s", (const char *) c0, (const char *) c0
		                              , (const char *) c0, (const char *) c0
		                              , (const char *) c0, (const char *) c0
		                              , (const char *) c0, (const char *) c0);
		ret += c2 != c3;
	}
	catch (struct CBStringException err) {
		printf ("Exception thrown [%d]: %s\n", __LINE__, err.what());
		ret ++;
	}

	printf ("\t# failures: %d\n", ret);
#endif
	return ret;
}

int test13 (void) {
int ret = 0;

	try {
		printf ("TEST: find() method\n");
		CBString c0, c1("Test");

		printf ("\t\"%s\".find (CBString());\n", (const char *) c0);
		ret += -1 != c0.find (CBString("x"));
		ret +=  1 != c1.find (CBString("e"));

		printf ("\t\"%s\".find (char *);\n", (const char *) c0);
		ret += -1 != c0.find ("x");
		ret +=  1 != c1.find ("e");

		ret +=  8 != CBString ("sssssssssap").find ("sap");
		ret +=  9 != CBString ("sssssssssap").find ("ap");
		ret +=  9 != CBString ("sssssssssap").find ("ap", 3);
		ret +=  9 != CBString ("sssssssssap").find ("a");
		ret +=  9 != CBString ("sssssssssap").find ("a", 3);
		ret += -1 != CBString ("sssssssssap").find ("x");
		ret += -1 != CBString ("sssssssssap").find ("x", 3);
		ret += -1 != CBString ("sssssssssap").find ("ax");
		ret += -1 != CBString ("sssssssssap").find ("ax", 3);
		ret += -1 != CBString ("sssssssssap").find ("sax");
		ret += -1 != CBString ("sssssssssap").find ("sax", 1);
		ret +=  8 != CBString ("sssssssssap").find ("sap", 3);
		ret +=  9 != CBString ("ssssssssssap").find ("sap", 3);
		ret +=  0 != CBString ("sssssssssap").find ("s");
		ret +=  3 != CBString ("sssssssssap").find ("s", 3);
		ret +=  9 != CBString ("sssssssssap").find ("a");
		ret +=  9 != CBString ("sssssssssap").find ("a", 5);
		ret +=  8 != CBString ("sasasasasap").find ("sap");
		ret +=  9 != CBString ("ssasasasasap").find ("sap");

		printf ("\t\"%s\".find (char);\n", (const char *) c0);
		ret += -1 != c0.find ('x');
		ret +=  1 != c1.find ('e');

		printf ("TEST: reversefind () method\n");
		printf ("\t\"%s\".reversefind (CBString());\n", (const char *) c0);
		ret += -1 != c0.reversefind (CBString("x"), c0.length());
		ret +=  1 != c1.reversefind (CBString("e"), c1.length());

		printf ("\t\"%s\".reversefind (char *);\n", (const char *) c0);
		ret += -1 != c0.reversefind ("x", c0.length());
		ret +=  1 != c1.reversefind ("e", c1.length());

		printf ("\t\"%s\".reversefind (char);\n", (const char *) c0);
		ret += -1 != c0.reversefind ('x', c0.length());
		ret +=  1 != c1.reversefind ('e', c1.length());

	}
	catch (struct CBStringException err) {
		printf ("Exception thrown [%d]: %s\n", __LINE__, err.what());
		ret ++;
	}

	printf ("\t# failures: %d\n", ret);
	return ret;
}

int test14 (void) {
int ret = 0;

	try {
		printf ("TEST: findchr(), reversefindchr() methods\n");
		CBString c0, c1("Test");

		printf ("\t\"%s\".findchr (CBString(\"abcdef\"));\n", (const char *) c0);
		ret += -1 != c0.findchr (CBString ("abcdef"));
		printf ("\t\"%s\".findchr (CBString(\"abcdef\"));\n", (const char *) c1);
		ret +=  1 != c1.findchr (CBString ("abcdef"));
		printf ("\t\"%s\".findchr (\"abcdef\");\n", (const char *) c0);
		ret += -1 != c0.findchr ("abcdef");
		printf ("\t\"%s\".findchr (\"abcdef\");\n", (const char *) c1);
		ret +=  1 != c1.findchr ("abcdef");

		printf ("\t\"%s\".reversefindchr (CBString(\"abcdef\"));\n", (const char *) c0);
		ret += -1 != c0.reversefindchr (CBString ("abcdef"), c0.length());
		printf ("\t\"%s\".reversefindchr (CBString(\"abcdef\"));\n", (const char *) c1);
		ret +=  1 != c1.reversefindchr (CBString ("abcdef"), c1.length());
		printf ("\t\"%s\".reversefindchr (\"abcdef\");\n", (const char *) c0);
		ret += -1 != c0.reversefindchr ("abcdef", c0.length());
		printf ("\t\"%s\".reversefindchr (\"abcdef\");\n", (const char *) c1);
		ret +=  1 != c1.reversefindchr ("abcdef", c1.length());

	}
	catch (struct CBStringException err) {
		printf ("Exception thrown [%d]: %s\n", __LINE__, err.what());
		ret ++;
	}

	printf ("\t# failures: %d\n", ret);
	return ret;
}

int test15 (void) {
int ret = 0;

	try {
		printf ("TEST: nfindchr(), nreversefindchr() methods\n");
		CBString c0, c1("Test");

		printf ("\t\"%s\".nfindchr (CBString(\"abcdef\"));\n", (const char *) c0);
		ret += -1 != c0.nfindchr (CBString ("abcdef"));
		printf ("\t\"%s\".nfindchr (CBString(\"abcdef\"));\n", (const char *) c1);
		ret +=  0 != c1.nfindchr (CBString ("abcdef"));
		printf ("\t\"%s\".nfindchr (\"abcdef\");\n", (const char *) c0);
		ret += -1 != c0.nfindchr ("abcdef");
		printf ("\t\"%s\".nfindchr (\"abcdef\");\n", (const char *) c1);
		ret +=  0 != c1.nfindchr ("abcdef");

		printf ("\t\"%s\".nreversefindchr (CBString(\"abcdef\"));\n", (const char *) c0);
		ret += -1 != c0.nreversefindchr (CBString ("abcdef"), c0.length());
		printf ("\t\"%s\".nreversefindchr (CBString(\"abcdef\"));\n", (const char *) c1);
		ret +=  3 != c1.nreversefindchr (CBString ("abcdef"), c1.length());
		printf ("\t\"%s\".nreversefindchr (\"abcdef\");\n", (const char *) c0);
		ret += -1 != c0.nreversefindchr ("abcdef", c0.length());
		printf ("\t\"%s\".nreversefindchr (\"abcdef\");\n", (const char *) c1);
		ret +=  3 != c1.nreversefindchr ("abcdef", c1.length());

	}
	catch (struct CBStringException err) {
		printf ("Exception thrown [%d]: %s\n", __LINE__, err.what());
		ret ++;
	}

	printf ("\t# failures: %d\n", ret);
	return ret;
}

int test16 (void) {
int ret = 0;

	printf ("TEST: midstr() method\n");

	try {
		CBString c0, c1("bogus"), c2;

		printf ("\t\"%s\".midstr (1,3)\n", (const char *) c0);
		ret += (c2 = c0.midstr (1,3)) != "";
		ret += '\0' != ((const char *)c2)[c2.length ()];

		printf ("\t\"%s\".midstr (1,3)\n", (const char *) c1);
		ret += (c2 = c1.midstr (1,3)) != "ogu";
		ret += '\0' != ((const char *)c2)[c2.length ()];
	}

	catch (struct CBStringException err) {
		printf ("Exception thrown [%d]: %s\n", __LINE__, err.what());
		ret ++;
	}

	printf ("\t# failures: %d\n", ret);
	return ret;
}

int test17 (void) {
int ret = 0;

	printf ("TEST: fill() method\n");

	try {
		CBString c0;

		c0.writeprotect ();
		EXCEPTION_EXPECTED (c0.fill (5, 'x'));
	}
	catch (struct CBStringException err) {
		printf ("Exception thrown [%d]: %s\n", __LINE__, err.what());
		ret ++;
	}

	try {
		CBString c0, c1("Test-test");

		printf ("\t\"%s\".fill(5,'x')\n", (const char *) c0);
		c0.fill (5, 'x');
		ret += c0 != "xxxxx";

		printf ("\t\"%s\".fill(5,'x')\n", (const char *) c1);
		c1.fill (5, 'x');
		ret += c1 != "xxxxx";
	}
	catch (struct CBStringException err) {
		printf ("Exception thrown [%d]: %s\n", __LINE__, err.what());
		ret ++;
	}

	printf ("\t# failures: %d\n", ret);
	return ret;
}

int test18 (void) {
int ret = 0;

	printf ("TEST: alloc() method\n");

	try {
		CBString c0;

		c0.writeprotect ();
		EXCEPTION_EXPECTED (c0.alloc (5));
	}
	catch (struct CBStringException err) {
		printf ("Exception thrown [%d]: %s\n", __LINE__, err.what());
		ret ++;
	}

	try {
		CBString c0, c1("Test-test");

		printf ("\t\"%s\".alloc(5)\n", (const char *) c0);
		c0.alloc (5);
		ret += c0 != "";

		printf ("\t\"%s\".alloc(5)\n", (const char *) c1);
		c1.alloc (5);
		ret += c1 != "Test-test";
	}
	catch (struct CBStringException err) {
		printf ("Exception thrown [%d]: %s\n", __LINE__, err.what());
		ret ++;
	}

	try {
		CBString c0;

		printf ("\t\"%s\".alloc(0)\n", (const char *) c0);
		c0.alloc (0);
		ret += c0 != "Error";
	}
	catch (struct CBStringException err) {
		printf ("\tException (%s) properly thrown\n", err.what());
	}

	try {
		CBString c0;

		printf ("\t\"%s\".alloc(-1)\n", (const char *) c0);
		c0.alloc (-1);
		ret += c0 != "Error";
	}
	catch (struct CBStringException err) {
		printf ("\tException (%s) properly thrown\n", err.what());
	}

	printf ("\t# failures: %d\n", ret);
	return ret;
}

int test19 (void) {
int ret = 0;

	printf ("TEST: setsubstr() method\n");

	try {
		CBString c0("Test-test");

		c0.writeprotect ();
		EXCEPTION_EXPECTED (c0.setsubstr (4, "extra"));
	}
	catch (struct CBStringException err) {
		printf ("Exception thrown [%d]: %s\n", __LINE__, err.what());
		ret ++;
	}

	try {
		CBString c0, c1("Test-test");

		printf ("\t\"%s\".setsubstr (4,\"extra\")\n", (const char *) c0);
		c0.setsubstr (4, "extra");
		ret += c0 != "    extra";
		printf ("\t\"%s\".setsubstr (4,\"extra\")\n", (const char *) c1);
		c1.setsubstr (4, "extra");
		ret += c1 != "Testextra";

	}
	catch (struct CBStringException err) {
		printf ("Exception thrown [%d]: %s\n", __LINE__, err.what());
		ret ++;
	}

	try {
		CBString c0;

		printf ("\t\"%s\".setsubstr(-1,\"extra\")\n", (const char *) c0);
		c0.setsubstr (-1, "extra");
		ret ++;
	}
	catch (struct CBStringException err) {
		printf ("\tException (%s) properly thrown\n", err.what());
	}

	printf ("\t# failures: %d\n", ret);
	return ret;
}

int test20 (void) {
int ret = 0;

	printf ("TEST: insert() method\n");

	try {
		CBString c0("Test-test");

		c0.writeprotect ();
		EXCEPTION_EXPECTED (c0.insert (4, "extra"));
	}
	catch (struct CBStringException err) {
		printf ("Exception thrown [%d]: %s\n", __LINE__, err.what());
		ret ++;
	}

	try {
		CBString c0, c1("Test-test");

		printf ("\t\"%s\".insert (4,\"extra\")\n", (const char *) c0);
		c0.insert (4, "extra");
		ret += c0 != "    extra";
		printf ("\t\"%s\".insert (4,\"extra\")\n", (const char *) c1);
		c1.insert (4, "extra");
		ret += c1 != "Testextra-test";

	}
	catch (struct CBStringException err) {
		printf ("Exception thrown [%d]: %s\n", __LINE__, err.what());
		ret ++;
	}

	try {
		CBString c0;

		printf ("\t\"%s\".insert(-1,\"extra\")\n", (const char *) c0);
		c0.insert (-1, "extra");
		ret ++;
	}
	catch (struct CBStringException err) {
		printf ("\tException (%s) properly thrown\n", err.what());
	}

	printf ("\t# failures: %d\n", ret);
	return ret;
}

int test21 (void) {
int ret = 0;

	printf ("TEST: insertchrs() method\n");

	try {
		CBString c0("Test-test");

		c0.writeprotect ();
		EXCEPTION_EXPECTED (c0.insertchrs (4, 2, 'x'));
	}
	catch (struct CBStringException err) {
		printf ("Exception thrown [%d]: %s\n", __LINE__, err.what());
		ret ++;
	}

	try {
		CBString c0, c1("Test-test");

		printf ("\t\"%s\".insertchrs (4,2,'x')\n", (const char *) c0);
		c0.insertchrs (4, 2, 'x');
		ret += c0 != "xxxxxx";
		printf ("\t\"%s\".insertchrs (4,2,'x')\n", (const char *) c1);
		c1.insertchrs (4, 2, 'x');
		ret += c1 != "Testxx-test";
	}
	catch (struct CBStringException err) {
		printf ("Exception thrown [%d]: %s\n", __LINE__, err.what());
		ret ++;
	}

	try {
		CBString c0;

		printf ("\t\"%s\".insertchrs (-1,2,'x')\n", (const char *) c0);
		c0.insertchrs (-1, 2, 'x');
		ret ++;
	}
	catch (struct CBStringException err) {
		printf ("\tException (%s) properly thrown\n", err.what());
	}

	printf ("\t# failures: %d\n", ret);
	return ret;
}

int test22 (void) {
int ret = 0;

	printf ("TEST: replace() method\n");

	try {
		CBString c0("Test-test");

		c0.writeprotect ();
		EXCEPTION_EXPECTED (c0.replace (4, 2, "beef"));
	}
	catch (struct CBStringException err) {
		printf ("Exception thrown [%d]: %s\n", __LINE__, err.what());
		ret ++;
	}

	try {
		CBString c0, c1("Test-test");

		printf ("\t\"%s\".replace (4,2,\"beef\")\n", (const char *) c0);
		c0.replace (4, 2, CBString ("beef"));
		ret += c0 != "    beef";
		c0 = "";
		c0.replace (4, 2, "beef");
		ret += c0 != "    beef";

		printf ("\t\"%s\".replace (4,2,\"beef\")\n", (const char *) c1);
		c1.replace (4, 2, CBString ("beef"));
		ret += c1 != "Testbeefest";
		c1 = "Test-test";
		c1.replace (4, 2, "beef");
		ret += c1 != "Testbeefest";
	}
	catch (struct CBStringException err) {
		printf ("Exception thrown [%d]: %s\n", __LINE__, err.what());
		ret ++;
	}

	printf ("\t# failures: %d\n", ret);
	return ret;
}

int test23 (void) {
int ret = 0;

	printf ("TEST: findreplace() method\n");

	try {
		CBString c0, c1("Test-test");

		printf ("\t\"%s\".findreplace (\"est\",\"beef\")\n", (const char *) c0);
		c0.findreplace ("est", "beef");
		ret += c0 != "";
		c0 = "";
		c0.findreplace (CBString ("est"), CBString ("beef"));
		ret += c0 != "";

		printf ("\t\"%s\".findreplace (\"est\",\"beef\")\n", (const char *) c1);
		c1.findreplace ("est", "beef");
		ret += c1 != "Tbeef-tbeef";
		c1 = "Test-test";
		c1.findreplace (CBString ("est"), CBString ("beef"));
		ret += c1 != "Tbeef-tbeef";

	}
	catch (struct CBStringException err) {
		printf ("Exception thrown [%d]: %s\n", __LINE__, err.what());
		ret ++;
	}

	try {
		CBString c0, c1("TeSt-tEsT");

		printf ("\t\"%s\".findreplacecaseless (\"est\",\"beef\")\n", (const char *) c0);
		c0.findreplacecaseless ("est", "beef");
		ret += c0 != "";
		c0 = "";
		c0.findreplacecaseless (CBString ("est"), CBString ("beef"));
		ret += c0 != "";

		printf ("\t\"%s\".findreplacecaseless (\"est\",\"beef\")\n", (const char *) c1);
		c1.findreplacecaseless ("est", "beef");
		ret += c1 != "Tbeef-tbeef";
		c1 = "Test-test";
		c1.findreplacecaseless (CBString ("est"), CBString ("beef"));
		ret += c1 != "Tbeef-tbeef";

	}
	catch (struct CBStringException err) {
		printf ("Exception thrown [%d]: %s\n", __LINE__, err.what());
		ret ++;
	}

	printf ("\t# failures: %d\n", ret);
	return ret;
}

int test24 (void) {
int ret = 0;

	printf ("TEST: remove() method\n");

	try {
		CBString c0, c1("Test-test");

		printf ("\t\"%s\".remove (4,2)\n", (const char *) c0);
		c0.remove (4, 2);
		ret += c0 != "";

		printf ("\t\"%s\".remove (4,2)\n", (const char *) c1);
		c1.remove (4, 2);
		ret += c1 != "Testest";
	}
	catch (struct CBStringException err) {
		printf ("Exception thrown [%d]: %s\n", __LINE__, err.what());
		ret ++;
	}

	printf ("\t# failures: %d\n", ret);
	return ret;
}

int test25 (void) {
int ret = 0;

	printf ("TEST: trunc() method\n");

	try {
		CBString c0, c1("Test-test");

		printf ("\t\"%s\".trunc (4)\n", (const char *) c0);
		c0.trunc (4);
		ret += c0 != "";

		printf ("\t\"%s\".trunc (4)\n", (const char *) c1);
		c1.trunc (4);
		ret += c1 != "Test";
	}
	catch (struct CBStringException err) {
		printf ("Exception thrown [%d]: %s\n", __LINE__, err.what());
		ret ++;
	}

	printf ("\t# failures: %d\n", ret);
	return ret;
}

int test26 (void) {
int ret = 0;

	printf ("TEST: repeat() method\n");

	try {
		CBString c0, c1("Test");

		printf ("\t\"%s\".repeat (4)\n", (const char *) c0);
		c0.repeat (4);
		ret += c0 != "";

		printf ("\t\"%s\".repeat (4)\n", (const char *) c1);
		c1.repeat (4);
		ret += c1 != "TestTestTestTest";
		c1 = "Test";
		c1.repeat (4);
		ret += c1 != "TestTestTestTest";
	}
	catch (struct CBStringException err) {
		printf ("Exception thrown [%d]: %s\n", __LINE__, err.what());
		ret ++;
	}

	printf ("\t# failures: %d\n", ret);
	return ret;
}

int test27 (void) {
int ret = 0;

	printf ("TEST: ltrim(), rtrim() methods\n");

	try {
		CBString c0, c1("  Test  "), c2("       ");

		printf ("\t\"%s\".ltrim ()\n", (const char *) c0);
		c0.ltrim ();
		ret += c0 != "";
		c0 = "";
		c0.rtrim ();
		ret += c0 != "";

		printf ("\t\"%s\".ltrim ()\n", (const char *) c1);
		c1.ltrim ();
		ret += c1 != "Test  ";
		c1 = "  Test  ";
		c1.rtrim ();
		ret += c1 != "  Test";

		printf ("\t\"%s\".ltrim ()\n", (const char *) c2);
		c2.ltrim ();
		ret += c2 != "";
		c2 = "       ";
		c2.rtrim ();
		ret += c2 != "";
	}
	catch (struct CBStringException err) {
		printf ("Exception thrown [%d]: %s\n", __LINE__, err.what());
		ret ++;
	}

	printf ("\t# failures: %d\n", ret);
	return ret;
}

#if !defined(BSTRLIB_CANNOT_USE_STL)

int test28 (void) {
int ret = 0;

	printf ("TEST: split(), join() mechanisms\n");

	try {
		CBString c0, c1("a b c d e f");
		struct CBStringList s;
		s.split (c1, ' ');

		c0.writeprotect ();
		EXCEPTION_EXPECTED (c0.join (s));
	}
	catch (struct CBStringException err) {
		printf ("Exception thrown [%d]: %s\n", __LINE__, err.what());
		ret ++;
	}

	try {
		CBString c0, c1("a b c d e f");
		struct CBStringList s;

		printf ("\t\"%s\".split (' ')\n", (const char *) c1);

		s.split (c1, ' ');
		CBString c2(s), c3(s, ',');

		printf ("\tc.join (<...>)\n");

		ret += c2 != "abcdef";
		ret += c3 != "a,b,c,d,e,f";
		c0.join (s);
		ret += c0 != "abcdef";
		c0.join (s, ',');
		ret += c0 != "a,b,c,d,e,f";

		CBString strPepe = "valor1@valor2@valor3@@@valor6";
		for (unsigned char c = (unsigned char) '\0';;c++) {
			CBStringList sl;
			CBString x;

			sl.split (strPepe, c);
			x.join (sl, c);
			if (x != strPepe) {
				printf ("\tfailure[%d] split/join mismatch\n\t\t%s\n\t\t%s\n", __LINE__, (const char *) strPepe, (const char *) x);
				ret++;
				break;
			}
			if (UCHAR_MAX == c) break;
		}

		{
			CBStringList sl;
			CBString x;

			sl.splitstr (strPepe, CBString ("or"));
			x.join (sl, CBString ("or"));
			if (x != strPepe) {
				printf ("\tfailure[%d] splitstr/join mismatch\n\t\t%s\n\t\t%s\n", __LINE__, (const char *) strPepe, (const char *) x);
				ret++;
			}
		}

		{
			CBStringList sl;
			CBString x;

			sl.splitstr (strPepe, CBString ("6"));
			x.join (sl, CBString ("6"));
			if (x != strPepe) {
				printf ("\tfailure[%d] splitstr/join mismatch\n\t\t%s\n\t\t%s\n", __LINE__, (const char *) strPepe, (const char *) x);
				ret++;
			}
		}

		{
			CBStringList sl;
			CBString x;

			sl.splitstr (strPepe, CBString ("val"));
			x.join (sl, CBString ("val"));
			if (x != strPepe) {
				printf ("\tfailure[%d] splitstr/join mismatch\n\t\t%s\n\t\t%s\n", __LINE__, (const char *) strPepe, (const char *) x);
				ret++;
			}
		}

		{
			CBStringList sl;
			CBString x;

			sl.splitstr (strPepe, CBString ("@@"));
			x.join (sl, CBString ("@@"));
			if (x != strPepe) {
				printf ("\tfailure[%d] splitstr/join mismatch\n\t\t%s\n\t\t%s\n", __LINE__, (const char *) strPepe, (const char *) x);
				ret++;
			}
		}

	}
	catch (struct CBStringException err) {
		printf ("Exception thrown [%d]: %s\n", __LINE__, err.what());
		ret ++;
	}

	printf ("\t# failures: %d\n", ret);
	return ret;
}

#endif

int test29 (void) {
int ret = 0;

	printf ("TEST: caselessEqual(), caselessCmp() mechanisms\n");

	try {
		CBString c0("Test"), c1("test"), c2("testy");

		printf ("\t\"%s\".caselessEqual (\"%s\")\n", (const char *) c0, (const char *) c1);
		ret += 1 != c0.caselessEqual (c1);
		ret += 1 != c1.caselessEqual (c0);
		printf ("\t\"%s\".caselessEqual (\"%s\")\n", (const char *) c0, (const char *) c2);
		ret += 0 != c0.caselessEqual (c2);
		ret += 0 != c2.caselessEqual (c0);

		printf ("\t\"%s\".caselessCmp (\"%s\")\n", (const char *) c0, (const char *) c1);
		ret += 0 != c0.caselessCmp (c1);
		ret += 0 != c1.caselessCmp (c0);
		printf ("\t\"%s\".caselessCmp (\"%s\")\n", (const char *) c0, (const char *) c2);
		ret += 0 == c0.caselessCmp (c2);
		ret += 0 == c2.caselessCmp (c0);
	}
	catch (struct CBStringException err) {
		printf ("Exception thrown [%d]: %s\n", __LINE__, err.what());
		ret ++;
	}

	printf ("\t# failures: %d\n", ret);
	return ret;
}

int test30 (void) {
int ret = 0;

	printf ("TEST: toupper(), tolower() mechanisms\n");

	try {
		CBString c0("Test-test");

		c0.writeprotect ();
		EXCEPTION_EXPECTED (c0.toupper());
		EXCEPTION_EXPECTED (c0.tolower());
	}
	catch (struct CBStringException err) {
		printf ("Exception thrown [%d]: %s\n", __LINE__, err.what());
		ret ++;
	}

	try {
		CBString c0;

		c0 = "Test";
		printf ("\t\"%s\".toupper ()\n", (const char *) c0);
		c0.toupper();
		ret += c0 != "TEST";

		c0 = "Test";
		printf ("\t\"%s\".tolower ()\n", (const char *) c0);
		c0.tolower ();
		ret += c0 != "test";
	}
	catch (struct CBStringException err) {
		printf ("Exception thrown [%d]: %s\n", __LINE__, err.what());
		ret ++;
	}

	printf ("\t# failures: %d\n", ret);
	return ret;
}

static size_t test31_aux (void *buff, size_t elsize, size_t nelem, void *parm) {
	buff = buff;
	elsize = elsize;
	nelem = nelem;
	parm = parm;
	return 0;
}

int test31 (void) {
CBString c;
int ret = 0;

	printf ("TEST: CBStream test\n");

	CBStream s((bNread) test31_aux, NULL);
	s << CBString("Test");

	ret += ((c = s.read ()) != CBString ("Test"));
	ret += !s.eof();

	printf ("\t\"%s\" through CBStream.read()\n", (const char *) c);

	s << CBString("Test");

	c.trunc (0);
	ret += ((s >> c) != CBString ("Test"));
	ret += !s.eof();

	printf ("\t\"%s\" through CBStream.>>\n", (const char *) c);

	return ret;
}

/*  int bMultiConcatNeedNULLAsLastArgument (bstring dst, ...)
 *
 *  Concatenate a sequence of exactly n char * arguments to dst.
 */
int bMultiConcatNeedNULLAsLastArgument (bstring dst, ...) {
va_list arglist;
int ret = 0;
	va_start (arglist, dst);
	do {
		bstring parm = va_arg (arglist, bstring);
		if (NULL == parm) break;
		if (NULL == parm->data || parm->slen > parm->mlen ||
		    parm->mlen < 0 || parm->slen < 0) {
			ret = BSTR_ERR;
			break;
		}
		ret = bconcat (dst, parm);
	} while (0 <= ret);
	va_end (arglist);
	return ret;
}

/*  int bMultiCatCstrNeedNULLAsLastArgument (bstring dst, ...)
 *
 *  Concatenate a sequence of exactly n char * arguments to dst.
 */
int bMultiCatCstrNeedNULLAsLastArgument (bstring dst, ...) {
va_list arglist;
int ret = 0;
	va_start (arglist, dst);
	do {
		char* parm = va_arg (arglist, char *);
		if (NULL == parm) break;
		ret = bcatcstr (dst, parm);
	} while (0 <= ret);
	va_end (arglist);
	return ret;
}

/*
 * The following macros are only available on more recent compilers that
 * support variable length macro arguments and __VA_ARGS__.  These can also
 * be dangerous because there is no compiler time type checking on the 
 * arguments.
 */


#define bMultiConcat(dst,...)  bMultiConcatNeedNULLAsLastArgument((dst),##__VA_ARGS__,NULL)
#define bMultiCatCstr(dst,...) bMultiCatCstrNeedNULLAsLastArgument((dst),##__VA_ARGS__,NULL)

#define bGlue3_aux(a,b,c) a ## b ## c
#define bGlue3(a,b,c)     bGlue3_aux(a,b,c)

#if defined(_MSC_VER)
#define _bDeclTbstrIdx(t,n,...) \
	static unsigned char bGlue3(_btmpuc_,t,n)[] = {__VA_ARGS__, '\0'}; \
	struct tagbstring t = { -32, sizeof(bGlue3(_btmpuc_,t,n))-1, bGlue3(_btmpuc_,t,n)}
#define bDeclTbstr(t,...) _bDeclTbstrIdx(t,__COUNTER__,__VA_ARGS__)
#else
#define bDeclTbstr(t,...) \
	static unsigned char bGlue3(_btmpuc_,t,__LINE__)[] = {__VA_ARGS__, '\0'}; \
	struct tagbstring t = { -__LINE__, sizeof(bGlue3(_btmpuc_,t,__LINE__))-1, bGlue3(_btmpuc_,t,__LINE__)}
#endif

static int test32(void) {
bstring b1 = bfromStatic ("a");
bstring b2 = bfromStatic ("e");
bstring b3 = bfromStatic ("i");
bstring b4 = bfromStatic ("");
int ret = 0;

	printf ("TEST: bMultiCatCstr, bMultiConcat\n");

	bMultiCatCstr(b1, "b", "c", "d");
	bMultiCatCstr(b2, "f", "g", "h");
	bMultiCatCstr(b3, "j", "k", "l");
	bMultiConcat(b4, b1, b2, b3);

	ret += 1 != biseqStatic (b1, "abcd");
	ret += 1 != biseqStatic (b2, "efgh");
	ret += 1 != biseqStatic (b3, "ijkl");
	ret += 1 != biseqStatic (b4, "abcdefghijkl");

	bdestroy (b1);
	bdestroy (b2);
	bdestroy (b3);
	bdestroy (b4);

	printf ("\t# failures: %d\n", ret);
	return ret;
}

static int test33(void) {
	bDeclTbstr (t1, 'H','e','l','l','o');
	bDeclTbstr (t2, 32,'w','o','r','l','d');
	bstring b = bfromStatic("[");
	int ret;

	printf ("TEST: bDeclTbstr\n");

	bconcat (b, &t1);
	bconcat (b, &t2);
	bcatStatic (b, "]");
	ret = 1 != biseqStatic (b, "[Hello world]");
	bdestroy (b);

	printf ("\t# failures: %d\n", ret);
	return ret;
}

int main () {
int ret = 0;

	printf ("Direct case testing of CPP core functions\n");

	ret += test0 ();
	ret += test1 ();
	ret += test2 ();
	ret += test3 ();
	ret += test4 ();
	ret += test5 ();
	ret += test6 ();
	ret += test7 ();
	ret += test8 ();
	ret += test9 ();
	ret += test10 ();
	ret += test11 ();
	ret += test12 ();
	ret += test13 ();
	ret += test14 ();
	ret += test15 ();
	ret += test16 ();
	ret += test17 ();
	ret += test18 ();
	ret += test19 ();
	ret += test20 ();
	ret += test21 ();
	ret += test22 ();
	ret += test23 ();
	ret += test24 ();
	ret += test25 ();
	ret += test26 ();
	ret += test27 ();
#if !defined(BSTRLIB_CANNOT_USE_STL)
	ret += test28 ();
#endif
	ret += test29 ();
	ret += test30 ();
	ret += test31 ();
	ret += test32 ();
	ret += test33 ();

	printf ("# test failures: %d\n", ret);

	return 0;
}

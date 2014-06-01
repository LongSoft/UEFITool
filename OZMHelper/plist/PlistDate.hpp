//
//	 PlistDate class appropriate for Apple Property Lists (plists).  Part of
//	 the PlistCpp Apple Property List (plist) serialization and parsing
//	 library.  
//
//	 https://github.com/animetrics/PlistCpp
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

#ifndef __PLIST_DATE_H__
#define __PLIST_DATE_H__
#include <ctime>
#include <string>

namespace Plist {

class Date
{
	public:
		Date();

		Date(int month, int day, int year, int hour24, int minute, int second, bool UTC);

		void set(int month, int day, int year, int hour24, int minute, int second, bool UTC);

		void setToCurrentTime();

		time_t secondsSinceDate(const Date& startDate) const;

		// returns -1 : first < second
		//          0 : first = second
		//          1 : first > second

		static int compare(const Date& first, const Date& second);

		bool operator > (const Date& rhs) const;

		bool operator < (const Date& rhs) const;

		bool operator == (const Date& rhs) const;

		// iso 8601 date string convention
		std::string timeAsXMLConvention() const;

		// iso 8601 date string convention
		void setTimeFromXMLConvention(const std::string& timeString);

	// Apple epoch is # of seconds since  01-01-2001. So we need to add the
	// number of seconds since 01-01-1970 which is proper unix epoch

		void setTimeFromAppleEpoch(double appleTime);

		time_t timeAsEpoch() const;

	// We need to subtract the number of seconds between 01-01-2001 and
	// 01-01-1970 to get Apple epoch from unix epoch

		double timeAsAppleEpoch() const;


	private:

		// stored as unix epoch, number of seconds since 01-01-1970
		time_t _time;
};

}

#endif

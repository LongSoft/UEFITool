

//  base64.hpp 
//  Autor Konstantin Pilipchuk
//  mailto:lostd@ukr.net
//
//

#if !defined(__BASE64_HPP_INCLUDED__)
#define __BASE64_HPP_INCLUDED__ 1

#pragma once

#include <iterator>

static
int _base64Chars[]= {'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
				     'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z',
			         '0','1','2','3','4','5','6','7','8','9',
			         '+','/' };


#define _0000_0011 0x03
#define _1111_1100 0xFC
#define _1111_0000 0xF0
#define _0011_0000 0x30
#define _0011_1100 0x3C
#define _0000_1111 0x0F
#define _1100_0000 0xC0
#define _0011_1111 0x3F

#define _EQUAL_CHAR   (-1)
#define _UNKNOWN_CHAR (-2)

#define _IOS_FAILBIT   std::ios_base::failbit
#define _IOS_EOFBIT    std::ios_base::eofbit
#define _IOS_BADBIT    std::ios_base::badbit
#define _IOS_GOODBIT   std::ios_base::goodbit

// TEMPLATE CLASS base64_put
template<class _E = char, class _Tr = std::char_traits<_E> >
class base64
{
public:

	typedef unsigned char byte_t;
	typedef _E            char_type;
	typedef _Tr           traits_type; 

	// base64 requires max line length <= 72 characters
	// you can fill end of line
	// it may be crlf, crlfsp, noline or other class like it

	struct lf
	{
		template<class _OI>
			_OI operator()(_OI _To) const{
			*_To = _Tr::to_char_type('\n'); ++_To;

			return (_To);
			}
	};

	struct crlf
	{
		template<class _OI>
			_OI operator()(_OI _To) const{
			*_To = _Tr::to_char_type('\r'); ++_To;
			*_To = _Tr::to_char_type('\n'); ++_To;

			return (_To);
		}
	};


	struct crlfsp
	{
		template<class _OI>
			_OI operator()(_OI _To) const{
			*_To = _Tr::to_char_type('\r'); ++_To;
			*_To = _Tr::to_char_type('\n'); ++_To;
			*_To = _Tr::to_char_type(' '); ++_To;

			return (_To);
		}
	};

	struct noline
	{
		template<class _OI>
			_OI operator()(_OI _To) const{
			return (_To);
		}
	};

	struct three2four
	{
		void zero()
		{
			_data[0] = 0;
			_data[1] = 0;
			_data[2] = 0;
		}

		byte_t get_0()	const
		{
			return _data[0];
		}
		byte_t get_1()	const
		{
			return _data[1];
		}
		byte_t get_2()	const
		{
			return _data[2];
		}

		void set_0(byte_t _ch)
		{
			_data[0] = _ch;
		}

		void set_1(byte_t _ch)
		{
			_data[1] = _ch;
		}

		void set_2(byte_t _ch)
		{
			_data[2] = _ch;
		}

		// 0000 0000  1111 1111  2222 2222
		// xxxx xxxx  xxxx xxxx  xxxx xxxx
		// 0000 0011  1111 2222  2233 3333

		int b64_0()	const	{return (_data[0] & _1111_1100) >> 2;}
		int b64_1()	const	{return ((_data[0] & _0000_0011) << 4) + ((_data[1] & _1111_0000)>>4);}
		int b64_2()	const	{return ((_data[1] & _0000_1111) << 2) + ((_data[2] & _1100_0000)>>6);}
		int b64_3()	const	{return (_data[2] & _0011_1111);}

		void b64_0(int _ch)	{_data[0] = ((_ch & _0011_1111) << 2) | (_0000_0011 & _data[0]);}

		void b64_1(int _ch)	{
			_data[0] = ((_ch & _0011_0000) >> 4) | (_1111_1100 & _data[0]);
			_data[1] = ((_ch & _0000_1111) << 4) | (_0000_1111 & _data[1]);	}

		void b64_2(int _ch)	{
			_data[1] = ((_ch & _0011_1100) >> 2) | (_1111_0000 & _data[1]);
			_data[2] = ((_ch & _0000_0011) << 6) | (_0011_1111 & _data[2]);	}

		void b64_3(int _ch){
			_data[2] = (_ch & _0011_1111) | (_1100_0000 & _data[2]);}

	private:
		byte_t _data[3];

	};




	template<class _II, class _OI, class _State, class _Endline>
		_II put(_II _First, _II _Last, _OI _To, _State& _St, _Endline _Endl)  const
	{
		three2four _3to4;
		int line_octets = 0;

		while(_First != _Last)
		{
			_3to4.zero();

			// берём по 3 символа
			_3to4.set_0(*_First);
			_First++;

			if(_First == _Last)
			{
				*_To = _Tr::to_char_type(_base64Chars[_3to4.b64_0()]); ++_To;
				*_To = _Tr::to_char_type(_base64Chars[_3to4.b64_1()]); ++_To;
				*_To = _Tr::to_char_type('='); ++_To;
				*_To = _Tr::to_char_type('='); ++_To;
				goto __end;
			}

			_3to4.set_1(*_First);
			_First++;

			if(_First == _Last)
			{
				*_To = _Tr::to_char_type(_base64Chars[_3to4.b64_0()]); ++_To;
				*_To = _Tr::to_char_type(_base64Chars[_3to4.b64_1()]); ++_To;
				*_To = _Tr::to_char_type(_base64Chars[_3to4.b64_2()]); ++_To;
				*_To = _Tr::to_char_type('='); ++_To;
				goto __end;
			}

			_3to4.set_2(*_First);
			_First++;

			*_To = _Tr::to_char_type(_base64Chars[_3to4.b64_0()]); ++_To;
			*_To = _Tr::to_char_type(_base64Chars[_3to4.b64_1()]); ++_To;
			*_To = _Tr::to_char_type(_base64Chars[_3to4.b64_2()]); ++_To;
			*_To = _Tr::to_char_type(_base64Chars[_3to4.b64_3()]); ++_To;

			if(line_octets == 17) // base64 позволяет длину строки не более 72 символов
			{
				_To = _Endl(_To);

				line_octets = 0;
			}
			else
				++line_octets;
		}

		__end: ;

		return (_First);

	}


	template<class _II, class _OI, class _State>
		_II get(_II _First, _II _Last, _OI _To, _State& _St) const
	{
		three2four _3to4;
		int _Char;

		while(_First != _Last)
		{

			// Take octet
			_3to4.zero();

			// -- 0 --
			// Search next valid char... 
			while((_Char =  _getCharType(*_First)) < 0 && _Char == _UNKNOWN_CHAR)
			{
				if(++_First == _Last)
				{
					_St |= _IOS_FAILBIT|_IOS_EOFBIT; return _First; // unexpected EOF
				}
			}

			if(_Char == _EQUAL_CHAR){
				// Error! First character in octet can't be '='
				_St |= _IOS_FAILBIT; 
				return _First; 
			}
			else
				_3to4.b64_0(_Char);


			// -- 1 --
			// Search next valid char... 
			while(++_First != _Last)
				if((_Char = _getCharType(*_First)) != _UNKNOWN_CHAR)
					break;

			if(_First == _Last)	{
				_St |= _IOS_FAILBIT|_IOS_EOFBIT; // unexpected EOF 
				return _First;
			}

			if(_Char == _EQUAL_CHAR){
				// Error! Second character in octet can't be '='
				_St |= _IOS_FAILBIT; 
				return _First; 
			}
			else
				_3to4.b64_1(_Char);


			// -- 2 --
			// Search next valid char... 
			while(++_First != _Last)
				if((_Char = _getCharType(*_First)) != _UNKNOWN_CHAR)
					break;

			if(_First == _Last)	{
				// Error! Unexpected EOF. Must be '=' or base64 character
				_St |= _IOS_FAILBIT|_IOS_EOFBIT; 
				return _First; 
			}

			if(_Char == _EQUAL_CHAR){
				// OK!
				_3to4.b64_2(0); 
				_3to4.b64_3(0); 

				// chek for EOF
				if(++_First == _Last)
				{
					// Error! Unexpected EOF. Must be '='. Ignore it.
					//_St |= _IOS_BADBIT|_IOS_EOFBIT;
					_St |= _IOS_EOFBIT;
				}
				else 
					if(_getCharType(*_First) != _EQUAL_CHAR)
					{
						// Error! Must be '='. Ignore it.
						//_St |= _IOS_BADBIT;
					}
				else
					++_First; // Skip '='

				// write 1 byte to output
				*_To = (byte_t) _3to4.get_0();
				return _First;
			}
			else
				_3to4.b64_2(_Char);


			// -- 3 --
			// Search next valid char... 
			while(++_First != _Last)
				if((_Char = _getCharType(*_First)) != _UNKNOWN_CHAR)
					break;

			if(_First == _Last)	{
				// Unexpected EOF. It's error. But ignore it.
				//_St |= _IOS_FAILBIT|_IOS_EOFBIT; 
					_St |= _IOS_EOFBIT; 
				
				return _First; 
			}

			if(_Char == _EQUAL_CHAR)
			{
				// OK!
				_3to4.b64_3(0); 

				// write to output 2 bytes
				*_To = (byte_t) _3to4.get_0();
				*_To = (byte_t) _3to4.get_1();

				++_First; // set position to next character

				return _First;
			}
			else
				_3to4.b64_3(_Char);


			// write to output 3 bytes
			*_To = (byte_t) _3to4.get_0();
			*_To = (byte_t) _3to4.get_1();
			*_To = (byte_t) _3to4.get_2();

			++_First;
			

		} // while(_First != _Last)

		return (_First);
	}

protected:

	int _getCharType(int C) const
	{
		if(_base64Chars[62] == C)
			return 62;

		if(_base64Chars[63] == C)
			return 63;

		if((_base64Chars[0] <= C) && (_base64Chars[25] >= C))
			return C - _base64Chars[0];

		if((_base64Chars[26] <= C) && (_base64Chars[51] >= C))
			return C - _base64Chars[26] + 26;

		if((_base64Chars[52] <= C) && (_base64Chars[61] >= C))
			return C - _base64Chars[52] + 52;

		if(C == _Tr::to_int_type('='))
			return _EQUAL_CHAR;

		return _UNKNOWN_CHAR;
	}


};


#endif

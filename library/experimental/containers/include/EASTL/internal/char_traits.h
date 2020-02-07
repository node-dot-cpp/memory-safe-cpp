/* -------------------------------------------------------------------------------
* Copyright (c) 2019, OLogN Technologies AG
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of the OLogN Technologies AG nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL OLogN Technologies AG BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
* -------------------------------------------------------------------------------*/

// Initial vesion from:
// https://github.com/electronicarts/EASTL/blob/3.15.00/include/EASTL/internal/char_trait.h

/////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// This file implements similar functionality to char_traits which is part of
// the C++ standard STL library specification. This is intended for internal
// EASTL use only.  Functionality can be accessed through the eastl::string or
// eastl::string_view types.  
//
// http://en.cppreference.com/w/cpp/string/char_traits
///////////////////////////////////////////////////////////////////////////////

#ifndef EASTL_CHAR_TRAITS_H
#define EASTL_CHAR_TRAITS_H

#if defined(EA_PRAGMA_ONCE_SUPPORTED)
	#pragma once
#endif

#include <EASTL/internal/config.h>
#include <EASTL/type_traits.h>

//EA_DISABLE_ALL_VC_WARNINGS()
#include <ctype.h>              // toupper, etc.
#include <string.h>             // memset, etc.
//EA_RESTORE_ALL_VC_WARNINGS()

namespace nodecpp
{
	///////////////////////////////////////////////////////////////////////////////
	/// DecodePart
	///
	/// These implement UTF8/UCS2/UCS4 encoding/decoding.
	///
	EASTL_API bool DecodePart(const char*& pSrc, const char* pSrcEnd, char*&     pDest, char*     pDestEnd);
	EASTL_API bool DecodePart(const char*& pSrc, const char* pSrcEnd, char16_t*& pDest, char16_t* pDestEnd);
	EASTL_API bool DecodePart(const char*& pSrc, const char* pSrcEnd, char32_t*& pDest, char32_t* pDestEnd);

	EASTL_API bool DecodePart(const char16_t*& pSrc, const char16_t* pSrcEnd, char*&     pDest, char*     pDestEnd);
	EASTL_API bool DecodePart(const char16_t*& pSrc, const char16_t* pSrcEnd, char16_t*& pDest, char16_t* pDestEnd);
	EASTL_API bool DecodePart(const char16_t*& pSrc, const char16_t* pSrcEnd, char32_t*& pDest, char32_t* pDestEnd);

	EASTL_API bool DecodePart(const char32_t*& pSrc, const char32_t* pSrcEnd, char*&     pDest, char*     pDestEnd);
	EASTL_API bool DecodePart(const char32_t*& pSrc, const char32_t* pSrcEnd, char16_t*& pDest, char16_t* pDestEnd);
	EASTL_API bool DecodePart(const char32_t*& pSrc, const char32_t* pSrcEnd, char32_t*& pDest, char32_t* pDestEnd);

	EASTL_API bool DecodePart(const int*& pSrc, const int* pSrcEnd, char*&     pDest, char*     pDestEnd);
	EASTL_API bool DecodePart(const int*& pSrc, const int* pSrcEnd, char16_t*& pDest, char16_t* pDestEnd);
	EASTL_API bool DecodePart(const int*& pSrc, const int* pSrcEnd, char32_t*& pDest, char32_t* pDestEnd);

	#if EA_CHAR8_UNIQUE
		bool DecodePart(const char8_t*& pSrc, const char8_t* pSrcEnd, char*&     pDest, char*     pDestEnd);
		bool DecodePart(const char8_t*& pSrc, const char8_t* pSrcEnd, char16_t*& pDest, char16_t* pDestEnd);
		bool DecodePart(const char8_t*& pSrc, const char8_t* pSrcEnd, char32_t*& pDest, char32_t* pDestEnd);
	#endif

	// #if EA_WCHAR_UNIQUE
		bool DecodePart(const wchar_t*& pSrc, const wchar_t* pSrcEnd, char*&     pDest, char*     pDestEnd);
		bool DecodePart(const wchar_t*& pSrc, const wchar_t* pSrcEnd, char16_t*& pDest, char16_t* pDestEnd);
		bool DecodePart(const wchar_t*& pSrc, const wchar_t* pSrcEnd, char32_t*& pDest, char32_t* pDestEnd);

		bool DecodePart(const char*&     pSrc, const char*     pSrcEnd, wchar_t*& pDest, wchar_t* pDestEnd);
		bool DecodePart(const char16_t*& pSrc, const char16_t* pSrcEnd, wchar_t*& pDest, wchar_t* pDestEnd);
		bool DecodePart(const char32_t*& pSrc, const char32_t* pSrcEnd, wchar_t*& pDest, wchar_t* pDestEnd);
	// #endif

	#if EA_CHAR8_UNIQUE //&& EA_WCHAR_UNIQUE
		bool DecodePart(const char8_t*& pSrc, const char8_t* pSrcEnd, wchar_t*& pDest, wchar_t* pDestEnd);
		bool DecodePart(const wchar_t*& pSrc, const wchar_t* pSrcEnd, char8_t*& pDest, char8_t* pDestEnd);
	#endif

static_assert((sizeof(wchar_t) == sizeof(char16_t)) || (sizeof(wchar_t) == sizeof(char32_t)), "bad sizeof(wchar_t)"); 

	// #if EA_WCHAR_UNIQUE
		inline bool DecodePart(const wchar_t*& pSrc, const wchar_t* pSrcEnd, char*& pDest, char* pDestEnd)
		{
		if constexpr (sizeof(wchar_t) == sizeof(char16_t))
			return DecodePart(reinterpret_cast<const char16_t*&>(pSrc), reinterpret_cast<const char16_t*>(pSrcEnd), pDest, pDestEnd);
		else
			return DecodePart(reinterpret_cast<const char32_t*&>(pSrc), reinterpret_cast<const char32_t*>(pSrcEnd), pDest, pDestEnd);
		}

		inline bool DecodePart(const wchar_t*& pSrc, const wchar_t* pSrcEnd, char16_t*& pDest, char16_t* pDestEnd)
		{
		if constexpr (sizeof(wchar_t) == sizeof(char16_t))
			return DecodePart(reinterpret_cast<const char16_t*&>(pSrc), reinterpret_cast<const char16_t*>(pSrcEnd), pDest, pDestEnd);
		else
			return DecodePart(reinterpret_cast<const char32_t*&>(pSrc), reinterpret_cast<const char32_t*>(pSrcEnd), pDest, pDestEnd);
		}

		inline bool DecodePart(const wchar_t*& pSrc, const wchar_t* pSrcEnd, char32_t*& pDest, char32_t* pDestEnd)
		{
		if constexpr (sizeof(wchar_t) == sizeof(char16_t))
			return DecodePart(reinterpret_cast<const char16_t*&>(pSrc), reinterpret_cast<const char16_t*>(pSrcEnd), pDest, pDestEnd);
		else
			return DecodePart(reinterpret_cast<const char32_t*&>(pSrc), reinterpret_cast<const char32_t*>(pSrcEnd), pDest, pDestEnd);
		}

		inline bool DecodePart(const char*& pSrc, const char* pSrcEnd, wchar_t*& pDest, wchar_t* pDestEnd)
		{
		if constexpr (sizeof(wchar_t) == sizeof(char16_t))
			return DecodePart(pSrc, pSrcEnd, reinterpret_cast<char16_t*&>(pDest), reinterpret_cast<char16_t*>(pDestEnd));
		else
			return DecodePart(pSrc, pSrcEnd, reinterpret_cast<char32_t*&>(pDest), reinterpret_cast<char32_t*>(pDestEnd));
		}

		inline bool DecodePart(const char16_t*& pSrc, const char16_t* pSrcEnd, wchar_t*& pDest, wchar_t* pDestEnd)
		{
		if constexpr (sizeof(wchar_t) == sizeof(char16_t))
			return DecodePart(pSrc, pSrcEnd, reinterpret_cast<char16_t*&>(pDest), reinterpret_cast<char16_t*>(pDestEnd));
		else
			return DecodePart(pSrc, pSrcEnd, reinterpret_cast<char32_t*&>(pDest), reinterpret_cast<char32_t*>(pDestEnd));
		}

		inline bool DecodePart(const char32_t*& pSrc, const char32_t* pSrcEnd, wchar_t*& pDest, wchar_t* pDestEnd)
		{
		if constexpr (sizeof(wchar_t) == sizeof(char16_t))
			return DecodePart(pSrc, pSrcEnd, reinterpret_cast<char16_t*&>(pDest), reinterpret_cast<char16_t*>(pDestEnd));
		else
			return DecodePart(pSrc, pSrcEnd, reinterpret_cast<char32_t*&>(pDest), reinterpret_cast<char32_t*>(pDestEnd));
		}
	// #endif

	#if EA_CHAR8_UNIQUE
	    inline bool DecodePart(const char8_t*& pSrc, const char8_t* pSrcEnd, char*& pDest, char* pDestEnd)
	    {
		    return DecodePart(reinterpret_cast<const char*&>(pSrc), reinterpret_cast<const char*>(pSrcEnd), pDest, pDestEnd);
	    }

	    inline bool DecodePart(const char8_t*& pSrc, const char8_t* pSrcEnd, char16_t*& pDest, char16_t* pDestEnd)
	    {
		    return DecodePart(reinterpret_cast<const char*&>(pSrc), reinterpret_cast<const char*>(pSrcEnd), pDest, pDestEnd);
	    }

	    inline bool DecodePart(const char8_t*& pSrc, const char8_t* pSrcEnd, char32_t*& pDest, char32_t* pDestEnd)
	    {
		    return DecodePart(reinterpret_cast<const char*&>(pSrc), reinterpret_cast<const char*>(pSrcEnd), pDest, pDestEnd);
	    }
    #endif

	#if EA_CHAR8_UNIQUE //&& EA_WCHAR_UNIQUE
		inline bool DecodePart(const char8_t*&  pSrc, const char8_t*  pSrcEnd, wchar_t*&  pDest, wchar_t*  pDestEnd)
		{
		#if (EA_WCHAR_SIZE == 2)
		    return DecodePart(pSrc, pSrcEnd, reinterpret_cast<char16_t*&>(pDest), reinterpret_cast<char16_t*>(pDestEnd));
		#elif (EA_WCHAR_SIZE == 4)
		    return DecodePart(pSrc, pSrcEnd, reinterpret_cast<char32_t*&>(pDest), reinterpret_cast<char32_t*>(pDestEnd));
		#endif
		}

		inline bool DecodePart(const wchar_t*&  pSrc, const wchar_t*  pSrcEnd, char8_t*&  pDest, char8_t*  pDestEnd)
		{
		#if (EA_WCHAR_SIZE == 2)
			return DecodePart(reinterpret_cast<const char16_t*&>(pSrc), reinterpret_cast<const char16_t*>(pSrcEnd), reinterpret_cast<char*&>(pDest), reinterpret_cast<char*>(pDestEnd));
		#elif (EA_WCHAR_SIZE == 4)
			return DecodePart(reinterpret_cast<const char32_t*&>(pSrc), reinterpret_cast<const char32_t*>(pSrcEnd), reinterpret_cast<char*&>(pDest), reinterpret_cast<char*>(pDestEnd));
		#endif
		}
	#endif

	///////////////////////////////////////////////////////////////////////////////
	// 'char traits' functionality
	//
	inline char CharToLower(char c)
		{ return (char)tolower((uint8_t)c); }

	template<typename T>
	inline T CharToLower(T c)
		{ if((unsigned)c <= 0xff) return (T)tolower((uint8_t)c); return c; }


	inline char CharToUpper(char c)
		{ return (char)toupper((uint8_t)c); }

	template<typename T>
	inline T CharToUpper(T c)
		{ if((unsigned)c <= 0xff) return (T)toupper((uint8_t)c); return c; }


	template <typename T>
	int Compare(const T* p1, const T* p2, size_t n)
	{
		for(; n > 0; ++p1, ++p2, --n)
		{
			if(*p1 != *p2)
				return (static_cast<typename make_unsigned<T>::type>(*p1) < 
						static_cast<typename make_unsigned<T>::type>(*p2)) ? -1 : 1;
		}
		return 0;
	}

	inline int Compare(const char* p1, const char* p2, size_t n)
	{
		return memcmp(p1, p2, n);
	}


	template <typename T>
	inline int CompareI(const T* p1, const T* p2, size_t n)
	{
		for(; n > 0; ++p1, ++p2, --n)
		{
			const T c1 = CharToLower(*p1);
			const T c2 = CharToLower(*p2);

			if(c1 != c2)
				return (static_cast<typename make_unsigned<T>::type>(c1) < 
						static_cast<typename make_unsigned<T>::type>(c2)) ? -1 : 1;
		}
		return 0;
	}


	template<typename T>
	inline const T* Find(const T* p, T c, size_t n)
	{
		for(; n > 0; --n, ++p)
		{
			if(*p == c)
				return p;
		}

		return NULL;
	}

	inline const char* Find(const char* p, char c, size_t n)
	{
		return (const char*)memchr(p, c, n);
	}


	template<typename T>
	inline constexpr size_t CharStrlen(const T* p)
	{
		const auto* pCurrent = p;
		while(*pCurrent)
			++pCurrent;
		return (size_t)(pCurrent - p);
	}


	template <typename T>
	inline T* CharStringUninitializedCopy(const T* pSource, const T* pSourceEnd, T* pDestination)
	{
		memmove(pDestination, pSource, (size_t)(pSourceEnd - pSource) * sizeof(T));
		return pDestination + (pSourceEnd - pSource);
	}


	template <typename T>
	const T* CharTypeStringFindEnd(const T* pBegin, const T* pEnd, T c)
	{
		const T* pTemp = pEnd;
		while(--pTemp >= pBegin)
		{
			if(*pTemp == c)
				return pTemp;
		}

		return pEnd;
	}

    
	template <typename T>
	const T* CharTypeStringRSearch(const T* p1Begin, const T* p1End, 
								   const T* p2Begin, const T* p2End)
	{
		// Test for zero length strings, in which case we have a match or a failure, 
		// but the return value is the same either way.
		if((p1Begin == p1End) || (p2Begin == p2End))
			return p1Begin;

		// Test for a pattern of length 1.
		if((p2Begin + 1) == p2End)
			return CharTypeStringFindEnd(p1Begin, p1End, *p2Begin);

		// Test for search string length being longer than string length.
		if((p2End - p2Begin) > (p1End - p1Begin))
			return p1End;

		// General case.
		const T* pSearchEnd = (p1End - (p2End - p2Begin) + 1);
		const T* pCurrent1;
		const T* pCurrent2;

		while(pSearchEnd != p1Begin)
		{
			// Search for the last occurrence of *p2Begin.
			pCurrent1 = CharTypeStringFindEnd(p1Begin, pSearchEnd, *p2Begin);
			if(pCurrent1 == pSearchEnd) // If the first char of p2 wasn't found, 
				return p1End;           // then we immediately have failure.

			// In this case, *pTemp == *p2Begin. So compare the rest.
			pCurrent2 = p2Begin;
			while(*pCurrent1++ == *pCurrent2++)
			{
				if(pCurrent2 == p2End)
					return (pCurrent1 - (p2End - p2Begin));
			}

			// A smarter algorithm might know to subtract more than just one,
			// but in most cases it won't make much difference anyway.
			--pSearchEnd;
		}

		return p1End;
	}


	template <typename T>
	inline const T* CharTypeStringFindFirstOf(const T* p1Begin, const T* p1End, const T* p2Begin, const T* p2End)
	{
		for (; p1Begin != p1End; ++p1Begin)
		{
			for (const T* pTemp = p2Begin; pTemp != p2End; ++pTemp)
			{
				if (*p1Begin == *pTemp)
					return p1Begin;
			}
		}
		return p1End;
	}


	template <typename T>
	inline const T* CharTypeStringRFindFirstNotOf(const T* p1RBegin, const T* p1REnd, const T* p2Begin, const T* p2End)
	{
		for (; p1RBegin != p1REnd; --p1RBegin)
		{
			const T* pTemp;
			for (pTemp = p2Begin; pTemp != p2End; ++pTemp)
			{
				if (*(p1RBegin - 1) == *pTemp)
					break;
			}
			if (pTemp == p2End)
				return p1RBegin;
		}
		return p1REnd;
	}


	template <typename T>
	inline const T* CharTypeStringFindFirstNotOf(const T* p1Begin, const T* p1End, const T* p2Begin, const T* p2End)
	{
		for (; p1Begin != p1End; ++p1Begin)
		{
			const T* pTemp;
			for (pTemp = p2Begin; pTemp != p2End; ++pTemp)
			{
				if (*p1Begin == *pTemp)
					break;
			}
			if (pTemp == p2End)
				return p1Begin;
		}
		return p1End;
	}


	template <typename T>
	inline const T* CharTypeStringRFindFirstOf(const T* p1RBegin, const T* p1REnd, const T* p2Begin, const T* p2End)
	{
		for (; p1RBegin != p1REnd; --p1RBegin)
		{
			for (const T* pTemp = p2Begin; pTemp != p2End; ++pTemp)
			{
				if (*(p1RBegin - 1) == *pTemp)
					return p1RBegin;
			}
		}
		return p1REnd;
	}


	template <typename T>
	inline const T* CharTypeStringRFind(const T* pRBegin, const T* pREnd, const T c)
	{
		while (pRBegin > pREnd)
		{
			if (*(pRBegin - 1) == c)
				return pRBegin;
			--pRBegin;
		}
		return pREnd;
	}


	inline char* CharStringUninitializedFillN(char* pDestination, size_t n, const char c)
	{
		if(n) // Some compilers (e.g. GCC 4.3+) generate a warning (which can't be disabled) if you call memset with a size of 0.
			memset(pDestination, (uint8_t)c, (size_t)n);
		return pDestination + n;
	}

	template<typename T>
	inline T* CharStringUninitializedFillN(T* pDestination, size_t n, const T c)
	{
		T * pDest           = pDestination;
		const T* const pEnd = pDestination + n;
		while(pDest < pEnd)
			*pDest++ = c;
		return pDestination + n;
	}


	inline char* CharTypeAssignN(char* pDestination, size_t n, char c)
	{
		if(n) // Some compilers (e.g. GCC 4.3+) generate a warning (which can't be disabled) if you call memset with a size of 0.
			return (char*)memset(pDestination, c, (size_t)n);
		return pDestination;
	}

	template<typename T>
	inline T* CharTypeAssignN(T* pDestination, size_t n, T c)
	{
		T* pDest            = pDestination;
		const T* const pEnd = pDestination + n;
		while(pDest < pEnd)
			*pDest++ = c;
		return pDestination;
	}
} // namespace nodecpp

#endif // EASTL_CHAR_TRAITS_H

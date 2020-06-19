/* -------------------------------------------------------------------------------
* Copyright (c) 2020, OLogN Technologies AG
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

#ifndef SAFE_MEMORY_STRING_LITERAL_H
#define SAFE_MEMORY_STRING_LITERAL_H


namespace safe_memory
{
	template<typename T>
	class SAFE_MEMORY_DEEP_CONST SAFE_MEMORY_NO_SIDE_EFFECT_WHEN_CONST basic_string_literal
	{
		const T* str;
	public:
		basic_string_literal( const T* str_) : str( str_ ) {}
		basic_string_literal( const basic_string_literal& other ) = default;
		basic_string_literal& operator = ( const basic_string_literal& other ) = default;
		basic_string_literal( basic_string_literal&& other ) = default;
		basic_string_literal& operator = ( basic_string_literal&& other ) = default;

		// bool operator == ( const basic_string_literal& other ) const { return strcmp( str, other.str ) == 0; }
		// bool operator != ( const basic_string_literal& other ) const { return strcmp( str, other.str ) != 0; }

//		bool operator == ( const char* other ) const { return strcmp( str, other.str ) == 0; }
//		bool operator != ( const char* other ) const { return strcmp( str, other.str ) != 0; }

		const T* c_str() const { return str; }
	};

	typedef basic_string_literal<char>    string_literal;
	typedef basic_string_literal<wchar_t> wstring_literal;

	/// string8 / string16 / string32
	// typedef basic_string<char8_t>  string8;
	typedef basic_string_literal<char16_t> string16_literal;
	typedef basic_string_literal<char32_t> string32_literal;

} //namespace safe_memory

#endif //SAFE_MEMORY_STRING_LITERAL_H
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

#ifndef SAFE_MEMORY_STRING_FORMAT_H
#define SAFE_MEMORY_STRING_FORMAT_H

#include <safememory/string.h>
#include <safememory/string_literal.h>
#include <fmt/format.h>
#include <iostream>


template <class T>
struct fmt::formatter<safememory::basic_string_literal<T>>: formatter<std::basic_string_view<T>> {
  // parse is inherited from formatter<string_view>.
    template <typename FormatContext>
    auto format(const safememory::basic_string_literal<T>& str, FormatContext& ctx) -> decltype(ctx.out()) {
        std::basic_string_view<T> sview(str.c_str());
        return formatter<std::basic_string_view<T>>::format(sview, ctx);
    }
};

template<class T>
std::basic_ostream<T>& operator<<(std::basic_ostream<T>& os, const safememory::basic_string_literal<T>& str)
{
  return os << str.c_str();
}

template <class T>
struct fmt::formatter<safememory::basic_string<T>>: formatter<std::basic_string_view<T>> {
  // parse is inherited from formatter<string_view>.
    template <typename FormatContext>
    auto format(const safememory::basic_string<T>& str, FormatContext& ctx) -> decltype(ctx.out()) {
        std::basic_string_view<T> sview(str.c_str(), str.size());
        return formatter<std::basic_string_view<T>>::format(sview, ctx);
    }
};

template<class T>
std::basic_ostream<T>& operator<<(std::basic_ostream<T>& os, const safememory::basic_string<T>& str)
{
  std::basic_string_view<T> sview(str.c_str(), str.size());
  return os << sview;
}

#endif //SAFE_MEMORY_STRING_FORMAT_H

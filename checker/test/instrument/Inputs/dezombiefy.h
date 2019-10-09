/* -------------------------------------------------------------------------------
* Copyright (c) 2018, OLogN Technologies AG
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

#ifndef NODECPP_DEZOMBIEFY_H
#define NODECPP_DEZOMBIEFY_H

#include <utility>

namespace nodecpp::safememory {

template<class T>
auto dezombiefy(T* x) {
	return x;
}

template<class T>
auto dezombiefy(T&& x) {
	return std::forward(x);
}

template<class T1, class T2>
auto mul(T1&& t1, T2&& t2) {
	return std::forward(t1) * std::forward(t2);
}

template<class T1, class T2>
auto div(T1&& t1, T2&& t2) {
	return std::forward(t1) / std::forward(t2);
}

template<class T1, class T2>
auto rem(T1&& t1, T2&& t2) {
	return std::forward(t1) % std::forward(t2);
}

template<class T1, class T2>
auto add(T1&& t1, T2&& t2) {
	return std::forward(t1) + std::forward(t2);
}

template<class T1, class T2>
auto sub(T1&& t1, T2&& t2) {
	return std::forward(t1) - std::forward(t2);
}
/// logical

template<class T1, class T2>
auto lt(T1&& t1, T2&& t2) {
	return std::forward(t1) < std::forward(t2);
}

template<class T1, class T2>
auto gt(T1&& t1, T2&& t2) {
	return std::forward(t1) > std::forward(t2);
}

template<class T1, class T2>
auto le(T1&& t1, T2&& t2) {
	return std::forward(t1) <= std::forward(t2);
}
template<class T1, class T2>
auto ge(T1&& t1, T2&& t2) {
	return std::forward(t1) >= std::forward(t2);
}

template<class T1, class T2>
auto eq(T1&& t1, T2&& t2) {
	return std::forward(t1) == std::forward(t2);
}

template<class T1, class T2>
auto ne(T1&& t1, T2&& t2) {
	return std::forward(t1) != std::forward(t2);
}

// template<class T1, class T2>
// auto cmp(T1&& t1, T2&& t2) {
// 	return t1 <=> t2;
// }

template<class T1, class T2>
auto and(T1&& t1, T2&& t2) {
	return std::forward(t1) & std::forward(t2);
}

template<class T1, class T2>
auto xor(T1&& t1, T2&& t2) {
	return std::forward(t1) ^ std::forward(t2);
}

template<class T1, class T2>
auto or(T1&& t1, T2&& t2) {
	return std::forward(t1) | std::forward(t2);
}

} // namespace nodecpp::safememory


#endif // NODECPP_DEZOMBIEFY_H
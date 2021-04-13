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

#ifndef SAFE_MEMORY_DETAIL_INSTRUMENT_H
#define SAFE_MEMORY_DETAIL_INSTRUMENT_H

#include <safememory/detail/safe_ptr_common.h>
#include <safe_memory_error.h>
#include <utility>

namespace safememory::detail {

// using safememory::detail::isPointerNotZombie;
using nodecpp::error::early_detected_zombie_pointer_access;

#ifndef NODECPP_DISABLE_ZOMBIE_ACCESS_EARLY_DETECTION
template<class T>
T*& dezombiefy(T*& x) {
	if ( NODECPP_LIKELY( isPointerNotZombie( x ) ) )
		return x;
	else
		throw early_detected_zombie_pointer_access; 
}

// template<class T>
// T* const & dezombiefy(T* const & x) {
// 	if ( NODECPP_LIKELY( isPointerNotZombie( x ) ) )
// 		return x;
// 	else
// 		throw early_detected_zombie_pointer_access; 
// }

// template<class T>
// const T*& dezombiefy(const T*& x) {
// 	if ( NODECPP_LIKELY( isPointerNotZombie( const_cast<T*>( x ) ) ) )
// 		return x;
// 	else
// 		throw early_detected_zombie_pointer_access; 
// }

template<class T>
T& dezombiefy(T& x) {
	if ( NODECPP_LIKELY( isPointerNotZombie( &x ) ) )
		return x;
	else
		throw early_detected_zombie_pointer_access; 
}

// template<class T>
// const T& dezombiefy(const T& x) {
// 	if ( NODECPP_LIKELY( isPointerNotZombie( const_cast<T*>( &x ) ) ) )
// 		return x;
// 	else
// 		throw early_detected_zombie_pointer_access; 
// }
#else
#define dezombiefy( x ) (x)
#endif // NODECPP_DISABLE_ZOMBIE_ACCESS_EARLY_DETECTION


/**
 * \brief Dezombiefy functions used by iterators are enabled by template parameter
 * 
 * When a soft_ptr hits stack optimization, it can potencially point to a zombie object
 * as destruction info is not feed back.
 * When we are dozombiefying we must either disable stack optimization or dezombiefy \c soft_ptr
 * same way we do with raw pointers.
 */
template<typename> class soft_ptr_no_checks; //fwd

template<class T>
void dezombiefySoftPtr(const soft_ptr_no_checks<T>& x) { } //TODO

template<typename> class soft_ptr_impl; //fwd

template<class T>
void dezombiefySoftPtr(const soft_ptr_impl<T>& x) { } //TODO

template<class T>
void dezombiefyRawPtr(T* x) {
	if ( NODECPP_UNLIKELY( !isPointerNotZombie( x ) ) )
		throw early_detected_zombie_pointer_access; 
}


template<class T1, class T2>
auto dz_mul(T1&& t1, T2&& t2) {
	return std::forward(t1) * std::forward(t2);
}

template<class T1, class T2>
auto dz_div(T1&& t1, T2&& t2) {
	return std::forward(t1) / std::forward(t2);
}

template<class T1, class T2>
auto dz_rem(T1&& t1, T2&& t2) {
	return std::forward(t1) % std::forward(t2);
}

template<class T1, class T2>
auto dz_add(T1&& t1, T2&& t2) {
	return std::forward(t1) + std::forward(t2);
}

template<class T1, class T2>
auto dz_sub(T1&& t1, T2&& t2) {
	return std::forward(t1) - std::forward(t2);
}
/// logical

template<class T1, class T2>
auto dz_lt(T1&& t1, T2&& t2) {
	return std::forward(t1) < std::forward(t2);
}

template<class T1, class T2>
auto dz_gt(T1&& t1, T2&& t2) {
	return std::forward(t1) > std::forward(t2);
}

template<class T1, class T2>
auto dz_le(T1&& t1, T2&& t2) {
	return std::forward(t1) <= std::forward(t2);
}
template<class T1, class T2>
auto dz_ge(T1&& t1, T2&& t2) {
	return std::forward(t1) >= std::forward(t2);
}

template<class T1, class T2>
auto dz_eq(T1&& t1, T2&& t2) {
	return std::forward(t1) == std::forward(t2);
}

template<class T1, class T2>
auto dz_ne(T1&& t1, T2&& t2) {
	return std::forward(t1) != std::forward(t2);
}

// template<class T1, class T2>
// auto cmp(T1&& t1, T2&& t2) {
// 	return t1 <=> t2;
// }

template<class T1, class T2>
auto dz_and(T1&& t1, T2&& t2) {
	return std::forward(t1) & std::forward(t2);
}

template<class T1, class T2>
auto dz_xor(T1&& t1, T2&& t2) {
	return std::forward(t1) ^ std::forward(t2);
}

template<class T1, class T2>
auto dz_or(T1&& t1, T2&& t2) {
	return std::forward(t1) | std::forward(t2);
}

} // namespace safememory::detail


#endif // SAFE_MEMORY_DETAIL_INSTRUMENT_H
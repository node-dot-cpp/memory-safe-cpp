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

#ifndef SAFE_PTR_H
#define SAFE_PTR_H

#include "safe_ptr_common.h"

#include "safe_ptr_no_checks.h"
#include "safe_ptr_impl.h"

namespace nodecpp::safememory {

template<class T>
struct safeness_declarator {
	static constexpr bool is_safe = true; // by default
};

/* Sample of user-defined exclusion:
template<> struct nodecpp::safememory::safeness_declarator<double> { static constexpr bool is_safe = false; };
*/

template<class T, bool is_safe> struct owning_ptr_type_ { typedef owning_ptr_impl<T> type; };
template<class T> struct owning_ptr_type_<T, false> { typedef owning_ptr_no_checks<T> type; };
template<class T> using owning_ptr = typename owning_ptr_type_<T, safeness_declarator<T>::is_safe>::type;

template<class T, bool is_safe> struct soft_ptr_type_ { typedef soft_ptr_impl<T> type; };
template<class T> struct soft_ptr_type_<T, false> { typedef soft_ptr_no_checks<T> type; };
template<class T> using soft_ptr = typename soft_ptr_type_<T, safeness_declarator<T>::is_safe>::type;

template<class T, bool is_safe> struct soft_this_ptr_type_ { typedef soft_this_ptr_impl<T> type; };
template<class T> struct soft_this_ptr_type_<T, false> { typedef soft_this_ptr_no_checks<T> type; };
template<class T> using soft_this_ptr = typename soft_this_ptr_type_<T, safeness_declarator<T>::is_safe>::type;

template<class T, bool is_safe> struct naked_ptr_type_ { typedef naked_ptr_impl<T> type; };
template<class T> struct naked_ptr_type_<T, false> { typedef naked_ptr_no_checks<T> type; };
template<class T> using naked_ptr = typename naked_ptr_type_<T, safeness_declarator<T>::is_safe>::type;
/*
//namespace typehelper {
//template<class T> struct owning_ptr_type { typedef owning_ptr_impl<T> type; };
template<class T> struct soft_ptr_type { typedef soft_ptr_impl<T> type; };
template<class T> struct soft_this_ptr_type { typedef soft_this_ptr_impl<T> type; };
template<class T> struct naked_ptr_type { typedef naked_ptr_impl<T> type; };

//template<> struct owning_ptr_type<double> { typedef owning_ptr_no_checks<double> type; };
template<> struct soft_ptr_type<double> { typedef soft_ptr_no_checks<double> type; };
template<> struct soft_this_ptr_type<double> { typedef soft_this_ptr_no_checks<double> type; };
template<> struct naked_ptr_type<double> { typedef naked_ptr_no_checks<double> type; };

//template<class T> using owning_ptr = typename owning_ptr_type<T>::type;
//template<class T> using soft_ptr = typename soft_ptr_type<T>::type;
template<class T> using soft_this_ptr = typename soft_this_ptr_type<T>::type;
template<class T> using naked_ptr = typename naked_ptr_type<T>::type;*/


// SAFE VERSION (default case)
/*template<class T> using owning_ptr = owning_ptr_impl<T>;
template<class T> using soft_ptr = soft_ptr_impl<T>;
template<class T> using soft_this_ptr = soft_this_ptr_impl<T>;
template<class T> using naked_ptr = naked_ptr_impl<T>;*/

template<class _Ty,
	class... _Types,
	std::enable_if_t<!std::is_array<_Ty>::value, int> = 0>
//NODISCARD typename owning_ptr_type_<_Ty>::type make_owning(_Types&&... _Args)
NODISCARD owning_ptr<_Ty> make_owning(_Types&&... _Args)
{
	if constexpr ( safeness_declarator<_Ty>::is_safe )
	{
		return make_owning_impl<_Ty, _Types ...>( ::std::forward<_Types>(_Args)... );
	}
	else
	{
		return make_owning_no_checks<_Ty, _Types ...>( ::std::forward<_Types>(_Args)... );
	}
	//return make_owning_impl<_Ty, _Types ...>( ::std::forward<_Types>(_Args)... );
//	typename owning_ptr_type<_Ty>::type ret( make_owning_t(), ::std::forward<_Types>(_Args)... );
//	return ret;
}

template<class T>
soft_ptr<T> soft_ptr_in_constructor(T* ptr) {
	return soft_ptr_in_constructor_impl<T>(ptr);
}

template<class T, class T1>
soft_ptr_impl<T> soft_ptr_static_cast( soft_ptr_impl<T1> p ) {
	return soft_ptr_static_cast_impl<T, T1>( p ) ;
}

template<class T, class T1>
soft_ptr_no_checks<T> soft_ptr_static_cast( soft_ptr_no_checks<T1> p ) {
	return soft_ptr_static_cast_no_checks<T, T1>( p ) ;
}

template<class T, class T1>
soft_ptr_impl<T> soft_ptr_reinterpret_cast( soft_ptr_impl<T1> p ) {
	return soft_ptr_reinterpret_cast_impl<T, T1>( p );
}

template<class T, class T1>
soft_ptr_no_checks<T> soft_ptr_reinterpret_cast( soft_ptr_no_checks<T1> p ) {
	return soft_ptr_reinterpret_cast_no_checks<T, T1>( p );
}

// UNSAFE (FAST) version (per-type)

/*template<> using owning_ptr<int> = owning_ptr_no_checks<int>;
template<> using soft_ptr = soft_ptr_no_checks<int>;
template<> using soft_this_ptr = soft_this_ptr_no_checks<int>;
template<> using naked_ptr = naked_ptr_no_checks<int>;*/

/*template<
	class... _Types,
	std::enable_if_t<!std::is_array<double>::value, int> = 0>
NODISCARD owning_ptr<double> make_owning(_Types&&... _Args)
{
	return make_owning_no_checks<double, _Types ...>( ::std::forward<_Types>(_Args)... );
}
NODISCARD owning_ptr<double> make_owning(double)
{
	return make_owning_no_checks<double>();
}*/

/*template<>
soft_ptr<int> soft_ptr_in_constructor(int* ptr) {
	return soft_ptr_in_constructor_no_checks<int>(ptr);
}

template<class T, class T1>
soft_ptr_no_checks<T> soft_ptr_static_cast( soft_ptr_no_checks<T1> p ) {
	return soft_ptr_static_cast_no_checks<T, T1>( p ) ;
}

template<class T, class T1>
soft_ptr_no_checks<T> soft_ptr_static_cast( soft_ptr_no_checks<T1> p ) {
	return soft_ptr_static_cast_no_checks<T, T1>( p ) ;
}

template<class T, class T1>
soft_ptr_no_checks<T> soft_ptr_reinterpret_cast( soft_ptr_no_checks<T1> p ) {
	return soft_ptr_reinterpret_cast_no_checks<T, T1>( p );
}*/

template<class T>
soft_ptr_no_checks<T> soft_ptr_static_cast( soft_ptr_no_checks<double> p ) {
	return soft_ptr_static_cast_no_checks<T, double>( p ) ;
}

template<class T1>
soft_ptr_no_checks<double> soft_ptr_static_cast( soft_ptr_no_checks<T1> p ) {
	return soft_ptr_static_cast_no_checks<double, T1>( p ) ;
}

template<class T>
soft_ptr_no_checks<T> soft_ptr_reinterpret_cast( soft_ptr_no_checks<double> p ) {
	return soft_ptr_reinterpret_cast_no_checks<T, double>( p );
}

template<class T1>
soft_ptr_no_checks<double> soft_ptr_reinterpret_cast( soft_ptr_no_checks<T1> p ) {
	return soft_ptr_reinterpret_cast_no_checks<double, T1>( p );
}


/*template<class T> using owning_ptr = owning_ptr_no_checks<T>;
template<class T> using soft_ptr = soft_ptr_no_checks<T>;
using soft_this_ptr = soft_this_ptr_no_checks;
template<class T> using naked_ptr = naked_ptr_no_checks<T>;

template<> using owning_ptr = owning_ptr_no_checks<T>;
template<class T> using soft_ptr = soft_ptr_no_checks<T>;
using soft_this_ptr = soft_this_ptr_no_checks;
template<class T> using naked_ptr = naked_ptr_no_checks<T>;*/

/*template<class T>
soft_ptr_no_checks<T> soft_ptr_in_constructor(T* ptr) {
	return soft_ptr_in_constructor_no_check<T>( ptr );
}*/


} // namespace nodecpp::safememory

#include "startup_checks.h"

#endif

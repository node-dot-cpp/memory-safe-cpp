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

#ifndef SAFE_PTR_FOR_MAP_H
#define SAFE_PTR_FOR_MAP_H

#include "safe_ptr.h"

namespace nodecpp::safememory
{

template<class _Ty>
using node_owning_ptr = owning_ptr<_Ty>;

template<class _Ty>
using node_soft_ptr = soft_ptr<_Ty>;

template<class _Ty,	class... _Types>
_Ty* node_make_owning(_Types&&... _Args)
{
	return new _Ty(::std::forward<_Types>(_Args)...);
}



template<class _Ty>
void node_delete_owning(_Ty ty)
{
	delete ty;
}


template<class T, class T1>
node_soft_ptr<T> node_soft_ptr_static_cast(node_soft_ptr<T1> p) {
	return soft_ptr_static_cast<T, T1>(p);
}

template<class T, class T1>
T node_soft_ptr_static_cast(T1 p) {
	return static_cast<T>(p);
}

} // namespace nodecpp::safememory

#endif // SAFE_PTR_FOR_MAP_H

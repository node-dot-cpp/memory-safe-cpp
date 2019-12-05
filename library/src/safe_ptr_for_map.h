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

#include "safe_ptr_impl.h"

namespace nodecpp::safememory
{


template<class _Ty,
	class... _Types,
	std::enable_if_t<!std::is_array<_Ty>::value, int> = 0>
	NODISCARD _Ty* allocate_and_construct_with_control_block(_Types&&... _Args)
	{
	uint8_t* data = reinterpret_cast<uint8_t*>( zombieAllocate( sizeof(FirstControlBlock) - getPrefixByteCount() + sizeof(_Ty) ) );
	uint8_t* dataForObj = data + sizeof(FirstControlBlock) - getPrefixByteCount();

	FirstControlBlock* cb = getControlBlock_(dataForObj);
	cb->init();
#ifdef NODECPP_SAFEMEMORY_HEAVY_DEBUG
		cb->dbgCheckValidity<void>();
#endif

//	owning_ptr_impl<_Ty> op(make_owning_t(), (_Ty*)(uintptr_t)(dataForObj));
	void* stackTmp = thg_stackPtrForMakeOwningCall;
	thg_stackPtrForMakeOwningCall = dataForObj;
	_Ty* objPtr = new ( dataForObj ) _Ty(::std::forward<_Types>(_Args)...);
	thg_stackPtrForMakeOwningCall = stackTmp;

	return dataForObj;
	}


template<class T>
void updatePtrForListItemsWithInvalidPtr(FirstControlBlock* cb)
{
	for ( size_t i=0; i<FirstControlBlock::maxSlots; ++i )
		if ( cb->slots[i].isUsed() )
			reinterpret_cast<soft_ptr_impl<T>*>(cb->slots[i].getPtr())->invalidatePtr();
	if ( cb->otherAllockedSlots.getPtr() )
		for ( size_t i=0; i<cb->otherAllockedSlots.getPtr()->otherAllockedCnt; ++i )
			if ( cb->otherAllockedSlots.getPtr()->slots[i].isUsed() )
				reinterpret_cast<soft_ptr_impl<T>*>(cb->otherAllockedSlots.getPtr()->slots[i].getPtr())->invalidatePtr();
}

template<class _Ty,
	std::enable_if_t<!std::is_array<_Ty>::value, int> = 0>
	void destruct_and_deallocate_with_control_block(_Ty* t)
	{
		if ( NODECPP_LIKELY(t) )
		{
			uint8_t* alloca = getAllocatedBlock_(t);
			FirstControlBlock* cb = getControlBlock_(t);
			destruct( t );
			updatePtrForListItemsWithInvalidPtr<void>();
			zombieDeallocate( alloca );
			cb->clear();
#ifdef NODECPP_SAFEMEMORY_HEAVY_DEBUG
			cb->dbgCheckValidity<void>();
#endif
		}
	}

template<class _Ty,
	std::enable_if_t<!std::is_array<_Ty>::value, int> = 0>
	void get_soft_ptr_from_control_block(_Ty* t)
	{
		if ( NODECPP_LIKELY(t) )
		{
//			uint8_t* alloca = getAllocatedBlock_(t);
			FirstControlBlock* cb = getControlBlock_(t);
			return soft_ptr_impl<_Ty>(cb, t);
		}
		else
		{
			retur soft_ptr_impl<_Ty>(nullptr);
		}
		
	}

} // namespace nodecpp::safememory

#endif // SAFE_PTR_FOR_MAP_H

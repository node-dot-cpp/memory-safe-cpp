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

#ifndef SAFE_MEMORY_DETAIL_ALLOCATOR_TO_EASTL_H
#define SAFE_MEMORY_DETAIL_ALLOCATOR_TO_EASTL_H

#include <safe_memory/safe_ptr.h>
#include <safe_memory/detail/safe_ptr_with_zero_offset.h>

#ifdef NODECPP_SAFEMEMORY_HEAVY_DEBUG
#include <set>
#endif

namespace safe_memory::detail {

using nodecpp::safememory::memory_safety;
using nodecpp::safememory::FirstControlBlock;
using nodecpp::safememory::module_id;
using nodecpp::safememory::lib_helpers::soft_ptr_with_zero_offset_impl;
using nodecpp::safememory::lib_helpers::soft_ptr_with_zero_offset_no_checks;
using nodecpp::safememory::getPrefixByteCount;
using nodecpp::safememory::zombieAllocate;
using nodecpp::safememory::getControlBlock_;
using nodecpp::safememory::zombieDeallocate;
using nodecpp::safememory::getAllocatedBlock_;
using nodecpp::safememory::allocate;
using nodecpp::safememory::deallocate;
using nodecpp::safememory::soft_ptr_impl;

template<class T>
struct array_of
{
	typedef array_of<T> this_type;

	size_t _capacity = 0;
	alignas(T) char _begin;

	[[noreturn]] static
	void throwPointerOutOfRange(const char* msg) {
		throw std::out_of_range(msg);
	}

public:
	array_of(size_t capacity) :_capacity(capacity) {}

	array_of(const array_of&) = delete;
	array_of(array_of&&) = delete;

	array_of& operator=(const array_of&) = delete;
	array_of& operator=(array_of&&) = delete;

	~array_of() {}

	template<memory_safety Safety>
	T& at(size_t ix) {
		if constexpr ( Safety == memory_safety::safe ) {
			if(ix >= _capacity)
				throwPointerOutOfRange("array_of::at(): ix >= _capacity");
		}

		return *(begin() + ix);
	}

	template<memory_safety Safety>
	const T& at(size_t ix) const {
		if constexpr ( Safety == memory_safety::safe ) {
			if(ix >= _capacity)
				throwPointerOutOfRange("array_of::at(): ix >= _capacity");
		}

		return *(begin() + ix);
	}

	// T& at_unsafe(size_t ix) {
	// 	return at<memory_safety::none>(ix);
	// }
	
	// const T& at_unsafe(size_t ix) const {
	// 	return at<memory_safety::none>(ix);
	// }

	//unsafe function, allow returning a non-derefenceable pointer as end()
	T* get_raw_ptr(size_t ix) {
		NODECPP_ASSERT(module_id, nodecpp::assert::AssertLevel::critical, ix <= _capacity );
		return begin() + ix;
	}

	//unsafe function, ptr should have been validated
	size_t get_index(const T* ptr) const {
		NODECPP_ASSERT(module_id, nodecpp::assert::AssertLevel::critical, static_cast<size_t>(ptr - begin()) <= _capacity );
		return static_cast<size_t>(ptr - begin());
	}

	size_t capacity() const { return _capacity; }
	T* begin() const { return const_cast<T*>(reinterpret_cast<const T*>(&_begin)); }
	T* end() const { return begin() + capacity(); }

	static
	size_t calculateSize(size_t size) {
		// TODO here we should fine tune the sizes of array_of<T> 
		return sizeof(this_type) + (sizeof(T) * size);
	}
};



template<class T>
struct allocator_to_eastl_impl {
	
	typedef soft_ptr_with_zero_offset_impl<array_of<T>> soft_array_type;

	soft_array_type allocate_array(std::size_t count) {

		std::size_t head = sizeof(FirstControlBlock) - getPrefixByteCount();

		// TODO here we should fine tune the sizes of array_of2<T> 
		std::size_t total = head + array_of<T>::calculateSize(count);
		void* data = zombieAllocate(total);

		// non trivial types get zeroed memory, just in case we get to deref
		// a non initialized position
		if constexpr (!std::is_trivial<T>::value)
			std::memset(data, 0, total);

		array_of<T>* dataForObj = reinterpret_cast<array_of<T>*>(reinterpret_cast<uintptr_t>(data) + head);
		
		auto cb = getControlBlock_(dataForObj);
		cb->init();

		::new ( dataForObj ) array_of<T>(count);

		return soft_array_type(make_zero_offset_t(), dataForObj);
	}

	void deallocate_array(soft_array_type p) {

		if (p) {
			array_of<T>* dataForObj = p.ptr;
			dataForObj->~array_of<T>();
			auto cb = getControlBlock_(dataForObj);
			cb->template updatePtrForListItemsWithInvalidPtr<T>();
			zombieDeallocate( getAllocatedBlock_(dataForObj) );
			cb->clear();
		}
	}
};

template<class T>
struct allocator_to_eastl_no_checks {

	typedef soft_ptr_with_zero_offset_no_checks<array_of<T>> soft_array_type;

	soft_array_type allocate_array(std::size_t count) {
		// TODO here we should fine tune the sizes of array_of2<T> 
		std::size_t sz = array_of<T>::calculateSize(count);
		array_of<T>* dataForObj = reinterpret_cast<array_of<T>*>(allocate(sz));
		
		::new ( dataForObj ) array_of<T>(count);
		return soft_array_type(make_zero_offset_t(), dataForObj);
	}

	void deallocate_array(soft_array_type p) {

		if (p) {
			array_of<T>* dataForObj = p.ptr;
			dataForObj->~array_of<T>();
			deallocate(dataForObj);
		}
	}

};


template<class T, memory_safety Safety>
using allocator_to_eastl = std::conditional_t<Safety == memory_safety::none,
			allocator_to_eastl_no_checks<T>, allocator_to_eastl_impl<T>>;


} // namespace safe_memory::detail

namespace nodecpp::safememory::lib_helpers {


using safe_memory::detail::array_of;

template<class T>
class soft_ptr_with_zero_offset_impl<array_of<T>>
{
	friend class owning_ptr_impl<array_of<T>>;
	friend struct safe_memory::detail::allocator_to_eastl_impl<T>;

	friend struct ::nodecpp::safememory::FirstControlBlock;

	template<class TT>
	friend class soft_ptr_with_zero_offset_base_no_checks;
	template<class TT>
	friend class soft_ptr_with_zero_offset_no_checks;

	array_of<T>* ptr= nullptr;

	FirstControlBlock* getControlBlock() const { return getControlBlock_( ptr ); }
	static FirstControlBlock* getControlBlock(void* t) { return getControlBlock_(t); }

public:

	static constexpr memory_safety is_safe = memory_safety::safe;
 
	soft_ptr_with_zero_offset_impl() {}

	soft_ptr_with_zero_offset_impl( make_zero_offset_t, array_of<T>* raw )
	{
		ptr = raw;
	}

	soft_ptr_with_zero_offset_impl( const soft_ptr_impl<array_of<T>>& other )
	{
		ptr = other.getDereferencablePtr();
	}
	soft_ptr_with_zero_offset_impl& operator = ( const soft_ptr_impl<array_of<T>>& other )
	{
		ptr = other.getDereferencablePtr();
		return *this;
	}

	soft_ptr_with_zero_offset_impl( const soft_ptr_with_zero_offset_impl<array_of<T>>& other )
	{
		ptr = other.ptr;
	}
	soft_ptr_with_zero_offset_impl& operator = ( const soft_ptr_with_zero_offset_impl<array_of<T>>& other )
	{
		ptr = other.ptr;
		return *this;
	}
	soft_ptr_with_zero_offset_impl( soft_ptr_with_zero_offset_impl<array_of<T>>&& other )
	{
		// Note: we do not null the 'other': behaves as an ordinary (raw) pointer
		if ( this == &other ) return;
		ptr = other.ptr;
	}

	soft_ptr_with_zero_offset_impl& operator = ( soft_ptr_with_zero_offset_impl<array_of<T>>&& other )
	{
		// Note: we do not null the 'other': behaves as an ordinary (raw) pointer
		if ( this == &other ) return *this;
		ptr = other.ptr;
		return *this;
	}

	soft_ptr_with_zero_offset_impl( std::nullptr_t nulp ) {}
	soft_ptr_with_zero_offset_impl& operator = ( std::nullptr_t nulp )
	{
		reset();
		return *this;
	}

	void swap( soft_ptr_with_zero_offset_impl<array_of<T>>& other )
	{
		T* tmp = ptr;
		ptr = other.ptr;
		other.ptr = tmp;
	}

	soft_ptr_impl<array_of<T>> get() const
	{
		if(NODECPP_LIKELY(ptr != nullptr)) {
			FirstControlBlock* cb = getControlBlock_( ptr );
			return soft_ptr_impl<array_of<T>>( cb, ptr );
		}
		else
			return soft_ptr_impl<array_of<T>>();
	}

	explicit operator bool() const noexcept
	{
		return ptr != nullptr;
	}

	void reset() { ptr = nullptr; }

	// bool operator == (const owning_ptr_impl<T>& other ) const { return ptr == other.t.getTypedPtr(); }
	// template<class T1> 
	// bool operator == (const owning_ptr_impl<T1>& other ) const { return ptr == other.t.getTypedPtr(); }

	bool operator == (const soft_ptr_impl<array_of<T>>& other ) const { return ptr == other.getDereferencablePtr(); }
	template<class T1> 
	bool operator == (const soft_ptr_impl<array_of<T1>>& other ) const { return ptr == other.getDereferencablePtr(); }

	bool operator == (const soft_ptr_with_zero_offset_impl<array_of<T>>& other ) const { return ptr == other.ptr; }
	template<class T1>
	bool operator == (const soft_ptr_with_zero_offset_impl<array_of<T1>>& other ) const { return ptr == other.ptr; }

	// bool operator != (const owning_ptr_impl<T>& other ) const { return ptr != other.t.getTypedPtr(); }
	// template<class T1> 
	// bool operator != (const owning_ptr_impl<T1>& other ) const { return ptr != other.t.getTypedPtr(); }

	bool operator != (const soft_ptr_impl<array_of<T>>& other ) const { return ptr != other.getDereferencablePtr(); }
	template<class T1> 
	bool operator != (const soft_ptr_impl<array_of<T1>>& other ) const { return ptr != other.getDereferencablePtr(); }

	bool operator != (const soft_ptr_with_zero_offset_impl<array_of<T>>& other ) const { return ptr != other.ptr; }
	template<class T1>
	bool operator != (const soft_ptr_with_zero_offset_impl<array_of<T1>>& other ) const { return ptr != other.ptr; }

	bool operator == (std::nullptr_t nullp ) const { NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::pedantic, nullp == nullptr); return ptr == nullptr; }
	bool operator != (std::nullptr_t nullp ) const { NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::pedantic, nullp == nullptr); return ptr != nullptr; }

	// bool operator == (const T* other ) const { return get_raw_ptr() == other; }
	// bool operator != (const T* other ) const { return get_raw_ptr() != other; }
	// bool operator <  (const T* other ) const { return get_raw_ptr() <  other; }
	// bool operator <= (const T* other ) const { return get_raw_ptr() <= other; }
	// bool operator >  (const T* other ) const { return get_raw_ptr() >  other; }
	// bool operator >= (const T* other ) const { return get_raw_ptr() >= other; }

	array_of<T>& operator * () const
	{
		return *ptr;
	}

	array_of<T>* operator -> () const 
	{
		return ptr;
	}

	T* get_raw_ptr() const { return ptr ? ptr->begin() : nullptr; }

	operator T*() const { return get_raw_ptr(); }

	T* operator+(std::ptrdiff_t n) const { return get_raw_ptr() + n; }
	// std::ptrdiff_t operator-(const T* other) const { return get_raw_ptr() - other; }

	~soft_ptr_with_zero_offset_impl()
	{
		NODECPP_DEBUG_COUNT_SOFT_PTR_ZERO_OFFSET_DTOR();
//		ptr = nullptr;
	}
};

// template<class T> bool operator != (const owning_ptr_impl<T>& p1, const soft_ptr_with_zero_offset_impl<T>& p2 ) { return p2 != p1; }
// template<class T> bool operator == (const owning_ptr_impl<T>& p1, const soft_ptr_with_zero_offset_impl<T>& p2 ) { return p2 == p1; }

// template<class T> bool operator != (const soft_ptr_impl<T>& p1, const soft_ptr_with_zero_offset_impl<T>& p2 ) { return p2 != p1; }
// template<class T> bool operator == (const soft_ptr_impl<T>& p1, const soft_ptr_with_zero_offset_impl<T>& p2 ) { return p2 == p1; }

// template<class T>
// std::ptrdiff_t operator-(const T* p1, const soft_ptr_with_zero_offset_impl<array_of<T>>& p2 ) { return p1 - p2.get_raw_ptr(); }

// template<class T>
// bool operator==(const T* p1, const soft_ptr_with_zero_offset_impl<array_of<T>>& p2 ) { return p1 == p2.get_raw_ptr(); }

// template<class T>
// bool operator!=(const T* p1, const soft_ptr_with_zero_offset_impl<array_of<T>>& p2 ) { return p1 != p2.get_raw_ptr(); }

// template<class T>
// bool operator<(const T* p1, const soft_ptr_with_zero_offset_impl<array_of<T>>& p2 ) { return p1 < p2.get_raw_ptr(); }

// template<class T>
// bool operator<=(const T* p1, const soft_ptr_with_zero_offset_impl<array_of<T>>& p2 ) { return p1 <= p2.get_raw_ptr(); }

// template<class T>
// bool operator>(const T* p1, const soft_ptr_with_zero_offset_impl<array_of<T>>& p2 ) { return p1 > p2.get_raw_ptr(); }

// template<class T>
// bool operator>=(const T* p1, const soft_ptr_with_zero_offset_impl<array_of<T>>& p2 ) { return p1 >= p2.get_raw_ptr(); }

template<class T>
class soft_ptr_with_zero_offset_no_checks<array_of<T>>
{
	friend struct safe_memory::detail::allocator_to_eastl_no_checks<T>;


	template<class TT>
	friend class soft_ptr_with_zero_offset_base_no_checks;
	template<class TT>
	friend class soft_ptr_with_zero_offset_no_checks;

	array_of<T>* ptr = nullptr;

public:

	static constexpr memory_safety is_safe = memory_safety::none;
 
	soft_ptr_with_zero_offset_no_checks() { }

	soft_ptr_with_zero_offset_no_checks( make_zero_offset_t, array_of<T>* raw )
	{
		ptr = raw;
	}

	// soft_ptr_with_zero_offset_no_checks( const owning_ptr_no_checks<T>& owner ) :ptr(owner.t) {	}
	// soft_ptr_with_zero_offset_no_checks<T>& operator = ( const owning_ptr_no_checks<T>& owner )
	// {
	// 	ptr = owner.t;
	// 	return *this;
	// }


	// soft_ptr_with_zero_offset_no_checks( const owning_ptr_base_no_checks<T>& owner ) :ptr(owner.t) { }
	// soft_ptr_with_zero_offset_no_checks<T>& operator = ( const owning_ptr_base_no_checks<T>& owner )
	// {
	// 	ptr = owner.t;
	// 	return *this;
	// }

	soft_ptr_with_zero_offset_no_checks( const soft_ptr_no_checks<array_of<T>>& other ) :ptr(other.t) { }
	soft_ptr_with_zero_offset_no_checks& operator = ( const soft_ptr_no_checks<array_of<T>>& other )
	{
		ptr = other.t;
		return *this;
	}

	soft_ptr_with_zero_offset_no_checks( const soft_ptr_with_zero_offset_no_checks<array_of<T>>& other ) :ptr(other.ptr) { }
	soft_ptr_with_zero_offset_no_checks& operator = ( const soft_ptr_with_zero_offset_no_checks<array_of<T>>& other )
	{
		ptr = other.ptr;
		return *this;
	}


	soft_ptr_with_zero_offset_no_checks( soft_ptr_with_zero_offset_no_checks<array_of<T>>&& other ) :ptr(other.ptr) { }
	soft_ptr_with_zero_offset_no_checks& operator = ( soft_ptr_with_zero_offset_no_checks<array_of<T>>&& other )
	{
		ptr = other.ptr;
		return *this;
	}

	soft_ptr_with_zero_offset_no_checks( std::nullptr_t nulp ) {}
	soft_ptr_with_zero_offset_no_checks& operator = ( std::nullptr_t nulp )
	{
		reset();
		return *this;
	}

	void swap( soft_ptr_with_zero_offset_no_checks<array_of<T>>& other )
	{
		T* tmp = ptr;
		ptr = other.ptr;
		other.ptr = tmp;
	}

	soft_ptr_no_checks<array_of<T>> get() const
	{
//		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, ptr != nullptr );
//		FirstControlBlock* cb = getControlBlock_( ptr );
		return soft_ptr_no_checks<array_of<T>>( fbc_ptr_t(), ptr );
	}

	explicit operator bool() const noexcept
	{
		return ptr != nullptr;
	}

	void reset() { ptr = nullptr; }

	// bool operator == (const owning_ptr_no_checks<T>& other ) const { return ptr == other.t; }
	// template<class T1> 
	// bool operator == (const owning_ptr_no_checks<T1>& other ) const { return ptr == other.t; }

	bool operator == (const soft_ptr_no_checks<array_of<T>>& other ) const { return ptr == other.getDereferencablePtr(); }
	template<class T1> 
	bool operator == (const soft_ptr_no_checks<array_of<T1>>& other ) const { return ptr == other.getDereferencablePtr(); }

	bool operator == (const soft_ptr_with_zero_offset_no_checks<array_of<T>>& other ) const { return ptr == other.ptr; }
	template<class T1>
	bool operator == (const soft_ptr_with_zero_offset_no_checks<array_of<T1>>& other ) const { return ptr == other.ptr; }

	// bool operator != (const owning_ptr_no_checks<T>& other ) const { return ptr != other.t; }
	// template<class T1> 
	// bool operator != (const owning_ptr_no_checks<T1>& other ) const { return ptr != other.t; }

	bool operator != (const soft_ptr_no_checks<array_of<T>>& other ) const { return ptr != other.getDereferencablePtr(); }
	template<class T1> 
	bool operator != (const soft_ptr_no_checks<array_of<T1>>& other ) const { return ptr != other.getDereferencablePtr(); }

	bool operator != (const soft_ptr_with_zero_offset_no_checks<array_of<T>>& other ) const { return ptr != other.ptr; }
	template<class T1>
	bool operator != (const soft_ptr_with_zero_offset_no_checks<array_of<T1>>& other ) const { return ptr != other.ptr; }

	bool operator == (std::nullptr_t nullp ) const { NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::pedantic, nullp == nullptr); return ptr == nullptr; }
	bool operator != (std::nullptr_t nullp ) const { NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::pedantic, nullp == nullptr); return ptr != nullptr; }

	// bool operator == (const T* other ) const { return get_raw_ptr() == other; }
	// bool operator != (const T* other ) const { return get_raw_ptr() != other; }
	// bool operator <  (const T* other ) const { return get_raw_ptr() <  other; }
	// bool operator <= (const T* other ) const { return get_raw_ptr() <= other; }
	// bool operator >  (const T* other ) const { return get_raw_ptr() >  other; }
	// bool operator >= (const T* other ) const { return get_raw_ptr() >= other; }

	array_of<T>& operator * () const
	{
		return *ptr;
	}

	array_of<T>* operator -> () const 
	{
		return ptr;
	}

	T* get_raw_ptr() const { return ptr ? ptr->begin() : nullptr; }

	operator T*() const { return get_raw_ptr(); }

	T* operator+(std::ptrdiff_t n) const { return get_raw_ptr() + n; }
	// std::ptrdiff_t operator-(const T* other) const { return get_raw_ptr() - other; }

	~soft_ptr_with_zero_offset_no_checks()
	{
		NODECPP_DEBUG_COUNT_SOFT_PTR_ZERO_OFFSET_DTOR();
	}
};

// template<class T>
// std::ptrdiff_t operator-(const T* p1, const soft_ptr_with_zero_offset_no_checks<array_of<T>>& p2 ) { return p1 - p2.get_raw_ptr(); }

// template<class T>
// bool operator==(const T* p1, const soft_ptr_with_zero_offset_no_checks<array_of<T>>& p2 ) { return p1 == p2.get_raw_ptr(); }

// template<class T>
// bool operator!=(const T* p1, const soft_ptr_with_zero_offset_no_checks<array_of<T>>& p2 ) { return p1 != p2.get_raw_ptr(); }

// template<class T>
// bool operator<(const T* p1, const soft_ptr_with_zero_offset_no_checks<array_of<T>>& p2 ) { return p1 < p2.get_raw_ptr(); }

// template<class T>
// bool operator<=(const T* p1, const soft_ptr_with_zero_offset_no_checks<array_of<T>>& p2 ) { return p1 <= p2.get_raw_ptr(); }

// template<class T>
// bool operator>(const T* p1, const soft_ptr_with_zero_offset_no_checks<array_of<T>>& p2 ) { return p1 > p2.get_raw_ptr(); }

// template<class T>
// bool operator>=(const T* p1, const soft_ptr_with_zero_offset_no_checks<array_of<T>>& p2 ) { return p1 >= p2.get_raw_ptr(); }


} // namespace nodecpp::safememory::lib_helpers



#endif // SAFE_MEMORY_DETAIL_ALLOCATOR_TO_EASTL_H

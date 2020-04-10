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

#ifndef SAFEMEMORY_DETAIL_STRING_DETAIL_H
#define SAFEMEMORY_DETAIL_STRING_DETAIL_H

#include <safe_ptr.h>
// #include "safe_ptr_with_zero_offset.h"
#include <iterator>

namespace safememory
{

namespace detail {

struct StackPtrForMakeOwningCallRiia {
	void* stackTmp = nodecpp::safememory::thg_stackPtrForMakeOwningCall;
	StackPtrForMakeOwningCallRiia(void* dataForObj) {
		nodecpp::safememory::thg_stackPtrForMakeOwningCall = dataForObj;
	}
	~StackPtrForMakeOwningCallRiia() {
		nodecpp::safememory::thg_stackPtrForMakeOwningCall = stackTmp;
	}
};

template<class T>
class alignas(T) alignas(T*) array_of
{
	typedef array_of<T> this_type;

	T* _begin;
	T* _end;
	T* _capacity_end;

	void throwPointerOutOfRange() const {
		//TODO
		throw 0;
	}

	T* checkPtrRange(T* ptr) const {

		if(ptr < _begin)
			throwPointerOutOfRange();
		if(ptr >= _end)
			throwPointerOutOfRange();
		return ptr;
	}

	T* checkCapacityRange(T* ptr) const {

		if(ptr < _begin)
			throwPointerOutOfRange();
		if(ptr > _capacity_end)
			throwPointerOutOfRange();
		return ptr;
	}

public:
	array_of(size_t capacity)
		{
			auto b = reinterpret_cast<uintptr_t>(this) + sizeof(array_of<T>);
			_begin = reinterpret_cast<T*>(b);
			_end = _begin;
			_capacity_end = _begin + capacity;
		}

	// array_of( T* ptr, size_t size ): 
	// 	_begin(ptr), _end(ptr), _capacity_end(ptr + size)
	// 	{}

	array_of(const array_of&) = delete;
	array_of(array_of&&) = default;

	array_of& operator=(const array_of&) = delete;
	array_of& operator=(array_of&&) = default;

	~array_of() {}

	void swap( array_of<T>& other )
	{
		swap(_begin, other._begin);
		swap(_end, other._end);
		swap(_capacity_end, other._capacity_end);
	}

	T& at(size_t ix) { return *checkPtrRange(_begin + ix); }
	const T& const_at(size_t ix) const { return *checkPtrRange(_begin + ix); }

	template<class... ARGS>
	void emplace_back(ARGS&&... args)
	{
		if(_end == _capacity_end)
			throwPointerOutOfRange();

		StackPtrForMakeOwningCallRiia Riia(_end);
		::new (static_cast<void*>(_end)) T(std::forward<ARGS>(args)...);
		++_end;
	}

	void pop_back()
	{
		if(_end == _begin)
			throwPointerOutOfRange();

		--_end; //decrement even if ~T() throws
		_end->~T();
	}

	void uninitialized_fill_n(size_t count, const T& value)
	{
		//TODO simplify for trivial types (like char)

		T* last = checkCapacityRange(_end + count);
		T* prev = _end;// keep a copy in case of exception
		try {
			for(;_end != last; ++_end) {
				StackPtrForMakeOwningCallRiia Riia(_end);
				::new (static_cast<void*>(_end)) T(value);
			}
		}
		catch(...) {
			while(_end != prev)
				pop_back();

			throw;
		}
	}

	void destroy_n(size_t count) {
		
		T* last = checkPtrRange(_end - count);

		while(_end != last) {
			--_end;
			_end->~T();
		}
	}

	size_t size() const {
		return static_cast<size_t>((_end - _begin) / sizeof(T));
	}
	void set_size_unsafe(size_t sz) {
		_end = _begin + sz;
	}

	T* get_raw_ptr(size_t sz) const {
		return checkCapacityRange(_begin + sz);
	}

	size_t capacity() const {
		return static_cast<size_t>((_capacity_end - _begin) / sizeof(T));
	}

	size_t remaining_capacity() const {
		return static_cast<size_t>((_capacity_end - _end) / sizeof(T));
	}

	void clear() {
		destroy_n(size());
	}

	void uninitialized_copy(const array_of& from) {
		
		uninitialized_copy(from, 0, from.size());
	}

	void uninitialized_copy_n(const array_of& from, size_t first, size_t count) {

		T* from_begin = from.checkPtrRange(from._begin + first);
		T* from_end = from.checkPtrRange(from_begin + count);

		checkCapacityRange(_end + count);// check we have room

		T* prev = _end;// keep a copy in case of exception
		try {
			for(;from_begin != from_end; ++from_begin, ++_end) {
				StackPtrForMakeOwningCallRiia Riia(_end);
				::new (static_cast<void*>(_end)) T(*from_begin);
			}
		}
		catch(...) {
			while(_end != prev)
				pop_back();

			throw;
		}
	}

	explicit operator bool() const noexcept
	{
		return _begin != nullptr;
	}

	T* begin() { return _begin; }
	const T* const_begin() const { return _begin; }
	T* end() { return _end; }
	const T* const_end() const { return _end; }
};

template<class _Ty>
	NODISCARD nodecpp::safememory::owning_ptr_impl<array_of<_Ty>> make_owning_array_impl(size_t size)
	{
		using namespace nodecpp::safememory;
		size_t head = sizeof(FirstControlBlock) - getPrefixByteCount();
		void* data = zombieAllocate( head + sizeof(array_of<_Ty>) + (sizeof(_Ty) * size));
		array_of<_Ty>* dataForObj = reinterpret_cast<array_of<_Ty>*>(reinterpret_cast<uintptr_t>(data) + head);
		owning_ptr_impl<array_of<_Ty>> op(make_owning_t(), dataForObj);
		// void* stackTmp = thg_stackPtrForMakeOwningCall;
		// thg_stackPtrForMakeOwningCall = dataForObj;
		array_of<_Ty>* objPtr = new ( dataForObj ) array_of<_Ty>(size);
		// thg_stackPtrForMakeOwningCall = stackTmp;
		//return owning_ptr_impl<_Ty>(objPtr);
		return op;
	}

template<class _Ty>
	NODISCARD 
	nodecpp::safememory::owning_ptr<array_of<_Ty>> make_owning_array(size_t size) {
		return make_owning_array_impl<_Ty>(size);
	}

template<class T>
class alignas(T) alignas(T*) array_of2
{
	typedef array_of2<T> this_type;

	size_t _capacity = 0;

	//TODO fix sizeof(this) used for allocation
	T _begin[1];

	void throwPointerOutOfRange() const {
		//TODO
		throw 0;
	}

public:
	array_of2(size_t capacity) :_capacity(capacity) {}

	array_of2(const array_of2&) = delete;
	array_of2(array_of2&&) = delete;

	array_of2& operator=(const array_of2&) = delete;
	array_of2& operator=(array_of2&&) = delete;

	~array_of2() {}

	T& at(size_t ix) {
		if(ix >= _capacity)
			throwPointerOutOfRange();

		return _begin[ix];
	}
	
	const T& at(size_t ix) const {
		if(ix >= _capacity)
			throwPointerOutOfRange();

		return _begin[ix];
	}

	T* get_raw_ptr(size_t ix) {
		// allow returning 'end()' pointer
		// caller is responsible of not dereferencing it
		if(ix > _capacity)
			throwPointerOutOfRange();
		
		return _begin + ix;
	}

	size_t capacity() const { return _capacity; }

	T* begin() { return _begin; }
	const T* begin() const { return _begin; }
};


template<class _Ty>
	NODISCARD nodecpp::safememory::owning_ptr_impl<array_of2<_Ty>> make_owning_array_of_impl(size_t size)
	{
		using namespace nodecpp::safememory;
		size_t head = sizeof(FirstControlBlock) - getPrefixByteCount();
		void* data = zombieAllocate( head + sizeof(array_of2<_Ty>) + (sizeof(_Ty) * size));
		array_of2<_Ty>* dataForObj = reinterpret_cast<array_of2<_Ty>*>(reinterpret_cast<uintptr_t>(data) + head);
		owning_ptr_impl<array_of2<_Ty>> op(make_owning_t(), dataForObj);
		// void* stackTmp = thg_stackPtrForMakeOwningCall;
		// thg_stackPtrForMakeOwningCall = dataForObj;
		array_of2<_Ty>* objPtr = new ( dataForObj ) array_of2<_Ty>(size);
		// thg_stackPtrForMakeOwningCall = stackTmp;
		//return owning_ptr_impl<_Ty>(objPtr);
		return op;
	}

template<class _Ty>
	NODISCARD 
	nodecpp::safememory::owning_ptr<array_of2<_Ty>> make_owning_array_of(size_t size) {
		return make_owning_array_of_impl<_Ty>(size);
	}

} //namespace detail

template <typename T>
class unsafe_iterator
{
public:
	typedef std::random_access_iterator_tag  	iterator_category;
	// typedef typename std::remove_const<T>::type value_type;
	typedef T									value_type;
	typedef std::ptrdiff_t                      difference_type;
	typedef T*   								pointer;
	typedef T&	 								reference;

//private:
	pointer mIterator;

public:
	constexpr unsafe_iterator()
		: mIterator(nullptr) { }

	constexpr explicit unsafe_iterator(pointer i)
		: mIterator(i) { }

	constexpr unsafe_iterator(const unsafe_iterator& ri)
		: mIterator(ri.mIterator) { }

	constexpr reference operator*() const
	{
		return *mIterator;
	}

	constexpr pointer operator->() const
		{ return &(operator*()); }

	constexpr unsafe_iterator& operator++()
		{ ++mIterator; return *this; }

	constexpr unsafe_iterator operator++(int)
	{
		unsafe_iterator ri(*this);
		++mIterator;
		return ri;
	}

	constexpr unsafe_iterator& operator--()
		{ --mIterator; return *this; }

	constexpr unsafe_iterator operator--(int)
	{
		unsafe_iterator ri(*this);
		--mIterator;
		return ri;
	}

	constexpr unsafe_iterator operator+(difference_type n) const
		{ return unsafe_iterator(mIterator + n); }

	constexpr unsafe_iterator& operator+=(difference_type n)
		{ mIterator += n; return *this; }

	constexpr unsafe_iterator operator-(difference_type n) const
		{ return unsafe_iterator(mIterator - n); }

	constexpr unsafe_iterator& operator-=(difference_type n)
		{ mIterator -= n; return *this; }

	constexpr reference operator[](difference_type n) const
		{ return mIterator[n]; }

	constexpr bool operator==(const unsafe_iterator& ri) const
		{ return mIterator == ri.mIterator; }

	constexpr bool operator!=(const unsafe_iterator& ri) const
		{ return !operator==(ri); }
};

template <typename T>
typename unsafe_iterator<T>::difference_type distance(const unsafe_iterator<T>& l, const unsafe_iterator<T>& r) {

		return l.mIterator - r.mIterator;
}


template <typename T, typename SoftArrayOfPtr>
class safe_iterator_impl
{
public:
	typedef std::random_access_iterator_tag  	iterator_category;
	// typedef typename std::remove_const<T>::type value_type;
	typedef T									value_type;
	typedef std::ptrdiff_t                      difference_type;
	typedef T*   								pointer;
	typedef T&									reference;

	typedef SoftArrayOfPtr 						soft_array_of_ptr_type;
//private:
	soft_array_of_ptr_type ptr;
	size_t ix = 0;


public:
	constexpr safe_iterator_impl() {}

	constexpr explicit safe_iterator_impl(soft_array_of_ptr_type ptr, size_t ix) : ptr(ptr), ix(ix) {}

	constexpr safe_iterator_impl(const safe_iterator_impl& ri) = default;

	constexpr pointer get_raw_ptr() const
	{
		return ptr->get_raw_ptr(ix);
	}

	constexpr reference operator*() const
	{
		return ptr->at(ix);
	}

	constexpr pointer operator->() const
		{ return &(operator*()); }

	constexpr safe_iterator_impl& operator++()
		{ ++ix; return *this; }

	constexpr safe_iterator_impl operator++(int)
	{
		safe_iterator_impl ri(*this);
		++ix;
		return ri;
	}

	constexpr safe_iterator_impl& operator--()
		{ --ix; return *this; }

	constexpr safe_iterator_impl operator--(int)
	{
		safe_iterator_impl ri(*this);
		--ix;
		return ri;
	}

	constexpr safe_iterator_impl operator+(difference_type n) const
		{ return safe_iterator_impl(ptr, ix + n); }

	constexpr safe_iterator_impl& operator+=(difference_type n)
		{ ix += n; return *this; }

	constexpr safe_iterator_impl operator-(difference_type n) const
		{ return safe_iterator_impl(ptr, ix - n); }

	constexpr safe_iterator_impl& operator-=(difference_type n)
		{ ix -= n; return *this; }

	constexpr reference operator[](difference_type n) const
		{ return ptr->at(ix + n); }

	constexpr bool operator==(const safe_iterator_impl& ri) const
		{ return ptr == ri.ptr && ix == ri.ix; }

	constexpr bool operator!=(const safe_iterator_impl& ri) const
		{ return !operator==(ri); }
};

template <typename T, typename T2>
typename safe_iterator_impl<T, T2>::difference_type distance(const safe_iterator_impl<T, T2>& l, const safe_iterator_impl<T, T2>& r) {

		return (l.ptr == r.ptr)? l.ix - r.ix : 0;
}

} // namespace safememory

namespace nodecpp::safememory {
	using ::safememory::unsafe_iterator;
	using ::safememory::safe_iterator_impl;
	using ::safememory::distance;

	namespace detail {
		using ::safememory::detail::array_of;
		using ::safememory::detail::make_owning_array;
	}
}

#endif // SAFEMEMORY_DETAIL_STRING_DETAIL_H

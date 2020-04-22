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

#ifndef SAFEMEMORY_DETAIL_SAFE_ALLOC_H
#define SAFEMEMORY_DETAIL_SAFE_ALLOC_H

#include <safe_ptr.h>
// #include "safe_ptr_with_zero_offset.h"
#include <iterator>

namespace safememory {

using ::nodecpp::safememory::owning_ptr;
using ::nodecpp::safememory::soft_ptr;

namespace detail {


template< class InputIt, class ForwardIt >
ForwardIt uninitialized_move_or_copy( InputIt first, InputIt last, ForwardIt d_first ) {
	if constexpr (std::is_nothrow_move_constructible<typename std::iterator_traits<InputIt>::value_type>::value) {
		return std::uninitialized_move(first, last, d_first);
	}
	else {
		return std::uninitialized_copy(first, last, d_first);
	}
}




template<class T>
struct array_of2
{
	typedef array_of2<T> this_type;

	size_t _capacity = 0;

	//TODO fix sizeof(this) used for allocation
	union {
		T _begin[1];
		size_t dummy;
	};


	struct StackPtrForMakeOwningCallRiia {
		void* stackTmp = nodecpp::safememory::thg_stackPtrForMakeOwningCall;
		StackPtrForMakeOwningCallRiia(void* dataForObj) {
			nodecpp::safememory::thg_stackPtrForMakeOwningCall = dataForObj;
		}
		~StackPtrForMakeOwningCallRiia() {
			nodecpp::safememory::thg_stackPtrForMakeOwningCall = stackTmp;
		}
	};


	[[noreturn]]
	void throwPointerOutOfRange() const {
		throw std::out_of_range("array_of2 -- out of range");
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

	StackPtrForMakeOwningCallRiia makeRiia() const {
		return StackPtrForMakeOwningCallRiia(const_cast<void*>(this));
	}

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
		/*array_of2<_Ty>* objPtr = */new ( dataForObj ) array_of2<_Ty>(size);
		// thg_stackPtrForMakeOwningCall = stackTmp;
		//return owning_ptr_impl<_Ty>(objPtr);
		return op;
	}

template<class _Ty>
	NODISCARD 
	nodecpp::safememory::owning_ptr<array_of2<_Ty>> make_owning_array_of(size_t size) {
		return make_owning_array_of_impl<_Ty>(size);
	}


template <typename T, typename SoftArrayOfPtr = safememory::soft_ptr<array_of2<typename std::remove_const<T>::type>>>
class unsafe_iterator
{
public:
	typedef std::random_access_iterator_tag  	iterator_category;
	// typedef typename std::remove_const<T>::type value_type;
	typedef T									value_type;
	typedef std::ptrdiff_t                      difference_type;
	typedef T*   								pointer;
	typedef T&	 								reference;
	typedef SoftArrayOfPtr  					soft_array_of_prt;


// private:
	pointer mIterator = nullptr;

public:
	// constexpr unsafe_iterator()
	// 	: mIterator(nullptr) { }
	explicit unsafe_iterator(soft_array_of_prt ptr)
		: mIterator(ptr->_begin) {}

	unsafe_iterator(soft_array_of_prt ptr, size_t ix)
		: mIterator(ptr->_begin + ix) {}

	unsafe_iterator(soft_array_of_prt, pointer ptr)
		: mIterator(ptr) {}

	unsafe_iterator(pointer ptr)
		: mIterator(ptr) {}

	unsafe_iterator(const unsafe_iterator& ri) = default;
	unsafe_iterator& operator=(const unsafe_iterator& ri) = default;

	template<class NonConstT, std::enable_if_t<std::is_same<T, const NonConstT>::value, int> = 0>
	unsafe_iterator(const unsafe_iterator<NonConstT, soft_array_of_prt>& ri)
		: mIterator(ri.mIterator) {}

	pointer get_raw_ptr() const {
		return mIterator;
	}

	reference operator*() const
	{
		return *mIterator;
	}

	pointer operator->() const
		{ return mIterator; }

	unsafe_iterator& operator++()
		{ ++mIterator; return *this; }

	unsafe_iterator operator++(int)
	{
		unsafe_iterator ri(*this);
		++mIterator;
		return ri;
	}

	unsafe_iterator& operator--()
		{ --mIterator; return *this; }

	unsafe_iterator operator--(int)
	{
		unsafe_iterator ri(*this);
		--mIterator;
		return ri;
	}

	unsafe_iterator operator+(difference_type n) const
		{ return unsafe_iterator(mIterator + n); }

	unsafe_iterator& operator+=(difference_type n)
		{ mIterator += n; return *this; }

	unsafe_iterator operator-(difference_type n) const
		{ return unsafe_iterator(mIterator - n); }

	difference_type operator-(const unsafe_iterator& other) const
		{ return distance(*this, other); }

	unsafe_iterator& operator-=(difference_type n)
		{ mIterator -= n; return *this; }

	reference operator[](difference_type n) const
		{ return mIterator[n]; }

	bool operator==(const unsafe_iterator& ri) const
		{ return mIterator == ri.mIterator; }

	bool operator!=(const unsafe_iterator& ri) const
		{ return !operator==(ri); }


	pointer _Unwrapped() const {
		return mIterator;
	}

	void _Seek_to(pointer ptr) {
		mIterator = ptr;
	}

	bool operator<(const unsafe_iterator& ri) const {
		return mIterator < ri.mIterator;
	}

	bool operator<=(const unsafe_iterator& ri) const {
		return mIterator <= ri.mIterator;
	}

	// constexpr bool operator>=(const unsafe_iterator& ri) const {
	// 	return !operator<(ri);
	// }


};

template <typename T, typename Arr>
typename unsafe_iterator<T, Arr>::difference_type distance(const unsafe_iterator<T, Arr>& l, const unsafe_iterator<T, Arr>& r) {

		return l.mIterator - r.mIterator;
}


template <typename T, typename SoftArrayOfPtr = safememory::soft_ptr<array_of2<typename std::remove_const<T>::type>>>
class safe_iterator
{
public:
	typedef std::random_access_iterator_tag  	iterator_category;
	// typedef typename std::remove_const<T>::type value_type;
	typedef T									value_type;
	typedef std::ptrdiff_t                      difference_type;
	typedef T*   								pointer;
	typedef T&									reference;
	typedef SoftArrayOfPtr 						soft_array_of_prt;

// private:
	soft_array_of_prt ptr;
	size_t ix = 0;


public:
	// constexpr safe_iterator() {}
	explicit safe_iterator(soft_array_of_prt ptr)
		: ptr(ptr), ix(0) {}

	safe_iterator(soft_array_of_prt ptr, size_t ix)
		: ptr(ptr), ix(ix) {}

	safe_iterator(soft_array_of_prt ptr, pointer to)
		: ptr(ptr) {
			ix = std::distance(ptr->begin(), to);
		}

	// GCC and clang fail to generate defaultd copy ctor/assign
	safe_iterator(const safe_iterator& ri) 
		:ptr(ri.ptr), ix(ri.ix) {}
		
	safe_iterator& operator=(const safe_iterator& ri) {
		if(this != &ri) {
			this->ptr = ri.ptr;
			this->ix = ri.ix;
		}
		return *this;
	}

	// allow non-const to const convertion
	template<class NonConstT, std::enable_if_t<std::is_same<T, const NonConstT>::value, int> = 0>
	safe_iterator(const safe_iterator<NonConstT, soft_array_of_prt>& ri)
		: ptr(ri.ptr), ix(ri.ix) {}

	pointer get_raw_ptr() const {
		return ptr->get_raw_ptr(ix);
	}

	reference operator*() const
	{
		return ptr->at(ix);
	}

	pointer operator->() const
		{ return ptr->get_raw_ptr(ix); }

	safe_iterator& operator++()
		{ ++ix; return *this; }

	safe_iterator operator++(int)
	{
		safe_iterator ri(*this);
		++ix;
		return ri;
	}

	safe_iterator& operator--()
		{ --ix; return *this; }

	safe_iterator operator--(int)
	{
		safe_iterator ri(*this);
		--ix;
		return ri;
	}

	safe_iterator operator+(difference_type n) const
		{ return safe_iterator(ptr, ix + n); }

	safe_iterator& operator+=(difference_type n)
		{ ix += n; return *this; }

	safe_iterator operator-(difference_type n) const
		{ return safe_iterator(ptr, ix - n); }

	difference_type operator-(const safe_iterator& other) const
		{ return distance(*this, other); }

	safe_iterator& operator-=(difference_type n)
		{ ix -= n; return *this; }

	reference operator[](difference_type n) const
		{ return ptr->at(ix + n); }

	bool operator==(const safe_iterator& ri) const
		{ return ptr == ri.ptr && ix == ri.ix; }

	bool operator!=(const safe_iterator& ri) const
		{ return !operator==(ri); }

	pointer _Unwrapped() const {
		return get_raw_ptr();
	}

	void _Seek_to(pointer to) {
		ix = std::distance(ptr->begin(), to);
	}

	bool operator<(const safe_iterator& ri) const {
		return ptr == ri.ptr && ix < ri.ix;
	}

	bool operator<=(const safe_iterator& ri) const {
		return ptr == ri.ptr && ix <= ri.ix;
	}

	// constexpr bool operator>=(const safe_iterator& ri) const {
	// 	return !operator<(ri);
	// }
	
};

template <typename T, typename Arr>
typename safe_iterator<T, Arr>::difference_type distance(const safe_iterator<T, Arr>& l, const safe_iterator<T, Arr>& r) {

		return (l.ptr == r.ptr)? l.ix - r.ix : 0;
}



} //namespace detail
} // namespace safememory

#ifdef NODECPP_WINDOWS
// this is to allow MS to optimize algorightms like std::find using TMP

namespace safememory {
namespace detail {

template <typename T, typename Arr>
constexpr void _Verify_range(const unsafe_iterator<T, Arr>& _First, const unsafe_iterator<T, Arr>& _Last) {
	if(!(_First <= _Last))
		throw std::out_of_range("_Verify_range");
}

template <typename T, typename Arr>
constexpr void _Verify_range(const safe_iterator<T, Arr>& _First, const safe_iterator<T, Arr>& _Last) {
	if(!(_First <= _Last))
		throw std::out_of_range("_Verify_range");
}

}
}

namespace std {

	namespace sfd = safememory::detail;	


template <typename T, typename Arr>
struct _Unwrappable<sfd::unsafe_iterator<T, Arr>, sfd::unsafe_iterator<T, Arr>> : std::true_type {
};

template <typename T, typename Arr>
struct _Wrapped_seekable<sfd::unsafe_iterator<T, Arr>, T*> : std::true_type {
};

template <typename T, typename Arr>
struct _Range_verifiable<sfd::unsafe_iterator<T, Arr>, sfd::unsafe_iterator<T, Arr>> : std::true_type {
};


template <typename T, typename Arr>
struct _Unwrappable<sfd::safe_iterator<T, Arr>, sfd::safe_iterator<T, Arr>> : std::true_type {
};

template <typename T, typename Arr>
struct _Wrapped_seekable<sfd::safe_iterator<T, Arr>, T*> : std::true_type {
};

template <typename T, typename Arr>
struct _Range_verifiable<sfd::safe_iterator<T, Arr>, sfd::safe_iterator<T, Arr>> : std::true_type {
};


} //namespace std
#endif //NODECPP_WINDOWS


#endif // SAFEMEMORY_DETAIL_SAFE_ALLOC_H

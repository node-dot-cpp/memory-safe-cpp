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
#include <iterator>

namespace safememory {

using ::nodecpp::safememory::owning_ptr;
using ::nodecpp::safememory::soft_ptr;
using ::nodecpp::safememory::memory_safety;
using ::nodecpp::safememory::make_owning;

namespace detail {

enum class iterator_validity {
	Null,                // default constructed iterator
	ValidCanDeref,       // valid, pointing a current element in the container
	ValidEnd,			 // valid, pointing to end()
	InvalidZoombie,	     // invalid but not escaping safememory rules 
	xxx_Broken_xxx       // invalid and escaping safety rules
};


template<class T>
struct array_of2
{
	typedef array_of2<T> this_type;

	size_t _capacity = 0;

	//TODO fix sizeof(this) used for allocation
	union {
		T _begin[1];
		char dummy;
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

	T& at_unsafe(size_t ix) {
		return _begin[ix];
	}
	
	const T& at_unsafe(size_t ix) const {
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
		
		// TODO here we should fine tune the sizes of array_of2<T> 
		size_t total = head + sizeof(array_of2<_Ty>) + (sizeof(_Ty) * size);
		void* data = zombieAllocate(total);

		// non trivial types get zeroed memory, just in case we get to deref
		// a non initialized position
		if constexpr (!std::is_trivial<_Ty>::value)
			std::memset(data, 0, total);

		array_of2<_Ty>* dataForObj = reinterpret_cast<array_of2<_Ty>*>(reinterpret_cast<uintptr_t>(data) + head);
		owning_ptr_impl<array_of2<_Ty>> op(make_owning_t(), dataForObj);
		// void* stackTmp = thg_stackPtrForMakeOwningCall;
		// thg_stackPtrForMakeOwningCall = dataForObj;
		/*array_of2<_Ty>* objPtr = */::new ( dataForObj ) array_of2<_Ty>(size);
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
	typedef typename std::remove_const<T>::type non_const_value_type;
	typedef T									value_type;
	typedef std::ptrdiff_t                      difference_type;
	typedef T*   								pointer;
	typedef T&	 								reference;
	typedef SoftArrayOfPtr  					soft_array_of_prt;


// private:
	pointer mIterator = nullptr;

public:
	unsafe_iterator() {}

	explicit unsafe_iterator(soft_array_of_prt arr)
		: mIterator(arr->begin()) {}

	unsafe_iterator(soft_array_of_prt arr, size_t ix)
		: mIterator(arr->begin() + ix) {}

	unsafe_iterator(soft_array_of_prt, pointer ptr)
		: mIterator(ptr) {}

	unsafe_iterator(pointer ptr)
		: mIterator(ptr) {}

	unsafe_iterator(const unsafe_iterator& ri) = default;
	unsafe_iterator& operator=(const unsafe_iterator& ri) = default;

	template<typename NonConstT, typename X = std::enable_if_t<std::is_same<NonConstT, non_const_value_type>::value>>
	unsafe_iterator(const unsafe_iterator<NonConstT, soft_array_of_prt>& ri)
		: mIterator(ri.mIterator) {}

	template<typename NonConstT, typename X = std::enable_if_t<std::is_same<NonConstT, non_const_value_type>::value>>
	unsafe_iterator& operator=(const unsafe_iterator<NonConstT, soft_array_of_prt>& ri) {
		mIterator = ri.mIterator;
		return *this;
	}


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
	typedef T									value_type;
	typedef typename std::remove_const<T>::type non_const_value_type;
	typedef std::ptrdiff_t                      difference_type;
	typedef T*   								pointer;
	typedef T&									reference;
	typedef SoftArrayOfPtr 						soft_array_of_prt;

// private:
	soft_array_of_prt arr;
	size_t ix = 0;


public:
	safe_iterator() {}
	explicit safe_iterator(soft_array_of_prt arr)
		: arr(arr), ix(0) {}

	safe_iterator(soft_array_of_prt arr, size_t ix)
		: arr(arr), ix(ix) {}

	safe_iterator(soft_array_of_prt arr, pointer to)
		: arr(arr) {
			ix = std::distance(arr->begin(), to);
		}

	// GCC and clang fail to generate defaultd copy ctor/assign
	safe_iterator(const safe_iterator& ri) 
		:arr(ri.arr), ix(ri.ix) {}
		
	safe_iterator& operator=(const safe_iterator& ri) {
		if(this != &ri) {
			this->arr = ri.arr;
			this->ix = ri.ix;
		}
		return *this;
	}

	// allow non-const to const convertion
	template<typename NonConstT, typename X = std::enable_if_t<std::is_same<NonConstT, non_const_value_type>::value>>
	safe_iterator(const safe_iterator<NonConstT, soft_array_of_prt>& ri)
		: arr(ri.arr), ix(ri.ix) {}

	// allow non-const to const convertion
	template<typename NonConstT, typename X = std::enable_if_t<std::is_same<NonConstT, non_const_value_type>::value>>
	safe_iterator& operator=(const safe_iterator<NonConstT, soft_array_of_prt>& ri) {
		this->arr = ri.arr;
		this->ix = ri.ix;
		return *this;
	}


	pointer get_raw_ptr() const {
		return arr->get_raw_ptr(ix);
	}

	reference operator*() const
	{
		return arr->at(ix);
	}

	pointer operator->() const
		{ return arr->get_raw_ptr(ix); }

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
		{ return safe_iterator(arr, ix + n); }

	safe_iterator& operator+=(difference_type n)
		{ ix += n; return *this; }

	safe_iterator operator-(difference_type n) const
		{ return safe_iterator(arr, ix - n); }

	difference_type operator-(const safe_iterator& other) const
		{ return distance(*this, other); }

	safe_iterator& operator-=(difference_type n)
		{ ix -= n; return *this; }

	reference operator[](difference_type n) const
		{ return arr->at(ix + n); }

	bool operator==(const safe_iterator& ri) const
		{ return arr == ri.arr && ix == ri.ix; }

	bool operator!=(const safe_iterator& ri) const
		{ return !operator==(ri); }

	pointer _Unwrapped() const {
		return get_raw_ptr();
	}

	void _Seek_to(pointer to) {
		ix = std::distance(arr->begin(), to);
	}

	bool operator<(const safe_iterator& ri) const {
		if(arr == ri.arr)
			return ix < ri.ix;

		throw std::invalid_argument("iterators don't match");
	}

	bool operator<=(const safe_iterator& ri) const {
		if(arr == ri.arr)
			return ix <= ri.ix;

		throw std::invalid_argument("iterators don't match");
	}

	// constexpr bool operator>=(const safe_iterator& ri) const {
	// 	return !operator<(ri);
	// }
	
};

template <typename T, typename Arr>
typename safe_iterator<T, Arr>::difference_type distance(const safe_iterator<T, Arr>& l, const safe_iterator<T, Arr>& r) {
	if(l.arr == r.arr)
		return l.ix - r.ix;
	
	throw std::invalid_argument("Iterators don't match");
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
		throw std::invalid_argument("Iterators range invalid");
}

template <typename T, typename Arr>
constexpr void _Verify_range(const safe_iterator<T, Arr>& _First, const safe_iterator<T, Arr>& _Last) {
	if(!(_First <= _Last))
		throw std::invalid_argument("Iterators range invalid");
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

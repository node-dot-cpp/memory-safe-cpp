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
#include <safe_ptr_with_zero_offset.h>
#include <iterator>

namespace safememory {

using ::nodecpp::safememory::owning_ptr;
using ::nodecpp::safememory::soft_ptr;
using ::nodecpp::safememory::memory_safety;
using ::nodecpp::safememory::make_owning;
using ::nodecpp::safememory::memory_safety;

namespace detail {

template<class T, memory_safety is_safe>
using soft_ptr_with_zero_offset = std::conditional_t<is_safe == memory_safety::none,
			::nodecpp::safememory::lib_helpers::soft_ptr_with_zero_offset_no_checks<T>,
			::nodecpp::safememory::lib_helpers::soft_ptr_with_zero_offset_impl<T>>;



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

	static
	size_t calculateSize(size_t size) {
		// TODO here we should fine tune the sizes of array_of2<T> 
		return sizeof(array_of2<T>) + (sizeof(T) * size)
	}
};


template<class T>
NODISCARD nodecpp::safememory::owning_ptr<array_of2<T>> make_owning_array_of_impl(size_t size) {
	using namespace nodecpp::safememory;
	size_t head = sizeof(FirstControlBlock) - getPrefixByteCount();
	
	// TODO here we should fine tune the sizes of array_of2<T> 
	size_t total = head + sizeof(array_of2<T>) + (sizeof(T) * size);
	void* data = zombieAllocate(total);

	// non trivial types get zeroed memory, just in case we get to deref
	// a non initialized position
	if constexpr (!std::is_trivial<T>::value)
		std::memset(data, 0, total);

	array_of2<T>* dataForObj = reinterpret_cast<array_of2<T>*>(reinterpret_cast<uintptr_t>(data) + head);
	owning_ptr_impl<array_of2<T>> op(make_owning_t(), dataForObj);
	// void* stackTmp = thg_stackPtrForMakeOwningCall;
	// thg_stackPtrForMakeOwningCall = dataForObj;
	/*array_of2<_Ty>* objPtr = */::new ( dataForObj ) array_of2<T>(size);
	// thg_stackPtrForMakeOwningCall = stackTmp;
	//return owning_ptr_impl<_Ty>(objPtr);
	return op;
}

template<class T, memory_safety is_safe>
NODISCARD 
auto make_owning_array_of(size_t size) -> owning_ptr<array_of2<T>, is_safe> {
	using namespace nodecpp::safememory;
	if constexpr ( is_safe == memory_safety::none ) {
		size_t head = 0;
		size_t total = head + sizeof(array_of2<T>) + (sizeof(T) * size);
		void* data = allocate( total );
		array_of2<T>* dataForObj = reinterpret_cast<array_of2<T>*>(reinterpret_cast<uintptr_t>(data) + head);
		owning_ptr<T, memory_safety::none> op( make_owning_t(), dataForObj );
		/*array_of2<T>* objPtr = */::new ( dataForObj ) array_of2<T>(size);
		return op;
	}
	else
		return make_owning_array_of_impl<T>(size);
}


template <typename T, typename SoftArrayOfPtr = soft_ptr_with_zero_offset<array_of2<typename std::remove_const<T>::type>, memory_safety::none>>
class safe_iterator_no_checks
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
	safe_iterator_no_checks() {}

	explicit safe_iterator_no_checks(soft_array_of_prt arr)
		: mIterator(arr->begin()) {}

	safe_iterator_no_checks(soft_array_of_prt arr, size_t ix)
		: mIterator(arr->begin() + ix) {}

	safe_iterator_no_checks(soft_array_of_prt, pointer ptr)
		: mIterator(ptr) {}

	safe_iterator_no_checks(pointer ptr)
		: mIterator(ptr) {}

	safe_iterator_no_checks(const safe_iterator_no_checks& ri) = default;
	safe_iterator_no_checks& operator=(const safe_iterator_no_checks& ri) = default;

	template<typename NonConstT, typename X = std::enable_if_t<std::is_same<NonConstT, non_const_value_type>::value>>
	safe_iterator_no_checks(const safe_iterator_no_checks<NonConstT, soft_array_of_prt>& ri)
		: mIterator(ri.mIterator) {}

	template<typename NonConstT, typename X = std::enable_if_t<std::is_same<NonConstT, non_const_value_type>::value>>
	safe_iterator_no_checks& operator=(const safe_iterator_no_checks<NonConstT, soft_array_of_prt>& ri) {
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

	safe_iterator_no_checks& operator++()
		{ ++mIterator; return *this; }

	safe_iterator_no_checks operator++(int)
	{
		safe_iterator_no_checks ri(*this);
		++mIterator;
		return ri;
	}

	safe_iterator_no_checks& operator--()
		{ --mIterator; return *this; }

	safe_iterator_no_checks operator--(int)
	{
		safe_iterator_no_checks ri(*this);
		--mIterator;
		return ri;
	}

	safe_iterator_no_checks operator+(difference_type n) const
		{ return safe_iterator_no_checks(mIterator + n); }

	safe_iterator_no_checks& operator+=(difference_type n)
		{ mIterator += n; return *this; }

	safe_iterator_no_checks operator-(difference_type n) const
		{ return safe_iterator_no_checks(mIterator - n); }

	difference_type operator-(const safe_iterator_no_checks& other) const
		{ return distance(*this, other); }

	safe_iterator_no_checks& operator-=(difference_type n)
		{ mIterator -= n; return *this; }

	reference operator[](difference_type n) const
		{ return mIterator[n]; }

	bool operator==(const safe_iterator_no_checks& ri) const
		{ return mIterator == ri.mIterator; }

	bool operator!=(const safe_iterator_no_checks& ri) const
		{ return !operator==(ri); }

	bool operator<(const safe_iterator_no_checks& ri) const {
		return mIterator < ri.mIterator;
	}

	bool operator>(const safe_iterator_no_checks& ri) const {
		return mIterator > ri.mIterator;
	}

	bool operator<=(const safe_iterator_no_checks& ri) const {
		return !operator>(ri);
	}

	bool operator>=(const safe_iterator_no_checks& ri) const {
		return !operator<(ri);
	}

	pointer _Unwrapped() const {
		return mIterator;
	}

	void _Seek_to(pointer ptr) {
		mIterator = ptr;
	}
};

template <typename T, typename Arr>
typename safe_iterator_no_checks<T, Arr>::difference_type distance(const safe_iterator_no_checks<T, Arr>& l, const safe_iterator_no_checks<T, Arr>& r) {

		return l.mIterator - r.mIterator;
}


template <typename T, typename SoftArrayOfPtr = soft_ptr_with_zero_offset<array_of2<typename std::remove_const<T>::type>, memory_safety::safe>>
class safe_iterator_impl
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
	safe_iterator_impl() {}
	explicit safe_iterator_impl(soft_array_of_prt arr)
		: arr(arr), ix(0) {}

	safe_iterator_impl(soft_array_of_prt arr, size_t ix)
		: arr(arr), ix(ix) {}

	safe_iterator_impl(soft_array_of_prt arr, pointer to)
		: arr(arr) {
			ix = std::distance(arr->begin(), to);
		}

	// GCC and clang fail to generate defaultd copy ctor/assign
	safe_iterator_impl(const safe_iterator_impl& ri) 
		:arr(ri.arr), ix(ri.ix) {}
		
	safe_iterator_impl& operator=(const safe_iterator_impl& ri) {
		if(this != &ri) {
			this->arr = ri.arr;
			this->ix = ri.ix;
		}
		return *this;
	}

	// allow non-const to const convertion
	template<typename NonConstT, typename X = std::enable_if_t<std::is_same<NonConstT, non_const_value_type>::value>>
	safe_iterator_impl(const safe_iterator_impl<NonConstT, soft_array_of_prt>& ri)
		: arr(ri.arr), ix(ri.ix) {}

	// allow non-const to const convertion
	template<typename NonConstT, typename X = std::enable_if_t<std::is_same<NonConstT, non_const_value_type>::value>>
	safe_iterator_impl& operator=(const safe_iterator_impl<NonConstT, soft_array_of_prt>& ri) {
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

	safe_iterator_impl& operator++()
		{ ++ix; return *this; }

	safe_iterator_impl operator++(int)
	{
		safe_iterator_impl ri(*this);
		++ix;
		return ri;
	}

	safe_iterator_impl& operator--()
		{ --ix; return *this; }

	safe_iterator_impl operator--(int)
	{
		safe_iterator_impl ri(*this);
		--ix;
		return ri;
	}

	safe_iterator_impl operator+(difference_type n) const
		{ return safe_iterator_impl(arr, ix + n); }

	safe_iterator_impl& operator+=(difference_type n)
		{ ix += n; return *this; }

	safe_iterator_impl operator-(difference_type n) const
		{ return safe_iterator_impl(arr, ix - n); }

	difference_type operator-(const safe_iterator_impl& other) const
		{ return distance(*this, other); }

	safe_iterator_impl& operator-=(difference_type n)
		{ ix -= n; return *this; }

	reference operator[](difference_type n) const
		{ return arr->at(ix + n); }

	bool operator==(const safe_iterator_impl& ri) const {
		if (arr == ri.arr)
			return ix == ri.ix;

		throw std::invalid_argument("Iterators don't match");
	}

	bool operator!=(const safe_iterator_impl& ri) const {
		return !operator==(ri);
	}

	bool operator<(const safe_iterator_impl& ri) const {
		if (arr == ri.arr)
			return ix < ri.ix;

		throw std::invalid_argument("Iterators don't match");
	}

	bool operator>(const safe_iterator_impl& ri) const {
		if (arr == ri.arr)
			return ix > ri.ix;

		throw std::invalid_argument("Iterators don't match");
	}

	bool operator<=(const safe_iterator_impl& ri) const {
		return !operator>(ri);
	}

	bool operator>=(const safe_iterator_impl& ri) const {
		return !operator<(ri);
	}

	pointer _Unwrapped() const {
		return get_raw_ptr();
	}

	void _Seek_to(pointer to) {
		ix = std::distance(arr->begin(), to);
	}
};

template <typename T, typename Arr>
typename safe_iterator_impl<T, Arr>::difference_type distance(const safe_iterator_impl<T, Arr>& l, const safe_iterator_impl<T, Arr>& r) {
	if(l.arr == r.arr)
		return l.ix - r.ix;
	
	throw std::invalid_argument("Iterators don't match");
}

template<class T, memory_safety is_safe>
using safe_array_iterator = std::conditional_t<is_safe == memory_safety::none,
			safe_iterator_no_checks<T>, safe_iterator_impl<T>>;


} // namespace detail
} // namespace safememory

#ifdef NODECPP_WINDOWS
// this is to allow MS to optimize algorightms like std::find using TMP

namespace safememory {
namespace detail {

template <typename T, typename Arr>
constexpr void _Verify_range(const safe_iterator_no_checks<T, Arr>& _First, const safe_iterator_no_checks<T, Arr>& _Last) {
	if(!(_First <= _Last))
		throw std::invalid_argument("Iterators range invalid");
}

template <typename T, typename Arr>
constexpr void _Verify_range(const safe_iterator_impl<T, Arr>& _First, const safe_iterator_impl<T, Arr>& _Last) {
	if(!(_First <= _Last))
		throw std::invalid_argument("Iterators range invalid");
}

}
}

namespace std {

	namespace sfd = safememory::detail;	


template <typename T, typename Arr>
struct _Unwrappable<sfd::safe_iterator_no_checks<T, Arr>, sfd::safe_iterator_no_checks<T, Arr>> : std::true_type {
};

template <typename T, typename Arr>
struct _Wrapped_seekable<sfd::safe_iterator_no_checks<T, Arr>, T*> : std::true_type {
};

template <typename T, typename Arr>
struct _Range_verifiable<sfd::safe_iterator_no_checks<T, Arr>, sfd::safe_iterator_no_checks<T, Arr>> : std::true_type {
};


template <typename T, typename Arr>
struct _Unwrappable<sfd::safe_iterator_impl<T, Arr>, sfd::safe_iterator_impl<T, Arr>> : std::true_type {
};

template <typename T, typename Arr>
struct _Wrapped_seekable<sfd::safe_iterator_impl<T, Arr>, T*> : std::true_type {
};

template <typename T, typename Arr>
struct _Range_verifiable<sfd::safe_iterator_impl<T, Arr>, sfd::safe_iterator_impl<T, Arr>> : std::true_type {
};


} //namespace std
#endif //NODECPP_WINDOWS


#endif // SAFEMEMORY_DETAIL_SAFE_ALLOC_H

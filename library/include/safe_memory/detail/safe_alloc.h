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

#ifndef SAFE_MEMORY_DETAIL_SAFE_ALLOC_H
#define SAFE_MEMORY_DETAIL_SAFE_ALLOC_H

#include <safe_memory/safe_ptr.h>
#include <safe_memory/detail/safe_ptr_with_zero_offset.h>
#include <iterator>

namespace safe_memory::detail {

enum class iterator_validity {
	Null,                // default constructed iterator
	ValidCanDeref,       // valid, pointing a current element in the container
	ValidEnd,			 // valid, pointing to end()
	InvalidZoombie,	     // invalid but not escaping safe_memory rules 
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

	[[noreturn]] static
	void throwPointerOutOfRange(const char* msg) {
		throw std::out_of_range(msg);
	}

public:
	array_of2(size_t capacity) :_capacity(capacity) {}

	array_of2(const array_of2&) = delete;
	array_of2(array_of2&&) = delete;

	array_of2& operator=(const array_of2&) = delete;
	array_of2& operator=(array_of2&&) = delete;

	~array_of2() {}

	template<memory_safety Safety>
	T& at(size_t ix) {
		if constexpr ( Safety == memory_safety::safe ) {
			if(ix >= _capacity)
				throwPointerOutOfRange("array_of2::at(): ix >= _capacity");
		}

		return _begin[ix];
	}

	template<memory_safety Safety>
	const T& at(size_t ix) const {
		if constexpr ( Safety == memory_safety::safe ) {
			if(ix >= _capacity)
				throwPointerOutOfRange("array_of2::at(): ix >= _capacity");
		}

		return _begin[ix];
	}

	T& at_unsafe(size_t ix) {
		return at<memory_safety::none>(ix);
	}
	
	const T& at_unsafe(size_t ix) const {
		return at<memory_safety::none>(ix);
	}

	//unsafe function, allow returning a non-derefenceable pointer as end()
	T* get_raw_ptr(size_t ix) {
		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, ix <= _capacity );
		return _begin + ix;
	}

	//unsafe function, ptr should have been validated
	size_t get_index(const T* ptr) const {
		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, static_cast<size_t>(ptr - _begin) <= _capacity );
		return static_cast<size_t>(ptr - _begin);
	}

	size_t capacity() const { return _capacity; }

	T* begin() { return _begin; }
	const T* begin() const { return _begin; }

	T* end() { return _begin + _capacity; }
	const T* end() const { return _begin + _capacity; }

	static
	size_t calculateSize(size_t size) {
		// TODO here we should fine tune the sizes of array_of2<T> 
		return sizeof(this_type) + (sizeof(T) * size);
	}
};


template<class T>
NODISCARD owning_ptr<array_of2<T>, memory_safety::safe> make_owning_array_of_impl(size_t size) {

	typedef array_of2<T> array_type;

	size_t head = sizeof(nodecpp::safememory::FirstControlBlock) - nodecpp::safememory::getPrefixByteCount();
	
	// TODO here we should fine tune the sizes of array_of2<T> 
	size_t total = head + array_type::calculateSize(size);
	void* data = nodecpp::safememory::zombieAllocate(total);

	// non trivial types get zeroed memory, just in case we get to deref
	// a non initialized position
	if constexpr (!std::is_trivial<T>::value)
		std::memset(data, 0, total);

	array_type* dataForObj = reinterpret_cast<array_type*>(reinterpret_cast<uintptr_t>(data) + head);
	owning_ptr<array_of2<T>, memory_safety::safe> op(make_owning_t(), dataForObj);
	// void* stackTmp = thg_stackPtrForMakeOwningCall;
	// thg_stackPtrForMakeOwningCall = dataForObj;
	/*array_of2<_Ty>* objPtr = */::new ( dataForObj ) array_type(size);
	// thg_stackPtrForMakeOwningCall = stackTmp;
	//return owning_ptr_impl<_Ty>(objPtr);
	return op;
}

template<class T, memory_safety Safety>
NODISCARD 
auto make_owning_array_of(size_t size) -> owning_ptr<array_of2<T>, Safety> {

	typedef array_of2<T> array_type;

	if constexpr ( Safety == memory_safety::none ) {
		size_t head = 0;
		size_t total = head + array_type::calculateSize(size);
		void* data = nodecpp::safememory::allocate( total );
		array_type* dataForObj = reinterpret_cast<array_type*>(reinterpret_cast<uintptr_t>(data) + head);
		owning_ptr<array_type, Safety> op( make_owning_t(), dataForObj );
		/*array_of2<T>* objPtr = */::new ( dataForObj ) array_type(size);
		return op;
	}
	else
		return make_owning_array_of_impl<T>(size);
}


template <typename T, bool bConst>
class safe_iterator_no_checks
{
public:
	typedef std::random_access_iterator_tag  			iterator_category;
	typedef std::conditional_t<bConst, const T, T>		value_type;
	typedef std::ptrdiff_t                      		difference_type;
	typedef value_type*								 	pointer;
	typedef value_type&								 	reference;
	typedef const T*									const_pointer;

	static constexpr memory_safety is_safe = memory_safety::none;

	// for non-const to const conversion
	template<typename, bool>
	friend class safe_iterator_no_checks;

private:
	pointer mIterator = nullptr;

public:

	safe_iterator_no_checks() {}

private:
	explicit safe_iterator_no_checks(pointer ptr)
		: mIterator(ptr) {}
public:
	template<class PTR>
	static safe_iterator_no_checks makeIx(const PTR& arr, size_t ix = 0) {
		// non safe function, we know ix is in range
		return safe_iterator_no_checks(arr->get_raw_ptr(ix));
	}
	
	template<class PTR>
	static safe_iterator_no_checks makePtr(const PTR&, pointer ptr) {
		return safe_iterator_no_checks(ptr);
	}

	safe_iterator_no_checks(const safe_iterator_no_checks& ri) = default;
	safe_iterator_no_checks& operator=(const safe_iterator_no_checks& ri) = default;

	safe_iterator_no_checks(safe_iterator_no_checks&& ri) = default;
	safe_iterator_no_checks& operator=(safe_iterator_no_checks&& ri) = default;

	// allow non-const to const convertion
	template<bool B, typename X = std::enable_if_t<bConst && !B>>
	safe_iterator_no_checks(const safe_iterator_no_checks<T, B>& ri)
		: mIterator(ri.mIterator) {}

	// allow non-const to const convertion
	template<bool B, typename X = std::enable_if_t<bConst && !B>>
	safe_iterator_no_checks& operator=(const safe_iterator_no_checks<T, B>& ri) {
		mIterator = ri.mIterator;
		return *this;
	}
	
	const_pointer get_raw_ptr() const {
		return mIterator;
	}

	reference operator*() const
	{
		return *mIterator;
	}

	pointer operator->() const
		{ return mIterator; }

	safe_iterator_no_checks& operator++() noexcept
		{ ++mIterator; return *this; }

	safe_iterator_no_checks operator++(int) noexcept
	{
		safe_iterator_no_checks ri(*this);
		++mIterator;
		return ri;
	}

	safe_iterator_no_checks& operator--() noexcept
		{ --mIterator; return *this; }

	safe_iterator_no_checks operator--(int) noexcept
	{
		safe_iterator_no_checks ri(*this);
		--mIterator;
		return ri;
	}

	safe_iterator_no_checks operator+(difference_type n) const noexcept
		{ return safe_iterator_no_checks(mIterator + n); }

	safe_iterator_no_checks& operator+=(difference_type n) noexcept
		{ mIterator += n; return *this; }

	safe_iterator_no_checks operator-(difference_type n) const noexcept
		{ return safe_iterator_no_checks(mIterator - n); }

	difference_type operator-(const safe_iterator_no_checks& other) const noexcept
		{ return mIterator - other.mIterator; }

	safe_iterator_no_checks& operator-=(difference_type n) noexcept
		{ mIterator -= n; return *this; }

	reference operator[](difference_type n) const
		{ return mIterator[n]; }

	bool operator==(const safe_iterator_no_checks& ri) const noexcept
		{ return mIterator == ri.mIterator; }

	bool operator!=(const safe_iterator_no_checks& ri) const noexcept
		{ return !operator==(ri); }

	bool operator<(const safe_iterator_no_checks& ri) const noexcept {
		return mIterator < ri.mIterator;
	}

	bool operator>(const safe_iterator_no_checks& ri) const noexcept {
		return mIterator > ri.mIterator;
	}

	bool operator<=(const safe_iterator_no_checks& ri) const noexcept {
		return !operator>(ri);
	}

	bool operator>=(const safe_iterator_no_checks& ri) const noexcept {
		return !operator<(ri);
	}

	constexpr bool is_safe_range(const safe_iterator_no_checks& ri) const {
		return true;
	}

	template<typename PTR>
	constexpr bool is_safe_range(const PTR& otherArr, size_t otherIx) const {
		return true;
	}


	iterator_validity validate_iterator(const safe_iterator_no_checks& b, const safe_iterator_no_checks& end) const noexcept {
		if(mIterator == nullptr)
			return iterator_validity::Null;
		else if(mIterator >= b.mIterator) {
			if(mIterator < end.mIterator)
				return detail::iterator_validity::ValidCanDeref;
			else if(mIterator == end.mIterator)
				return detail::iterator_validity::ValidEnd;
		}
		return iterator_validity::xxx_Broken_xxx;
	}

	pointer _Unwrapped() const {
		return mIterator;
	}

	void _Seek_to(pointer ptr) {
		mIterator = ptr;
	}
};

template <typename T, bool C>
typename safe_iterator_no_checks<T, C>::difference_type distance(const safe_iterator_no_checks<T, C>& l, const safe_iterator_no_checks<T, C>& r) {
		return r - l;
}


template <typename T, bool bConst, memory_safety Safety>
class safe_iterator_impl
{
public:
	typedef std::random_access_iterator_tag  			iterator_category;
	typedef std::conditional_t<bConst, const T, T>		value_type;
	typedef std::ptrdiff_t                      		difference_type;
	typedef value_type*								 	pointer;
	typedef value_type&								 	reference;
	typedef const T*									const_pointer;
	typedef soft_ptr<array_of2<T>, Safety> 				soft_array_of_prt;

	static constexpr memory_safety is_safe = Safety;

	// for non-const to const conversion
	template<typename, bool, memory_safety>
	friend class safe_iterator_impl;

private:
	soft_array_of_prt arr;
	size_t ix = 0;


public:

	safe_iterator_impl() {}

private:
	template<class PTR>
	safe_iterator_impl(const PTR& arr, size_t ix)
		: arr(arr), ix(ix) {}

public:
	template<class PTR>
	static safe_iterator_impl makeIx(const PTR& arr, size_t ix = 0) {
		return safe_iterator_impl(arr, ix);
	}

	template<class PTR>
	static safe_iterator_impl makePtr(const PTR& arr, pointer to) {
		return safe_iterator_impl(arr, arr->get_index(to));
	}

	// GCC and clang fail to generate defaultd copy ctor/assign
	safe_iterator_impl(const safe_iterator_impl& ri) = default;
	safe_iterator_impl& operator=(const safe_iterator_impl& ri) = default;

	safe_iterator_impl(safe_iterator_impl&& ri) = default; 
	safe_iterator_impl& operator=(safe_iterator_impl&& ri) = default;


	// allow non-const to const convertion
	template<bool B, typename X = std::enable_if_t<bConst && !B>>
	safe_iterator_impl(const safe_iterator_impl<T, B, is_safe>& ri)
		: arr(ri.arr), ix(ri.ix) {}

	// allow non-const to const convertion
	template<bool B, typename X = std::enable_if_t<bConst && !B>>
	safe_iterator_impl& operator=(const safe_iterator_impl<T, B, is_safe>& ri) {
		this->arr = ri.arr;
		this->ix = ri.ix;
		return *this;
	}

	const_pointer get_raw_ptr() const {
		// this is unsafe function, always called after ix was validated
		return arr->get_raw_ptr(ix);
	}

	reference operator*() const
	{
		return arr->template at<is_safe>(ix);
	}

	pointer operator->() const
		{ return &(arr->template at<is_safe>(ix)); }

	safe_iterator_impl& operator++() noexcept
		{ ++ix; return *this; }

	safe_iterator_impl operator++(int) noexcept
	{
		safe_iterator_impl ri(*this);
		++ix;
		return ri;
	}

	safe_iterator_impl& operator--() noexcept
		{ --ix; return *this; }

	safe_iterator_impl operator--(int) noexcept
	{
		safe_iterator_impl ri(*this);
		--ix;
		return ri;
	}

	safe_iterator_impl operator+(difference_type n) const noexcept
		{ return safe_iterator_impl(arr, ix + n); }

	safe_iterator_impl& operator+=(difference_type n) noexcept
		{ ix += n; return *this; }

	safe_iterator_impl operator-(difference_type n) const noexcept
		{ return safe_iterator_impl(arr, ix - n); }

	difference_type operator-(const safe_iterator_impl& other) const noexcept {
		if(arr == other.arr)
			return ix - other.ix;
		else
			return 0;
	}

	safe_iterator_impl& operator-=(difference_type n) noexcept
		{ ix -= n; return *this; }

	reference operator[](difference_type n) const
		{ return arr->template at<is_safe>(ix + n); }

	bool operator==(const safe_iterator_impl& ri) const noexcept {
		return arr == ri.arr && ix == ri.ix;
	}

	bool operator!=(const safe_iterator_impl& ri) const noexcept {
		return !operator==(ri);
	}

	bool operator<(const safe_iterator_impl& ri) const noexcept {
		return arr == ri.arr && ix < ri.ix;
	}

	bool operator>(const safe_iterator_impl& ri) const noexcept {
		return arr == ri.arr && ix > ri.ix;
	}

	bool operator<=(const safe_iterator_impl& ri) const noexcept {
		return !operator>(ri);
	}

	bool operator>=(const safe_iterator_impl& ri) const noexcept {
		return !operator<(ri);
	}

	constexpr bool is_safe_range(const safe_iterator_impl& ri) const {
		if constexpr (is_safe == memory_safety::none)
			return true;
		else {			
			if (arr != nullptr && arr == ri.arr)
				return ix <= ri.ix && ri.ix <= arr->capacity();
			else
				return false;
		}
	}

	template<typename PTR>
	constexpr bool is_safe_range(const PTR& otherArr, size_t otherIx) const {
		if constexpr (is_safe == memory_safety::none)
			return true;
		else {
			if (arr != nullptr && arr == otherArr)
				return ix <= otherIx && otherIx <= arr->capacity();
			else
				return false;
		}
	}

	bool is_end() const {
		return ix >= arr->capacity();
	}

	iterator_validity validate_iterator(const safe_iterator_impl&, const safe_iterator_impl& end) const noexcept {
		if(arr == nullptr)
			return iterator_validity::Null;
		else if(arr == end.arr) {

			if(ix < end.ix)
				return iterator_validity::ValidCanDeref;

			else if(ix == end.ix)
				return iterator_validity::ValidEnd;

			else if(ix < arr->capacity())
				return iterator_validity::InvalidZoombie;
		}
		return iterator_validity::InvalidZoombie;
	}

	pointer _Unwrapped() const {
		return const_cast<pointer>(get_raw_ptr());
	}

	void _Seek_to(pointer to) {
		ix = arr->get_index(to);
	}
};

template <typename T, bool C, memory_safety S>
typename safe_iterator_impl<T, C, S>::difference_type distance(const safe_iterator_impl<T, C, S>& l, const safe_iterator_impl<T, C, S>& r) {
	return r - l;
}


template<class T, bool bConst, memory_safety Safety>
using safe_array_iterator = std::conditional_t<Safety == memory_safety::none,
			safe_iterator_no_checks<T, bConst>, safe_iterator_impl<T, bConst, memory_safety::safe>>;

} // namespace safe_memory::detail

#ifdef NODECPP_WINDOWS
// this is to allow MS to optimize algorightms like std::find using TMP

namespace safe_memory {
namespace detail {

template <typename T, bool C>
constexpr void _Verify_range(const safe_iterator_no_checks<T, C>& _First, const safe_iterator_no_checks<T, C>& _Last) {
	if(!(_First <= _Last))
		throw std::invalid_argument("Iterators range invalid");
}

template <typename T, bool C, memory_safety S>
constexpr void _Verify_range(const safe_iterator_impl<T, C, S>& _First, const safe_iterator_impl<T, C, S>& _Last) {
	if(!(_First.is_safe_range(_Last)))
		throw std::invalid_argument("Iterators range invalid");
}

}
}

namespace std {

namespace sf = safe_memory;	
namespace sfd = safe_memory::detail;	


template <typename T, bool C>
struct _Unwrappable<sfd::safe_iterator_no_checks<T, C>, sfd::safe_iterator_no_checks<T, C>> : std::true_type {
};

template <typename T, bool C>
struct _Wrapped_seekable<sfd::safe_iterator_no_checks<T, C>, T*> : std::true_type {
};

template <typename T, bool C>
struct _Range_verifiable<sfd::safe_iterator_no_checks<T, C>, sfd::safe_iterator_no_checks<T, C>> : std::true_type {
};


template <typename T, bool C, sf::memory_safety S>
struct _Unwrappable<sfd::safe_iterator_impl<T, C, S>, sfd::safe_iterator_impl<T, C, S>> : std::true_type {
};

template <typename T, bool C, sf::memory_safety S>
struct _Wrapped_seekable<sfd::safe_iterator_impl<T, C, S>, T*> : std::true_type {
};

template <typename T, bool C, sf::memory_safety S>
struct _Range_verifiable<sfd::safe_iterator_impl<T, C, S>, sfd::safe_iterator_impl<T, C, S>> : std::true_type {
};


} //namespace std
#endif //NODECPP_WINDOWS


#endif // SAFE_MEMORY_DETAIL_SAFE_ALLOC_H

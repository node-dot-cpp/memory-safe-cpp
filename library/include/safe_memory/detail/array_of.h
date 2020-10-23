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

#ifndef SAFE_MEMORY_DETAIL_ARRAY_OF
#define SAFE_MEMORY_DETAIL_ARRAY_OF

#include <safe_memory/detail/iterator_validity.h>
#include <safe_memory/detail/soft_ptr_with_zero_offset.h>


namespace safe_memory::detail {

using nodecpp::safememory::memory_safety;
using nodecpp::safememory::FirstControlBlock;
using nodecpp::safememory::module_id;
// using nodecpp::safememory::lib_helpers::soft_ptr_with_zero_offset_impl;
// using nodecpp::safememory::lib_helpers::soft_ptr_with_zero_offset_no_checks;
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

template<size_t SZ, class T>
struct fixed_array_of : public array_of<T>
{
	T buff[SZ];

public:
	fixed_array_of(std::initializer_list<T> init) :array_of<T>(SZ) {
		auto jt = array_of<T>::begin();
		auto it = init.begin();
		while(it != init.end()) {
			*jt = *it;
			++it;
			++jt;
		}
	}

	fixed_array_of(const fixed_array_of&) = delete;
	fixed_array_of(fixed_array_of&&) = delete;

	fixed_array_of& operator=(const fixed_array_of&) = delete;
	fixed_array_of& operator=(fixed_array_of&&) = delete;

	~fixed_array_of() {}
};


//mb specializations for array<T>


template<class T>
class soft_ptr_with_zero_offset_impl<array_of<T>> : public soft_ptr_with_zero_offset_base
{
	// friend class owning_ptr_impl<array_of<T>>;
	friend struct ::nodecpp::safememory::FirstControlBlock;


	// template<class TT>
	// friend class soft_ptr_with_zero_offset_base_no_checks;
	template<class TT>
	friend class soft_ptr_with_zero_offset_no_checks;

//	array_of<T>* ptr= nullptr;

	// FirstControlBlock* getControlBlock() const { return getControlBlock_( ptr ); }
	// static FirstControlBlock* getControlBlock(void* t) { return getControlBlock_(t); }

public:

	static constexpr memory_safety is_safe = memory_safety::safe;
 
	soft_ptr_with_zero_offset_impl() { }

	soft_ptr_with_zero_offset_impl( make_zero_offset_t, array_of<T>* raw ) :soft_ptr_with_zero_offset_base(raw) { }

	soft_ptr_with_zero_offset_impl( const soft_ptr_with_zero_offset_impl<array_of<T>>& other ) = default;
	soft_ptr_with_zero_offset_impl& operator=( const soft_ptr_with_zero_offset_impl<array_of<T>>& other ) = default;
	soft_ptr_with_zero_offset_impl( soft_ptr_with_zero_offset_impl<array_of<T>>&& other ) = default;
	soft_ptr_with_zero_offset_impl& operator=( soft_ptr_with_zero_offset_impl<array_of<T>>&& other ) = default;

	soft_ptr_with_zero_offset_impl( std::nullptr_t ) { }
	soft_ptr_with_zero_offset_impl& operator=( std::nullptr_t ) { soft_ptr_with_zero_offset_base::reset(); return *this; }

	using soft_ptr_with_zero_offset_base::swap;

	soft_ptr_impl<array_of<T>> get() const
	{
		if(NODECPP_LIKELY(ptr != nullptr)) {
			FirstControlBlock* cb = getControlBlock_( ptr );
			return soft_ptr_impl<array_of<T>>( cb, ptr );
		}
		else
			return soft_ptr_impl<array_of<T>>();
	}

	using soft_ptr_with_zero_offset_base::operator bool;
	using soft_ptr_with_zero_offset_base::reset;

	bool operator == (const soft_ptr_with_zero_offset_impl<array_of<T>>& other ) const noexcept
		{ return soft_ptr_with_zero_offset_base::operator==(other); }
	bool operator != (const soft_ptr_with_zero_offset_impl<array_of<T>>& other ) const noexcept
		{ return soft_ptr_with_zero_offset_base::operator!=(other); }

	template<class T1>
	bool operator == (const soft_ptr_with_zero_offset_impl<array_of<T1>>& other ) const noexcept
		{ return soft_ptr_with_zero_offset_base::operator==(other); }
	template<class T1>
	bool operator != (const soft_ptr_with_zero_offset_impl<array_of<T1>>& other ) const noexcept
		{ return soft_ptr_with_zero_offset_base::operator!=(other); }

	bool operator == (std::nullptr_t) const noexcept
		{ return soft_ptr_with_zero_offset_base::operator==(nullptr); }
	bool operator != (std::nullptr_t) const noexcept
		{ return soft_ptr_with_zero_offset_base::operator!=(nullptr); }

	array_of<T>& operator * () const noexcept { return *reinterpret_cast<array_of<T>*>(ptr); }
	array_of<T>* operator -> () const noexcept { return reinterpret_cast<array_of<T>*>(ptr); }
	
	T* get_raw_ptr() const noexcept { return ptr ? reinterpret_cast<array_of<T>*>(ptr)->begin() : nullptr; }
	
	operator T*() const noexcept { return get_raw_ptr(); }
	T* operator+(std::ptrdiff_t n) const noexcept { return get_raw_ptr() + n; }
	std::ptrdiff_t operator-(const T* other) const noexcept { return get_raw_ptr() - other; }
	T& operator[](std::size_t n) const noexcept { return *(get_raw_ptr() + n); }

	~soft_ptr_with_zero_offset_impl() {	NODECPP_DEBUG_COUNT_SOFT_PTR_ZERO_OFFSET_DTOR(); }
};


template<class T>
class soft_ptr_with_zero_offset_no_checks<array_of<T>> : public soft_ptr_with_zero_offset_base
{
	// template<class TT>
	// friend class soft_ptr_with_zero_offset_base_no_checks;
	template<class TT>
	friend class soft_ptr_with_zero_offset_no_checks;

	// array_of<T>* ptr = nullptr;

public:

	static constexpr memory_safety is_safe = memory_safety::none;
 
	soft_ptr_with_zero_offset_no_checks() { }
	soft_ptr_with_zero_offset_no_checks( make_zero_offset_t, array_of<T>* raw ) :soft_ptr_with_zero_offset_base(raw) { }

	soft_ptr_with_zero_offset_no_checks( const soft_ptr_with_zero_offset_no_checks<array_of<T>>& other ) = default;
	soft_ptr_with_zero_offset_no_checks& operator=( const soft_ptr_with_zero_offset_no_checks<array_of<T>>& other ) = default;

	soft_ptr_with_zero_offset_no_checks( soft_ptr_with_zero_offset_no_checks<array_of<T>>&& other ) = default;
	soft_ptr_with_zero_offset_no_checks& operator=( soft_ptr_with_zero_offset_no_checks<array_of<T>>&& other ) = default;

	soft_ptr_with_zero_offset_no_checks( std::nullptr_t ) { }
	soft_ptr_with_zero_offset_no_checks& operator=( std::nullptr_t )
		{ soft_ptr_with_zero_offset_base::reset(); return *this; }

	using soft_ptr_with_zero_offset_base::swap;

// 	soft_ptr_no_checks<array_of<T>> get() const
// 	{
// //		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, ptr != nullptr );
// //		FirstControlBlock* cb = getControlBlock_( ptr );
// 		return soft_ptr_no_checks<array_of<T>>( fbc_ptr_t(), ptr );
// 	}

	using soft_ptr_with_zero_offset_base::operator bool;
	using soft_ptr_with_zero_offset_base::reset;


	bool operator == (const soft_ptr_with_zero_offset_no_checks<array_of<T>>& other ) const noexcept
		{ return soft_ptr_with_zero_offset_base::operator==(other); }
	bool operator != (const soft_ptr_with_zero_offset_no_checks<array_of<T>>& other ) const noexcept
		{ return soft_ptr_with_zero_offset_base::operator!=(other); }

	template<class T1>
	bool operator == (const soft_ptr_with_zero_offset_no_checks<array_of<T1>>& other ) const noexcept
		{ return soft_ptr_with_zero_offset_base::operator==(other); }
	template<class T1>
	bool operator != (const soft_ptr_with_zero_offset_no_checks<array_of<T1>>& other ) const noexcept
		{ return soft_ptr_with_zero_offset_base::operator!=(other); }

	bool operator == (std::nullptr_t) const noexcept
		{ return soft_ptr_with_zero_offset_base::operator==(nullptr); }
	bool operator != (std::nullptr_t) const noexcept
		{ return soft_ptr_with_zero_offset_base::operator!=(nullptr); }

	array_of<T>& operator * () const noexcept { return *reinterpret_cast<array_of<T>*>(ptr); }
	array_of<T>* operator -> () const noexcept { return reinterpret_cast<array_of<T>*>(ptr); }

	T* get_raw_ptr() const noexcept { return ptr ? reinterpret_cast<array_of<T>*>(ptr)->begin() : nullptr; }
	operator T*() const noexcept { return get_raw_ptr(); }
	T* operator+(std::ptrdiff_t n) const noexcept { return get_raw_ptr() + n; }
	T& operator[](std::size_t n) const noexcept { return *(get_raw_ptr() + n); }

	~soft_ptr_with_zero_offset_no_checks() { NODECPP_DEBUG_COUNT_SOFT_PTR_ZERO_OFFSET_DTOR(); }
};

template <typename T, bool bConst, template<typename> typename ArrayPtr>
class array_of_iterator_impl
{
public:
	typedef std::random_access_iterator_tag  			iterator_category;
	typedef std::conditional_t<bConst, const T, T>		value_type;
	typedef std::ptrdiff_t                      		difference_type;
	typedef value_type*								 	pointer;
	typedef value_type&								 	reference;
	typedef const T*									const_pointer;
	typedef ArrayPtr<array_of<T>>						soft_array_of_prt;

	static constexpr memory_safety is_safe = memory_safety::safe;

	// for non-const to const conversion
	template<typename, bool, template<typename> typename>
	friend class array_of_iterator_impl;

private:
	soft_array_of_prt arr;
	size_t ix = 0;


public:

	array_of_iterator_impl() {}

private:
	template<class PTR>
	array_of_iterator_impl(const PTR& arr, size_t ix)
		: arr(arr), ix(ix) {}

public:
	template<class PTR>
	static array_of_iterator_impl makeIx(const PTR& arr, size_t ix = 0) {
		return array_of_iterator_impl(arr, ix);
	}

	template<class PTR>
	static array_of_iterator_impl makePtr(const PTR& arr, pointer to) {
		return array_of_iterator_impl(arr, arr->get_index(to));
	}

	// GCC and clang fail to generate defaultd copy ctor/assign
	array_of_iterator_impl(const array_of_iterator_impl& ri) = default;
	array_of_iterator_impl& operator=(const array_of_iterator_impl& ri) = default;

	array_of_iterator_impl(array_of_iterator_impl&& ri) = default; 
	array_of_iterator_impl& operator=(array_of_iterator_impl&& ri) = default;


	// allow non-const to const convertion
	template<bool B, typename X = std::enable_if_t<bConst && !B>>
	array_of_iterator_impl(const array_of_iterator_impl<T, B, ArrayPtr>& ri)
		: arr(ri.arr), ix(ri.ix) {}

	// allow non-const to const convertion
	template<bool B, typename X = std::enable_if_t<bConst && !B>>
	array_of_iterator_impl& operator=(const array_of_iterator_impl<T, B, ArrayPtr>& ri) {
		this->arr = ri.arr;
		this->ix = ri.ix;
		return *this;
	}

	pointer get_raw_ptr() const {
		// this is unsafe function, always called after ix was validated
		return arr ? arr->get_raw_ptr(ix) : nullptr;
	}

	reference operator*() const
	{
		return arr->template at<is_safe>(ix);
	}

	pointer operator->() const
		{ return &(arr->template at<is_safe>(ix)); }

	array_of_iterator_impl& operator++() noexcept
		{ ++ix; return *this; }

	array_of_iterator_impl operator++(int) noexcept
	{
		array_of_iterator_impl ri(*this);
		++ix;
		return ri;
	}

	array_of_iterator_impl& operator--() noexcept
		{ --ix; return *this; }

	array_of_iterator_impl operator--(int) noexcept
	{
		array_of_iterator_impl ri(*this);
		--ix;
		return ri;
	}

	array_of_iterator_impl operator+(difference_type n) const noexcept
		{ return array_of_iterator_impl(arr, ix + n); }

	array_of_iterator_impl& operator+=(difference_type n) noexcept
		{ ix += n; return *this; }

	array_of_iterator_impl operator-(difference_type n) const noexcept
		{ return array_of_iterator_impl(arr, ix - n); }

	difference_type operator-(const array_of_iterator_impl& other) const noexcept {
		if(arr == other.arr)
			return ix - other.ix;
		else
			return 0;
	}

	array_of_iterator_impl& operator-=(difference_type n) noexcept
		{ ix -= n; return *this; }

	reference operator[](difference_type n) const
		{ return arr->template at<is_safe>(ix + n); }

	bool operator==(const array_of_iterator_impl& ri) const noexcept {
		return arr == ri.arr && ix == ri.ix;
	}

	bool operator!=(const array_of_iterator_impl& ri) const noexcept {
		return !operator==(ri);
	}

	bool operator<(const array_of_iterator_impl& ri) const noexcept {
		return arr == ri.arr && ix < ri.ix;
	}

	bool operator>(const array_of_iterator_impl& ri) const noexcept {
		return arr == ri.arr && ix > ri.ix;
	}

	bool operator<=(const array_of_iterator_impl& ri) const noexcept {
		return !operator>(ri);
	}

	bool operator>=(const array_of_iterator_impl& ri) const noexcept {
		return !operator<(ri);
	}

	constexpr bool is_safe_range(const array_of_iterator_impl& ri) const {
		if constexpr (is_safe == memory_safety::none)
			return true;
		else {			
			if (arr == ri.arr)
				return ix <= ri.ix && ri.ix <= (arr ? arr->capacity() : 0);
			else
				return false;
		}
	}

	template<typename PTR>
	constexpr bool is_safe_range(const PTR& otherArr, size_t otherIx) const {
		if constexpr (is_safe == memory_safety::none)
			return true;
		else {
			if (arr == otherArr)
				return ix <= otherIx && otherIx <= (arr ? arr->capacity() : 0);
			else
				return false;
		}
	}

	bool is_end() const {
		return ix >= (arr ? arr->capacity() : 0);
	}

	iterator_validity validate_iterator(const array_of_iterator_impl&, const array_of_iterator_impl& end) const noexcept {
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
};

template <typename T, bool b, template<typename> typename P>
typename array_of_iterator_impl<T, b, P>::difference_type distance(const array_of_iterator_impl<T, b, P>& l, const array_of_iterator_impl<T, b, P>& r) {
	return r - l;
}


template <typename T, bool bConst>
class array_of_iterator_no_checks
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
	friend class array_of_iterator_no_checks;

private:
	pointer mIterator = nullptr;

public:

	array_of_iterator_no_checks() {}

private:
	explicit array_of_iterator_no_checks(pointer ptr)
		: mIterator(ptr) {}
public:
	template<class PTR>
	static array_of_iterator_no_checks makeIx(const PTR& arr, size_t ix = 0) {
		// non safe function, we know ix is in range
		return array_of_iterator_no_checks(arr->get_raw_ptr(ix));
	}
	
	template<class PTR>
	static array_of_iterator_no_checks makePtr(const PTR&, pointer ptr) {
		return array_of_iterator_no_checks(ptr);
	}

	array_of_iterator_no_checks(const array_of_iterator_no_checks& ri) = default;
	array_of_iterator_no_checks& operator=(const array_of_iterator_no_checks& ri) = default;

	array_of_iterator_no_checks(array_of_iterator_no_checks&& ri) = default;
	array_of_iterator_no_checks& operator=(array_of_iterator_no_checks&& ri) = default;

	// allow non-const to const convertion
	template<bool B, typename X = std::enable_if_t<bConst && !B>>
	array_of_iterator_no_checks(const array_of_iterator_no_checks<T, B>& ri)
		: mIterator(ri.mIterator) {}

	// allow non-const to const convertion
	template<bool B, typename X = std::enable_if_t<bConst && !B>>
	array_of_iterator_no_checks& operator=(const array_of_iterator_no_checks<T, B>& ri) {
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

	array_of_iterator_no_checks& operator++() noexcept
		{ ++mIterator; return *this; }

	array_of_iterator_no_checks operator++(int) noexcept
	{
		array_of_iterator_no_checks ri(*this);
		++mIterator;
		return ri;
	}

	array_of_iterator_no_checks& operator--() noexcept
		{ --mIterator; return *this; }

	array_of_iterator_no_checks operator--(int) noexcept
	{
		array_of_iterator_no_checks ri(*this);
		--mIterator;
		return ri;
	}

	array_of_iterator_no_checks operator+(difference_type n) const noexcept
		{ return array_of_iterator_no_checks(mIterator + n); }

	array_of_iterator_no_checks& operator+=(difference_type n) noexcept
		{ mIterator += n; return *this; }

	array_of_iterator_no_checks operator-(difference_type n) const noexcept
		{ return array_of_iterator_no_checks(mIterator - n); }

	difference_type operator-(const array_of_iterator_no_checks& other) const noexcept
		{ return mIterator - other.mIterator; }

	array_of_iterator_no_checks& operator-=(difference_type n) noexcept
		{ mIterator -= n; return *this; }

	reference operator[](difference_type n) const noexcept
		{ return mIterator[n]; }

	bool operator==(const array_of_iterator_no_checks& ri) const noexcept
		{ return mIterator == ri.mIterator; }

	bool operator!=(const array_of_iterator_no_checks& ri) const noexcept
		{ return !operator==(ri); }

	bool operator<(const array_of_iterator_no_checks& ri) const noexcept {
		return mIterator < ri.mIterator;
	}

	bool operator>(const array_of_iterator_no_checks& ri) const noexcept {
		return mIterator > ri.mIterator;
	}

	bool operator<=(const array_of_iterator_no_checks& ri) const noexcept {
		return !operator>(ri);
	}

	bool operator>=(const array_of_iterator_no_checks& ri) const noexcept {
		return !operator<(ri);
	}

	constexpr bool is_safe_range(const array_of_iterator_no_checks& ri) const noexcept {
		return true;
	}

	template<typename PTR>
	constexpr bool is_safe_range(const PTR& otherArr, size_t otherIx) const noexcept {
		return true;
	}


	iterator_validity validate_iterator(const array_of_iterator_no_checks& b, const array_of_iterator_no_checks& end) const noexcept {
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
};

template <typename T, bool C>
typename array_of_iterator_no_checks<T, C>::difference_type distance(const array_of_iterator_no_checks<T, C>& l, const array_of_iterator_no_checks<T, C>& r) {
		return r - l;
}


} // namespace safe_memory::detail 

namespace eastl {

	// template <typename T>
	// inline void destruct(const typename safe_memory::detail::allocator_to_eastl_no_checks<T>::soft_array_type& first, T* last)
	// {
	// 	eastl::destruct(first.get_raw_ptr(), last);
	// }

	// template <typename T>
	// inline void destruct(const typename safe_memory::detail::allocator_to_eastl_impl<T>::soft_array_type& first, T* last)
	// {
	// 	eastl::destruct(first.get_raw_ptr(), last);
	// }

}

#endif // SAFE_MEMORY_DETAIL_ARRAY_OF

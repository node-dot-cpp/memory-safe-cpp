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

// helper class for arrays, while this class has the concept of an array,
// it doesn't actually have the memory of the array.
// it is assumed that the allocator will give enought memory after this class
// to actually to put the array elements.
// This class is tightly coupled with 'allocate_array' function
template<class T>
struct array_of
{
	typedef array_of<T> this_type;

	size_t _capacity = 0;
	alignas(T) char _begin;

public:
	array_of(size_t capacity) :_capacity(capacity) {}

	array_of(const array_of&) = delete;
	array_of(array_of&&) = delete;

	array_of& operator=(const array_of&) = delete;
	array_of& operator=(array_of&&) = delete;

	// ~array_of() {}

	//unsafe function, allow returning a non-derefenceable pointer as end()
	T* get_raw_ptr(size_t ix) {
		NODECPP_ASSERT(module_id, nodecpp::assert::AssertLevel::regular, ix <= _capacity);
		return begin() + ix;
	}

	//unsafe function, ptr should have been validated
	size_t get_index(const T* ptr) const {
		NODECPP_ASSERT(module_id, nodecpp::assert::AssertLevel::regular, begin() <= ptr);
		NODECPP_ASSERT(module_id, nodecpp::assert::AssertLevel::regular, static_cast<size_t>(ptr - begin()) <= capacity());
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

// implementation of a fixed size array_of that reserves enought
// memory after array_of to actually place the elements.
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
class soft_ptr_with_zero_offset_impl<array_of<T>>
{
	array_of<T>* ptr = nullptr;

public:

	static constexpr memory_safety is_safe = memory_safety::safe;
 
	soft_ptr_with_zero_offset_impl() {}

	soft_ptr_with_zero_offset_impl( make_zero_offset_t, array_of<T>* raw ) :ptr(raw) {}

	soft_ptr_with_zero_offset_impl( const soft_ptr_with_zero_offset_impl& other ) = default;
	soft_ptr_with_zero_offset_impl& operator=( const soft_ptr_with_zero_offset_impl& other ) = default;

	soft_ptr_with_zero_offset_impl( soft_ptr_with_zero_offset_impl&& other ) = default;
	soft_ptr_with_zero_offset_impl& operator=( soft_ptr_with_zero_offset_impl&& other ) = default;

	soft_ptr_with_zero_offset_impl( std::nullptr_t ) { }
	soft_ptr_with_zero_offset_impl& operator=( std::nullptr_t ){ reset(); return *this; }

	void reset() noexcept { ptr = nullptr; }
	explicit operator bool() const noexcept { return ptr != nullptr; }
	void swap( soft_ptr_with_zero_offset_impl& other ) noexcept {
		array_of<T>* tmp = ptr;
		ptr = other.ptr;
		other.ptr = tmp;
	}

	bool operator == (const soft_ptr_with_zero_offset_impl& other ) const noexcept { return ptr == other.ptr; }
	bool operator != (const soft_ptr_with_zero_offset_impl& other ) const noexcept { return ptr != other.ptr; }

	bool operator == (std::nullptr_t) const noexcept { return ptr == nullptr; }
	bool operator != (std::nullptr_t) const noexcept { return ptr != nullptr; }

	array_of<T>& operator*() const noexcept { return *get_array_of_ptr(); }
	array_of<T>* operator->() const noexcept { return get_array_of_ptr(); }
	array_of<T>* get_array_of_ptr() const noexcept { return ptr; }

	T* operator+(std::ptrdiff_t n) const noexcept { return get_raw_begin() + n; }
	T& operator[](std::size_t n) const noexcept { return get_raw_begin()[n]; }
	T* get_raw_begin() const noexcept { return ptr ? get_array_of_ptr()->begin() : nullptr; }

	// mb: destructor should be trivial to allow use in unions
	// ~soft_ptr_with_zero_offset_impl();
};

template<class T>
std::ptrdiff_t operator-(const T* left, const soft_ptr_with_zero_offset_impl<array_of<T>>& right) {
	return left - right.get_raw_begin();
}

template<class T>
std::ptrdiff_t operator-(const soft_ptr_with_zero_offset_impl<array_of<T>>& left, const T* right) {
	return left.get_raw_begin() - right;
}

template<class T>
std::ptrdiff_t operator==(const T* left, const soft_ptr_with_zero_offset_impl<array_of<T>>& right) {
	return left == right.get_raw_begin();
}

template<class T>
std::ptrdiff_t operator==(const soft_ptr_with_zero_offset_impl<array_of<T>>& left, const T* right) {
	return left.get_raw_begin() == right;
}

template<class T>
std::ptrdiff_t operator!=(const T* left, const soft_ptr_with_zero_offset_impl<array_of<T>>& right) {
	return left != right.get_raw_begin();
}

template<class T>
std::ptrdiff_t operator!=(const soft_ptr_with_zero_offset_impl<array_of<T>>& left, const T* right) {
	return left.get_raw_begin() != right;
}

template<class T>
std::ptrdiff_t operator<(const T* left, const soft_ptr_with_zero_offset_impl<array_of<T>>& right) {
	return left < right.get_raw_begin();
}

template<class T>
std::ptrdiff_t operator<(const soft_ptr_with_zero_offset_impl<array_of<T>>& left, const T* right) {
	return left.get_raw_begin() < right;
}

template<class T>
std::ptrdiff_t operator<=(const T* left, const soft_ptr_with_zero_offset_impl<array_of<T>>& right) {
	return left <= right.get_raw_begin();
}

template<class T>
std::ptrdiff_t operator<=(const soft_ptr_with_zero_offset_impl<array_of<T>>& left, const T* right) {
	return left.get_raw_begin() <= right;
}

template<class T>
std::ptrdiff_t operator>(const T* left, const soft_ptr_with_zero_offset_impl<array_of<T>>& right) {
	return left > right.get_raw_begin();
}

template<class T>
std::ptrdiff_t operator>(const soft_ptr_with_zero_offset_impl<array_of<T>>& left, const T* right) {
	return left.get_raw_begin() > right;
}

template<class T>
std::ptrdiff_t operator>=(const T* left, const soft_ptr_with_zero_offset_impl<array_of<T>>& right) {
	return left >= right.get_raw_begin();
}

template<class T>
std::ptrdiff_t operator>=(const soft_ptr_with_zero_offset_impl<array_of<T>>& left, const T* right) {
	return left.get_raw_begin() >= right;
}

// mb: soft_ptr_with_zero_offset_no_checks is not actually used and may be outdated
template<class T>
class soft_ptr_with_zero_offset_no_checks<array_of<T>> : public soft_ptr_with_zero_offset_base
{
	template<class TT>
	friend class soft_ptr_with_zero_offset_no_checks;

public:

	static constexpr memory_safety is_safe = memory_safety::none;
 
	soft_ptr_with_zero_offset_no_checks() {}
	soft_ptr_with_zero_offset_no_checks( make_zero_offset_t, array_of<T>* raw ) :soft_ptr_with_zero_offset_base(raw) {}

	soft_ptr_with_zero_offset_no_checks( const soft_ptr_with_zero_offset_no_checks& other ) = default;
	soft_ptr_with_zero_offset_no_checks& operator=( const soft_ptr_with_zero_offset_no_checks& other ) = default;

	soft_ptr_with_zero_offset_no_checks( soft_ptr_with_zero_offset_no_checks&& other ) = default;
	soft_ptr_with_zero_offset_no_checks& operator=( soft_ptr_with_zero_offset_no_checks&& other ) = default;

	soft_ptr_with_zero_offset_no_checks( std::nullptr_t ) { }
	soft_ptr_with_zero_offset_no_checks& operator=( std::nullptr_t )
		{ soft_ptr_with_zero_offset_base::reset(); return *this; }

	using soft_ptr_with_zero_offset_base::reset;
	using soft_ptr_with_zero_offset_base::swap;
	using soft_ptr_with_zero_offset_base::operator bool;
	using soft_ptr_with_zero_offset_base::operator==;
	using soft_ptr_with_zero_offset_base::operator!=;

	array_of<T>& operator*() const noexcept { return *get_array_of_ptr(); }
	array_of<T>* operator->() const noexcept { return get_array_of_ptr(); }
	array_of<T>* get_array_of_ptr() const noexcept { return reinterpret_cast<array_of<T>*>(ptr); }
	
	T* operator+(std::ptrdiff_t n) const noexcept { return get_raw_begin() + n; }
	T& operator[](std::size_t n) const noexcept { return get_raw_begin()[n]; }
	T* get_raw_begin() const noexcept { return ptr ? get_array_of_ptr()->begin() : nullptr; }

	// mb: destructor should be trivial to allow use in unions
	// ~soft_ptr_with_zero_offset_no_checks();
};


template<class T>
std::ptrdiff_t operator-(const T* left, const soft_ptr_with_zero_offset_no_checks<array_of<T>>& right) {
	return left - right.get_raw_begin();
}

template<class T>
std::ptrdiff_t operator-(const soft_ptr_with_zero_offset_no_checks<array_of<T>>& left, const T* right) {
	return left.get_raw_begin() - right;
}

template<class T>
std::ptrdiff_t operator==(const T* left, const soft_ptr_with_zero_offset_no_checks<array_of<T>>& right) {
	return left == right.get_raw_begin();
}

template<class T>
std::ptrdiff_t operator==(const soft_ptr_with_zero_offset_no_checks<array_of<T>>& left, const T* right) {
	return left.get_raw_begin() == right;
}

template<class T>
std::ptrdiff_t operator!=(const T* left, const soft_ptr_with_zero_offset_no_checks<array_of<T>>& right) {
	return left != right.get_raw_begin();
}

template<class T>
std::ptrdiff_t operator!=(const soft_ptr_with_zero_offset_no_checks<array_of<T>>& left, const T* right) {
	return left.get_raw_begin() != right;
}

template<class T>
std::ptrdiff_t operator<(const T* left, const soft_ptr_with_zero_offset_no_checks<array_of<T>>& right) {
	return left < right.get_raw_begin();
}

template<class T>
std::ptrdiff_t operator<(const soft_ptr_with_zero_offset_no_checks<array_of<T>>& left, const T* right) {
	return left.get_raw_begin() < right;
}

template<class T>
std::ptrdiff_t operator<=(const T* left, const soft_ptr_with_zero_offset_no_checks<array_of<T>>& right) {
	return left <= right.get_raw_begin();
}

template<class T>
std::ptrdiff_t operator<=(const soft_ptr_with_zero_offset_no_checks<array_of<T>>& left, const T* right) {
	return left.get_raw_begin() <= right;
}

template<class T>
std::ptrdiff_t operator>(const T* left, const soft_ptr_with_zero_offset_no_checks<array_of<T>>& right) {
	return left > right.get_raw_begin();
}

template<class T>
std::ptrdiff_t operator>(const soft_ptr_with_zero_offset_no_checks<array_of<T>>& left, const T* right) {
	return left.get_raw_begin() > right;
}

template<class T>
std::ptrdiff_t operator>=(const T* left, const soft_ptr_with_zero_offset_no_checks<array_of<T>>& right) {
	return left >= right.get_raw_begin();
}

template<class T>
std::ptrdiff_t operator>=(const soft_ptr_with_zero_offset_no_checks<array_of<T>>& left, const T* right) {
	return left.get_raw_begin() >= right;
}


// small helper class to set safety to none when using raw pointers
template <typename T>
struct safety_helper {
	static constexpr memory_safety is_safe = T::is_safe;
};

template<typename T>
struct safety_helper<T*> {
	static constexpr memory_safety is_safe = memory_safety::none;
};

// when this iterator is used for string, we should be able to construct it
// from a heap pointer or from stack pointer (when SSO)
template <typename T, bool bConst, typename ArrPtr>
class array_of_iterator
{
protected:
	typedef array_of_iterator<T, bConst, ArrPtr>	this_type;
	typedef ArrPtr									array_pointer;
	static constexpr bool is_raw_pointer = std::is_pointer<array_pointer>::value;

	// for non-const to const conversion
	template<typename, bool, typename>
	friend class array_of_iterator;

public:
	typedef std::random_access_iterator_tag  			iterator_category;
	typedef std::conditional_t<bConst, const T, T>		value_type;
	typedef ptrdiff_t			                    	difference_type;
	typedef value_type*									pointer;
	typedef value_type&									reference;

	static constexpr memory_safety is_safe = safety_helper<array_pointer>::is_safe;

protected:
	array_pointer  arr = nullptr;
	size_t ix = 0;
	size_t sz = 0;

	// this ctor is private because of unsafety
	array_of_iterator(array_pointer arr, size_t ix, size_t sz)
		: arr(arr), ix(ix), sz(sz) {}

	[[noreturn]] static void throwRangeException(const char* msg) { throw std::out_of_range(msg); }

public:
	// default ctor must always be available for iterators
	array_of_iterator() {}

	// static factory methods are unsafe and flaged by static checker tool
	static this_type makeIx(array_pointer arr, size_t ix, size_t sz) {
		return this_type(arr, ix, sz);
	}

	static size_t getIndex(array_pointer arr, pointer to) {
		if constexpr (is_raw_pointer) {
			return static_cast<size_t>(to - arr);
		}
		else {
			return arr ? arr->get_index(to) : 0;
		}
	}

	static this_type makePtr(array_pointer arr, pointer to, size_t sz) {
		if constexpr (!is_raw_pointer) {
			NODECPP_ASSERT(module_id, nodecpp::assert::AssertLevel::regular, arr ? arr->capacity() == sz : true);
		}

		return this_type(arr, getIndex(arr, to), sz);
	}

	// mb: at basic_string, iterable size is one less that actual array capacity
	// because of ending null '\0'
	
	static this_type makeIxForString(array_pointer arr, size_t ix, size_t sz) {
		if constexpr (!is_raw_pointer) {
			NODECPP_ASSERT(module_id, nodecpp::assert::AssertLevel::regular, arr ? arr->capacity() == sz + 1 : true);
		}

		return this_type(arr, ix, sz);
	}

	array_of_iterator(const array_of_iterator& ri) = default;
	array_of_iterator& operator=(const array_of_iterator& ri) = default;

	array_of_iterator(array_of_iterator&& ri) = default; 
	array_of_iterator& operator=(array_of_iterator&& ri) = default;

	// allow non-const to const convertion
	template<bool B, typename X = std::enable_if_t<bConst && !B>>
	array_of_iterator(const array_of_iterator<T, B, ArrPtr>& ri)
		: arr(ri.arr), ix(ri.ix), sz(ri.sz) {}

	// allow non-const to const convertion
	template<bool B, typename X = std::enable_if_t<bConst && !B>>
	array_of_iterator& operator=(const array_of_iterator<T, B, ArrPtr>& ri) {
		this->arr = ri.arr;
		this->ix = ri.ix;
		this->sz = ri.sz;
		return *this;
	}

	reference operator*() const {
		if(NODECPP_LIKELY(arr && ix < sz)) {
			if constexpr(is_raw_pointer)
				return arr[ix];
			else
				return *(arr->get_raw_ptr(ix));
		}
		else
			throwRangeException("array_of_iterator::operator*");
	}

	pointer operator->() const {
		if(NODECPP_LIKELY(arr && ix < sz)) {
			if constexpr (is_raw_pointer)
				return arr + ix;
			else
				return arr->get_raw_ptr(ix);
		}
		else
			throwRangeException("array_of_iterator::operator->");
	}

	//mb: when we try to increment/decrement iterator outside its boundaries,
	// it stays in a range [0, end] and doesn't throw. At dereference
	// we throw if iterator is at end

	this_type& operator++() noexcept {
		if(ix < sz)
			++ix;
		return *this;
	}

	this_type operator++(int) noexcept {
		this_type ri(*this);
		operator++();
		return ri;
	}

	this_type& operator--() noexcept {
		if(0 < ix)
			--ix;
		return *this;
	}

	this_type operator--(int) noexcept {
		this_type ri(*this);
		operator--();
		return ri;
	}

	this_type operator+(difference_type n) const noexcept {
		return this_type(arr, std::min(ix + n, sz), sz);
	}

	this_type& operator+=(difference_type n) noexcept {
		ix = std::min(ix + n, sz);
		return *this;
	}

	this_type operator-(difference_type n) const noexcept {
		return this_type(arr, std::min(ix - n, sz), sz);
	}

	this_type& operator-=(difference_type n) noexcept {
		ix = std::min(ix - n, sz);
		return *this;
	}

	difference_type operator-(const this_type& ri) const noexcept {
		if(NODECPP_LIKELY(arr == ri.arr))
			return ix - ri.ix;
		else
			throwRangeException("array_of_iterator::operator-");
	}


	reference operator[](difference_type n) const {
		size_t tmp = ix + n;
		if(NODECPP_LIKELY(arr && tmp < sz)) {
			if constexpr (is_raw_pointer)
				return arr[tmp];
			else
				return *(arr->get_raw_ptr(tmp));
		}
		else
			throwRangeException("array_of_iterator::operator[]");
	}

	bool operator==(const this_type& ri) const noexcept {

		if(NODECPP_LIKELY(arr == ri.arr))
			return ix == ri.ix;
		else if(!arr || !ri.arr)
			return false;
		else
			throwRangeException("array_of_iterator::operator==");
	}

	bool operator!=(const this_type& ri) const noexcept {
		return !operator==(ri);
	}

	bool operator<(const this_type& ri) const noexcept {

		if(NODECPP_LIKELY(arr == ri.arr))
			return ix < ri.ix;
		else
			throwRangeException("array_of_iterator::operator<");
	}

	bool operator>(const this_type& ri) const noexcept {
		return ri.operator<(*this);
	}

	bool operator<=(const this_type& ri) const noexcept {
		return !this->operator>(ri);
	}

	bool operator>=(const this_type& ri) const noexcept {
		return !this->operator<(ri);
	}

	// mb: about toRaw: 

	// on vector arr will be null for default constructed iterator (invalid)
	// and for end() iterator of empty vector (valid)
	// eastl::vector will correctly handle a nullptr returning from 'toRaw' 

	// on string arr is null only on default constructed iterator (invalid),
	// because even empty strings have a dereferenceable '\0' char on eastl.
	// so passing a nullptr to eastl::string will probably break things.
	// however 'begin' argument will never be null in such case.



	// this is unsafe function, ix may be end, and not derefenceable
	pointer getRaw() const {
		if constexpr (is_raw_pointer)
			return arr + ix;
		else
			return arr ? arr->get_raw_ptr(ix) : nullptr;
	}

	pointer getRawBegin() const {
		if constexpr (is_raw_pointer)
			return arr;
		else
			return arr ? arr->begin() : nullptr;
	}

	// convert a single iterator to raw
	pointer toRaw(const T* begin) const {
		if (getRawBegin() == begin)
			return getRaw();
		else
			throwRangeException("array_of_iterator::toRaw");
	}

	// convert a iterator pair to raw
	std::pair<pointer, pointer> toRaw(const T* begin, const this_type& ri) const {
		if (getRawBegin() == begin && arr == ri.arr && ix <= ri.ix)
			return {getRaw(), ri.getRaw()};
		else
			throwRangeException("array_of_iterator::toRaw");
	}

	// convert an external iterator pair to raw
	std::pair<pointer, pointer> toRawOther(const this_type& ri) const {
		if (arr == ri.arr && ix <= ri.ix)
			return {getRaw(), ri.getRaw()};
		else
			throwRangeException("array_of_iterator::toRaw");
	}
};

template <typename T, bool b, typename ArrPtr>
typename array_of_iterator<T, b, ArrPtr>::difference_type distance(const array_of_iterator<T, b, ArrPtr>& l, const array_of_iterator<T, b, ArrPtr>& r) {
	return r - l;
}


template <typename T>
using array_of_iterator_stack = array_of_iterator<T, false, T*>;

template <typename T>
using const_array_of_iterator_stack = array_of_iterator<T, true, T*>;

} // namespace safe_memory::detail 

#endif // SAFE_MEMORY_DETAIL_ARRAY_OF

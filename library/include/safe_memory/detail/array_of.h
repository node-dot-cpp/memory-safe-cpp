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
// #include <safe_memory/detail/soft_ptr_with_zero_offset.h>


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
// using nodecpp::safememory::soft_ptr_impl;

/** 
 * \brief Helper class for allocation of arrays.
 * 
 * While this class has the concept of an array or buffer,
 * it doesn't actually have the memory of the array.
 * Nor it will construct or destruct any of its elements.
 * It is assumed that the allocator will give enought memory right after this class
 * to actually to put the array elements.
 * This class is coupled with \c allocate_array function.
 */ 
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

	///unsafe function, allow returning a non-derefenceable pointer as end()
	T* get_raw_ptr(size_t ix) {
		NODECPP_ASSERT(module_id, nodecpp::assert::AssertLevel::regular, ix <= _capacity);
		return begin() + ix;
	}

	///unsafe function, ptr should have been validated
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


/**
 * 
 * Implementation of \c array_of that reserves enought
 * memory after the array header to a actually place elements.
 * Can be used on the stack or embedded in other ojects.
 * It still won't construct or destruct any of their elements.
 */
template<size_t SZ, class T>
struct fixed_array_of : public array_of<T>
{
	char buff[sizeof(T) * SZ];

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




/// small helper class to set safety to none when using raw pointers
template <typename T>
struct safety_helper {
	static constexpr memory_safety is_safe = T::is_safe;
};

template<typename T>
struct safety_helper<T*> {
	static constexpr memory_safety is_safe = memory_safety::none;
};

/**
 * \brief Safe and generic iterator for arrays.
 * 
 * This iterator is the safety replacement of \c T* for arrays.
 * It knows the size of the array being iterated, and will check dereferences are
 * always in bounds of the array.
 * It will warranty no memory outside the array will be referenced.
 * Heap memory safety is optional and depends on the \p ArrPtr parameter type.
 * 
 * Current implementation uses a \c begin pointer and two indexes, but implementation with
 * Three pointers is also posible.
 * 
 * Also at current implementation, when we try to increment/decrement iterator
 * outside its boundaries, it stays in a range [0, end] and doesn't throw.
 * At dereference we throw if iterator is at end.
 * 
 * 
 * This class is currently used in two different scenarios.
 * When used as \a stack_only iterator, the \c ArrPtr parameter is usually a \c T*
 * and static checker must enforce lifetime rules similar to those of raw pointers.
 * When used as \a heap_safe iterator, the \c ArrPtr parameter is usually a \c soft_ptr
 * and because of that this class is also safe to store at the heap.
 * 
 * On \c safe_memory::string because of SSO, we sometimes point to an array on the stack
 * and other times at an array on the heap. For \a heap_safe iterators, implementation will
 * always move the buffer to the heap before creating the iterator.
 * The default constructed iterator is different from an iterator to an empty string,
 * as the \c eastl::basic_string implementation has a buffer with a \c '\0' character when empty.
 * Only a default contructed iterator will have \p ArrPtr as a \c nullptr .
 * 
 * On \c safe_memory::vector always point to heap array, and iterator to an empty container
 * is the same that default constructed iterator. \p ArrPtr is \c nullptr in both cases.
 * 
 */

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

	/// this ctor is private because it is unsafe and shouldn't be reached by user
	array_of_iterator(array_pointer arr, size_t ix, size_t sz)
		: arr(arr), ix(ix), sz(sz) {}

	[[noreturn]] static void throwRangeException(const char* msg) { throw std::out_of_range(msg); }

public:
	/// default ctor must always be available for iterators
	array_of_iterator() {}

	/// static factory methods are unsafe but static checker tool will keep user hands away
	static this_type makeIx(array_pointer arr, size_t ix, size_t sz) {
		if constexpr (!is_raw_pointer) {
			NODECPP_ASSERT(module_id, nodecpp::assert::AssertLevel::regular, arr ? arr->capacity() == sz : true);
		}

		return this_type(arr, ix, sz);
	}

	/// At basic_string, iterable size is one less than array capacity because of ending null '\0'
	static this_type makeIxForString(array_pointer arr, size_t ix, size_t sz) {
		if constexpr (!is_raw_pointer) {
			NODECPP_ASSERT(module_id, nodecpp::assert::AssertLevel::regular, arr ? arr->capacity() == sz + 1 : true);
		}

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

		return makeIx(arr, getIndex(arr, to), sz);
	}

	static this_type makePtrForString(array_pointer arr, pointer to, size_t sz) {

		return makeIxForString(arr, getIndex(arr, to), sz);
	}

	array_of_iterator(const array_of_iterator& ri) = default;
	array_of_iterator& operator=(const array_of_iterator& ri) = default;

	array_of_iterator(array_of_iterator&& ri) = default; 
	array_of_iterator& operator=(array_of_iterator&& ri) = default;

	/// allow non-const to const constructor
	template<bool B, typename X = std::enable_if_t<bConst && !B>>
	array_of_iterator(const array_of_iterator<T, B, ArrPtr>& ri)
		: arr(ri.arr), ix(ri.ix), sz(ri.sz) {}

	/// allow non-const to const assignment
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

	/**
	 * \brief Convert this iterator to raw pointer.
	 * 
	 * This methods will check that this iterator references the same array pointed by \c begin
	 * and will be called on all iterators comming from user side, before going into \c eastl
	 * implementation side. Because of that this method has the responsibility of returning
	 * a value that \c eastl will handle correctly and won't break things.
	 * Even in cases where the user handles us with badly stuff. 
	 * 
	 * Notes:
	 * On \c vector arr will be null for default constructed iterator (invalid)
	 * and for end() iterator of empty vector (valid), and
	 * \c eastl::vector will correctly handle a nullptr returning from 'toRaw'
	 * 
	 * On \c string arr is null only on default constructed iterator (invalid),
	 * because even empty strings have a dereferenceable \c '\0' char on eastl.
	 * so passing a nullptr to \c eastl::string may break things.
	 * However \c begin argument won't be null in such case.
	 */
	pointer toRaw(const T* begin) const {
		if (getRawBegin() == begin)
			return getRaw();
		else
			throwRangeException("array_of_iterator::toRaw");
	}

	/**
	 * \brief convert a iterator pair to raw pointers.
	 * 
	 * Same as \c toRaw above, but for a pair of iterators.
	 * 
	 * We check that \c this and \p ri both point to the same array than \p begin
	 * and that the order is correct
	 */
	std::pair<pointer, pointer> toRaw(const T* begin, const this_type& ri) const {
		if (getRawBegin() == begin && arr == ri.arr && ix <= ri.ix)
			return {getRaw(), ri.getRaw()};
		else
			throwRangeException("array_of_iterator::toRaw");
	}

	/**
	 * \brief convert a external iterator pair to raw pointers.
	 * 
	 * Similar to \c toRaw above, but used when iterator pair if from another instance.
	 * 
	 * In this case we only need to check that both, \c this and \p ri are iterators
	 * to the same array and that the order is correct.
	 */
	std::pair<pointer, pointer> toRawOther(const this_type& ri) const {
		if (arr == ri.arr && ix <= ri.ix)
			return {getRaw(), ri.getRaw()};
		else
			throwRangeException("array_of_iterator::toRaw");
	}

private:
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

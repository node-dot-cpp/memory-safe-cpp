/* -------------------------------------------------------------------------------
* Copyright (c) 2021, OLogN Technologies AG
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

#ifndef SAFE_MEMORY_DETAIL_ARRAY_ITERATOR_H
#define SAFE_MEMORY_DETAIL_ARRAY_ITERATOR_H

#include <utility> //for std::pair
#include <iterator> //for std::random_access_iterator_tag
#include <cstddef>
#include <safememory/memory_safety.h>
#include <nodecpp_assert.h>

namespace safememory::detail {

/**
 * \brief Safe and generic iterator for arrays.
 * 
 * This iterator is the safety replacement of \c T* for arrays.
 * It knows the size of the array being iterated, and will check dereferences are
 * always in bounds of the array.
 * It will warranty no memory outside the array will be referenced.
 * Because of \p ArrPtr parameter it can be used as iterator of \c safememory::array
 * and also to iterate any array buffer on the stack, or allocated on the heap.  
 * 
 * This class can be used as \a stack_only or as \a heap_safe.
 * 
 * When used as \a stack_only iterator, it can point to array or buffer on the stack or
 * on the heap, static checker must enforce lifetime rules similar to those of raw pointers.
 * To make things easier for checker we provide a derived class \c array_stack_only_iterator
 * with exactly the same behaviour as this.
 * 
 * When used as \a heap_safe iterator, it must point to an array on the heap and the class itself 
 * is also safe to store at the heap. This usually will use a \c soft_ptr as \p ArrPtr.
 * 
 * Current implementation uses a \c begin pointer and two indexes, but implementation with
 * Three pointers is also posible.
 * 
 * Also at current implementation, we throw if iterator is at end or outside range.
 * 
 * 
 * On \c safememory::string because of SSO, we sometimes point to an array on the stack
 * and other times at an array on the heap. For \a heap_safe iterators, implementation will
 * always move the buffer to the heap before creating the iterator.
 * The default constructed iterator is different from an iterator to an empty string,
 * as the \c eastl::basic_string implementation has a buffer with a \c '\0' character when empty.
 * Only a default contructed iterator will have \p ArrPtr as a \c nullptr .
 * 
 * On \c safememory::vector always point to heap array, and iterator to an empty container
 * is the same that default constructed iterator. \p ArrPtr is \c nullptr in both cases.
 * 
 * On \c safememory::string_literal the \p ArrPtr is a \c T* but the iterator is safe to store
 * on the heap anyway, because the pointed string literal has an infinite lifetime.
 * 
 * On \c safememory::array the data is hold inside the array body so \a heap_safe iterators
 * can only be created when the array itself is on the heap, an a \c soft_this_ptr2 is used
 * to create the \c soft_ptr used as \p ArrPtr
 * 
 */

template <typename T, bool is_const, typename ArrPtr>
class array_heap_safe_iterator
{
protected:
	typedef array_heap_safe_iterator<T, is_const, ArrPtr>			this_type;
	typedef array_heap_safe_iterator<T, false, ArrPtr>			this_type_non_const;

	typedef ArrPtr									            array_pointer;
	static constexpr bool is_raw_pointer = std::is_pointer<array_pointer>::value;

	// for non-const to const conversion
	template<typename, bool, typename>
	friend class array_heap_safe_iterator;

	template<typename TT>
	static constexpr bool sfinae = is_const && std::is_same_v<TT, this_type_non_const>;

	// default to 'safe', only on soft_ptr_no_checks we can relax to 'none'
	template <typename>
	struct array_heap_safe_iterator_safety_helper {
		static constexpr memory_safety is_safe = memory_safety::safe;
	};

	template<typename> class soft_ptr_no_checks; //fwd
	template<typename TT>
	struct array_heap_safe_iterator_safety_helper<soft_ptr_no_checks<TT>> {
		static constexpr memory_safety is_safe = memory_safety::none;
	};


public:
	typedef std::random_access_iterator_tag  			iterator_category;
	typedef std::conditional_t<is_const, const T, T>		value_type;
	typedef std::ptrdiff_t                              difference_type;
	typedef std::size_t                                 size_type;
	typedef value_type*									pointer;
	typedef value_type&									reference;

	static constexpr memory_safety is_safe = array_heap_safe_iterator_safety_helper<array_pointer>::is_safe;

protected:
	array_pointer  arr = nullptr;
	size_type ix = 0;
	size_type sz = 0;

	/// this ctor is private because it is unsafe and shouldn't be reached by user
	constexpr array_heap_safe_iterator(array_pointer arr, size_type ix, size_type sz)
		: arr(arr), ix(ix), sz(sz) {}

	[[noreturn]] static void throwRangeException(const char* msg) { throw std::out_of_range(msg); }
	[[noreturn]] static void ThrowInvalidArgumentException(const char* msg) { throw std::invalid_argument(msg); }

	static constexpr void extraSanityCheck(array_pointer arr, size_type ix, size_type sz) {

		if constexpr (false) {
			// ix must be equal or lower than sz
			NODECPP_ASSERT(module_id, nodecpp::assert::AssertLevel::regular, ix <= sz);

			if(!arr) {
				// if arr is null, then ix and sz must be zero
				NODECPP_ASSERT(module_id, nodecpp::assert::AssertLevel::regular, ix == 0);
				NODECPP_ASSERT(module_id, nodecpp::assert::AssertLevel::regular, sz == 0);
			}
			else if constexpr (!is_raw_pointer) {
				// allow to create an iterator that reaches only part of the array
				NODECPP_ASSERT(module_id, nodecpp::assert::AssertLevel::regular, sz <= arr->size());
			}
		}
	}

public:
	/// default ctor must always be available for iterators
	constexpr array_heap_safe_iterator() {}

	/// static factory methods are unsafe but static checker tool will keep user hands away
	static constexpr this_type makeIx(array_pointer arr, size_type ix, size_type sz) {

		extraSanityCheck(arr, ix, sz);
		return this_type(arr, ix, sz);
	}

	static constexpr size_type getIndex(array_pointer arr, pointer to) {
		if constexpr (is_raw_pointer)
			return static_cast<size_type>(to - arr);
		else 
			return arr ? static_cast<size_type>(to - arr->data()) : 0;
	}

	static constexpr this_type makePtr(array_pointer arr, pointer to, size_type sz) {

		return makeIx(arr, getIndex(arr, to), sz);
	}

	array_heap_safe_iterator(const array_heap_safe_iterator& ri) = default;
	array_heap_safe_iterator& operator=(const array_heap_safe_iterator& ri) = default;

	array_heap_safe_iterator(array_heap_safe_iterator&& ri) = default; 
	array_heap_safe_iterator& operator=(array_heap_safe_iterator&& ri) = default;

	/// allow non-const to const constructor
	template<typename Other, std::enable_if_t<sfinae<Other>, bool> = true>
	array_heap_safe_iterator(const Other& ri)
		: arr(ri.arr), ix(ri.ix), sz(ri.sz) {}

	/// allow non-const to const assignment
	template<typename Other, std::enable_if_t<sfinae<Other>, bool> = true>
	array_heap_safe_iterator& operator=(const Other& ri) {
		this->arr = ri.arr;
		this->ix = ri.ix;
		this->sz = ri.sz;
		return *this;
	}

	reference operator*() const {
		if constexpr (is_safe == memory_safety::safe) {
			if(NODECPP_UNLIKELY(!arr || !(ix < sz)))
				throwRangeException("array_heap_safe_iterator::operator*");
		}

		if constexpr(is_raw_pointer)
			return *(arr + ix);
		else
			return *(arr->data() + ix);
	}

	pointer operator->() const {
		if constexpr (is_safe == memory_safety::safe) {
			if(NODECPP_UNLIKELY(!arr || !(ix < sz)))
				throwRangeException("array_heap_safe_iterator::operator->");
		}

		if constexpr (is_raw_pointer)
			return arr + ix;
		else
			return arr->data() + ix;
	}

	this_type& operator++() noexcept { ++ix; return *this; }
	this_type& operator--() noexcept { --ix; return *this; }

	this_type operator++(int) noexcept { this_type ri(*this); ++ix; return ri; }
	this_type operator--(int) noexcept { this_type ri(*this); --ix; return ri; }

	this_type operator+(difference_type n) const noexcept {	return this_type(arr, ix + n, sz); }
	this_type operator-(difference_type n) const noexcept {	return this_type(arr, ix - n, sz); }

	this_type& operator+=(difference_type n) noexcept {	ix += n; return *this; }
	this_type& operator-=(difference_type n) noexcept { ix -= n; return *this; }

	constexpr reference operator[](difference_type n) const {
		
		size_type tmp = ix + n;
		if constexpr (is_safe == memory_safety::safe) {
			if(NODECPP_UNLIKELY(!arr || !(tmp < sz)))
				throwRangeException("array_heap_safe_iterator::operator[]");
		}

		if constexpr (is_raw_pointer)
			return *(arr + tmp);
		else
			return *(arr->data() + tmp);
	}

	difference_type operator-(const this_type& ri) const noexcept {

		// it1 == it2 => it1 - it2 == 0, even for null iterators
		if(arr == ri.arr)
			return ix - ri.ix;
		else
			ThrowInvalidArgumentException("array_heap_safe_iterator::operator-");
	}

	bool operator==(const this_type& ri) const noexcept {

		if(arr == ri.arr)
			return ix == ri.ix;
		else if(!arr || !ri.arr)
			return false;
		else
			ThrowInvalidArgumentException("array_heap_safe_iterator::operator==");
	}

	bool operator!=(const this_type& ri) const noexcept {
		return !operator==(ri);
	}

	bool operator<(const this_type& ri) const noexcept {

		if(arr == ri.arr)
			return ix < ri.ix;
		else if(!arr)
			return true;
		else if(!ri.arr)
			return false;
		else
			ThrowInvalidArgumentException("array_heap_safe_iterator::operator<");
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
			ThrowInvalidArgumentException("array_heap_safe_iterator::toRaw");
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
			ThrowInvalidArgumentException("array_heap_safe_iterator::toRaw");
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
			ThrowInvalidArgumentException("array_heap_safe_iterator::toRaw");
	}

	pointer getRaw() const {
		if constexpr (is_raw_pointer)
			return arr + ix; // arr may be null
		else
			return arr ? arr->data() + ix : nullptr;
	}

	pointer getRawBegin() const {
		if constexpr (is_raw_pointer)
			return arr; // arr may be null
		else
			return arr ? arr->data() : nullptr;
	}

};

/**
 * \brief Safe and generic iterator for arrays.
 * 
 * This iterator is exactly identical to \c array_heap_safe_iterator but checker
 * will enforce a different set of rules for it.
 * 
 * Library will use this iterator when scope rules must be enforced on iterator
 * 
 */
template <typename T, bool is_const, typename ArrPtr>
class array_stack_only_iterator :protected array_heap_safe_iterator<T, is_const, ArrPtr>
{
protected:
	typedef array_stack_only_iterator<T, is_const, ArrPtr>	 this_type;
	typedef array_stack_only_iterator<T, false, ArrPtr>	     this_type_non_const;
	typedef array_heap_safe_iterator<T, is_const, ArrPtr>    base_type;
	typedef ArrPtr									         array_pointer;

	// for non-const to const conversion
	template<typename, bool, typename>
	friend class array_stack_only_iterator;

	template<typename TT>
	static constexpr bool sfinae = is_const && std::is_same_v<TT, this_type_non_const>;


public:
	typedef typename base_type::iterator_category  iterator_category;
	typedef typename base_type::value_type         value_type;
	typedef typename base_type::difference_type    difference_type;
	typedef typename base_type::size_type          size_type;
	typedef typename base_type::pointer            pointer;
	typedef typename base_type::reference          reference;

protected:
	/// this ctor is private because it is unsafe and shouldn't be reached by user
	constexpr array_stack_only_iterator(array_pointer arr, size_type ix, size_type sz)
		: base_type(arr, ix, sz) {}
	/// this ctor is private because it is unsafe and shouldn't be reached by user
	constexpr array_stack_only_iterator(const base_type& ri)
		: base_type(ri) {}
	constexpr array_stack_only_iterator(base_type&& ri)
		: base_type(std::move(ri)) {}
public:
	/// default ctor must always be available for iterators
	constexpr array_stack_only_iterator() :base_type() {}

	/// static factory methods are unsafe but static checker tool will keep user hands away
	static constexpr this_type makeIx(array_pointer arr, size_type ix, size_type sz) {

		base_type::extraSanityCheck(arr, ix, sz);
		return this_type(arr, ix, sz);
	}

	static constexpr this_type makePtr(array_pointer arr, pointer to, size_type sz) {

		return makeIx(arr, base_type::getIndex(arr, to), sz);
	}

	array_stack_only_iterator(const array_stack_only_iterator& ri) = default;
	array_stack_only_iterator& operator=(const array_stack_only_iterator& ri) = default;

	array_stack_only_iterator(array_stack_only_iterator&& ri) = default; 
	array_stack_only_iterator& operator=(array_stack_only_iterator&& ri) = default;

	/// allow non-const to const constructor
	template<typename Other, std::enable_if_t<sfinae<Other>, bool> = true>
	array_stack_only_iterator(const Other& ri)
		: base_type(static_cast<const typename base_type::this_type_non_const&>(ri)) {}

	/// allow non-const to const assignment
	template<typename Other, std::enable_if_t<sfinae<Other>, bool> = true>
	array_stack_only_iterator& operator=(const Other& ri) {
		base_type::operator=(static_cast<const typename base_type::this_type_non_const&>(ri));
		return *this;
	}

	using base_type::operator*;
	using base_type::operator->;

	this_type& operator++() noexcept { base_type::operator++(); return *this; }
	this_type& operator--() noexcept { base_type::operator--(); return *this; }

	this_type operator++(int i) noexcept { return base_type::operator++(i); }
	this_type operator--(int i) noexcept { return base_type::operator--(i); }

	this_type operator+(difference_type n) const noexcept {	return base_type::operator+(n); }
	this_type operator-(difference_type n) const noexcept {	return base_type::operator-(n); }

	this_type& operator+=(difference_type n) noexcept { base_type::operator+=(n); return *this; }
	this_type& operator-=(difference_type n) noexcept { base_type::operator-=(n); return *this; }

	using base_type::operator[];

	difference_type operator-(const this_type& ri) const noexcept { return base_type::operator-(ri); }
	bool operator==(const this_type& ri) const noexcept { return base_type::operator==(ri); }
	bool operator!=(const this_type& ri) const noexcept { return base_type::operator!=(ri); }
	bool operator<(const this_type& ri) const noexcept { return base_type::operator<(ri); }
	bool operator>(const this_type& ri) const noexcept { return base_type::operator>(ri); }
	bool operator<=(const this_type& ri) const noexcept { return base_type::operator<=(ri); }
	bool operator>=(const this_type& ri) const noexcept { return base_type::operator>=(ri); }

	pointer toRaw(const T* begin) const { return base_type::toRaw(begin); }
	std::pair<pointer, pointer> toRaw(const T* begin, const this_type& ri) const { return base_type::toRaw(begin, ri); }
	std::pair<pointer, pointer> toRawOther(const this_type& ri) const {	return base_type::toRawOther(ri); }
};


template <typename T, bool is_const, typename ArrPtr>
typename array_heap_safe_iterator<T, is_const, ArrPtr>::difference_type distance(
	const array_heap_safe_iterator<T, is_const, ArrPtr>& l, const array_heap_safe_iterator<T, is_const, ArrPtr>& r) {
	return r - l;
}

template <typename T, bool is_const, typename ArrPtr>
typename array_stack_only_iterator<T, is_const, ArrPtr>::difference_type distance(
	const array_stack_only_iterator<T, is_const, ArrPtr>& l, const array_stack_only_iterator<T, is_const, ArrPtr>& r) {
	return r - l;
}


} // namespace safememory::detail 

#endif // SAFE_MEMORY_DETAIL_ARRAY_ITERATOR_H

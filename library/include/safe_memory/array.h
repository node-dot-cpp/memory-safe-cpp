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

/////////////////////////////////////////////////////////////////////////////
// First version inspired from EASTL-3.17.03
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////


#ifndef SAFE_MEMORY_ARRAY_H
#define SAFE_MEMORY_ARRAY_H

#include <EASTL/internal/config.h>
#include <EASTL/iterator.h>
#include <EASTL/algorithm.h>
#include <EASTL/memory.h>
#include <safe_memory/safe_ptr.h>
#include <safe_memory/detail/array_of.h>
#include <safe_memory/detail/allocator_to_eastl.h>

namespace safe_memory
{

/** 
 * \brief A safe_memory array.
 * 
 * The implementation of \c array for \c safe_memory has a couple of interesting details.
 * 
 * For \c safe_memory we can't use aggregate initialization, and I wasn't able to make a constructor
 * that can initialize the member array in the initializers (not the constructor body)
 * So current implementation uses std::initializer_list in the contructor body, the member array
 * is of char type, to avoid default construct of the elements before constructor body, and
 * default constructor and desctructor is handled manually.
 * But this implementation can't be used in \c constexpr constext.
  */ 

template <typename T, eastl_size_t N = 1, memory_safety Safety = safeness_declarator<T>::is_safe>
struct array
{
public:
	typedef array<T, N, Safety>                           this_type;
	typedef T                                             value_type;
	typedef value_type&                                   reference;
	typedef const value_type&                             const_reference;
	typedef value_type*                                   pointer;
	typedef const value_type*                             const_pointer;
	typedef eastl_size_t                                  size_type;
	typedef ptrdiff_t                                     difference_type;

	typedef soft_ptr<this_type, Safety>                                             soft_ptr_this_type;

	typedef typename detail::array_of_iterator_stack<T>                             stack_only_iterator;
	typedef typename detail::const_array_of_iterator_stack<T>                       const_stack_only_iterator;
	typedef typename detail::array_of_iterator<T, false, soft_ptr_this_type, true>  heap_safe_iterator;
	typedef typename detail::array_of_iterator<T, true, soft_ptr_this_type, true>   const_heap_safe_iterator;

	static constexpr bool use_base_iterator = Safety == memory_safety::none;
	
	typedef std::conditional_t<use_base_iterator, pointer, stack_only_iterator>               iterator;
	typedef std::conditional_t<use_base_iterator, const_pointer, const_stack_only_iterator>   const_iterator;
	typedef eastl::reverse_iterator<iterator>                                                 reverse_iterator;
	typedef eastl::reverse_iterator<const_iterator>                                           const_reverse_iterator;

	typedef heap_safe_iterator                                         iterator_safe;
	typedef const_heap_safe_iterator                                   const_iterator_safe;
	typedef eastl::reverse_iterator<iterator_safe>                     reverse_iterator_safe;
	typedef eastl::reverse_iterator<const_iterator_safe>               const_reverse_iterator_safe;

	//TODO: mb: add support for empty array
	static_assert(N != 0, "Empty safe_memory::array not supported yet!");

public:
	static constexpr memory_safety is_safe = Safety;

	static constexpr size_type count = N;

private:
	soft_this_ptr2<is_safe> this_ptr;
	// use char array as we don't want default construct of elements
	alignas(value_type) char mValue[sizeof(value_type) * N];

public:
	constexpr array() {
		eastl::uninitialized_default_fill(begin_unsafe(), end_unsafe());
	}
	
	constexpr array(std::initializer_list<value_type> init) {
		
		eastl_size_t sz = eastl::min_alt(N, init.size());
		
		auto it = init.begin();
		auto r = eastl::uninitialized_copy_ptr(it, it + sz, begin_unsafe());
		
		// default construct any pending element (when initializer_list is shorter than N)
		eastl::uninitialized_default_fill(r, end_unsafe());
	}

	/* constexpr */
	~array() {
		eastl::destruct(begin_unsafe(), end_unsafe());
	}

	constexpr bool empty() const noexcept { return N == 0; }
	constexpr size_type size() const noexcept { return N; }
	constexpr size_type max_size() const noexcept { return N; }

	/* constexpr */ T* data() noexcept { return reinterpret_cast<T*>(&mValue); }
	/* constexpr */ const T* data() const noexcept { return reinterpret_cast<const T*>(&mValue); }

	constexpr pointer       begin_unsafe() noexcept { return data(); }
	constexpr const_pointer begin_unsafe() const noexcept { return data(); }
	constexpr const_pointer cbegin_unsafe() const noexcept { return data(); }

	constexpr pointer       end_unsafe() noexcept { return data() + size(); }
	constexpr const_pointer end_unsafe() const noexcept { return data() + size(); }
	constexpr const_pointer cend_unsafe() const noexcept { return data() + size(); }

	constexpr iterator       begin() noexcept { return makeIt(begin_unsafe()); }
	constexpr const_iterator begin() const noexcept { return makeIt(begin_unsafe()); }
	constexpr const_iterator cbegin() const noexcept { return makeIt(begin_unsafe()); }

	constexpr iterator       end() noexcept { return makeIt(end_unsafe()); }
	constexpr const_iterator end() const noexcept { return makeIt(end_unsafe()); }
	constexpr const_iterator cend() const noexcept { return makeIt(end_unsafe()); }

	constexpr reverse_iterator       rbegin() noexcept { return reverse_iterator(makeIt(end_unsafe())); }
	constexpr const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(makeIt(end_unsafe())); }
	constexpr const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(makeIt(end_unsafe())); }

	constexpr reverse_iterator       rend() noexcept { return reverse_iterator(makeIt(begin_unsafe())); }
	constexpr const_reverse_iterator rend() const noexcept { return const_reverse_iterator(makeIt(begin_unsafe())); }
	constexpr const_reverse_iterator crend() const noexcept { return const_reverse_iterator(makeIt(begin_unsafe())); }

	constexpr iterator_safe       begin_safe() noexcept { return makeSafeIt(begin_unsafe()); }
	constexpr const_iterator_safe begin_safe() const noexcept { return makeSafeIt(begin_unsafe()); }
	constexpr const_iterator_safe cbegin_safe() const noexcept { return makeSafeIt(begin_unsafe()); }

	constexpr iterator_safe       end_safe() noexcept { return makeSafeIt(end_unsafe()); }
	constexpr const_iterator_safe end_safe() const noexcept { return makeSafeIt(end_unsafe()); }
	constexpr const_iterator_safe cend_safe() const noexcept { return makeSafeIt(end_unsafe()); }

	constexpr reverse_iterator_safe       rbegin_safe() noexcept { return reverse_iterator_safe(makeSafeIt(end_unsafe())); }
	constexpr const_reverse_iterator_safe rbegin_safe() const noexcept { return const_reverse_iterator_safe(makeSafeIt(end_unsafe())); }
	constexpr const_reverse_iterator_safe crbegin_safe() const noexcept { return const_reverse_iterator_safe(makeSafeIt(end_unsafe())); }

	constexpr reverse_iterator_safe       rend_safe() noexcept { return reverse_iterator_safe(makeSafeIt(begin_unsafe())); }
	constexpr const_reverse_iterator_safe rend_safe() const noexcept { return const_reverse_iterator_safe(makeSafeIt(begin_unsafe())); }
	constexpr const_reverse_iterator_safe crend_safe() const noexcept { return const_reverse_iterator_safe(makeSafeIt(begin_unsafe())); }

	void fill(const value_type& value) { 
		eastl::fill_n(begin_unsafe(), size(), value);
	}

	// Unlike the swap function for other containers, array::swap takes linear time,
	// may exit via an exception, and does not cause iterators to become associated with the other container.
	void swap(this_type& x) noexcept(eastl::is_nothrow_swappable<value_type>::value) {
		eastl::swap_ranges(begin_unsafe(), end_unsafe(), x.begin_unsafe());
	}

	constexpr reference       operator[](size_type i) {
		if constexpr(is_safe == memory_safety::safe) {
			if(NODECPP_UNLIKELY(i >= size()))
				ThrowRangeException("array::operator[] -- out of range");
		}

		return data()[i];
	}
	
	constexpr const_reference operator[](size_type i) const {
		if constexpr(is_safe == memory_safety::safe) {
			if(NODECPP_UNLIKELY(i >= size()))
				ThrowRangeException("array::operator[] -- out of range");
		}

		return data()[i];
	}

	constexpr const_reference at(size_type i) const {
		// check regarless of safety
		if(NODECPP_UNLIKELY(i >= size()))
			ThrowRangeException("array::at -- out of range");

		return data()[i];
	}

	constexpr reference       at(size_type i) {
		// check regarless of safety
		if(NODECPP_UNLIKELY(i >= size()))
			ThrowRangeException("array::at -- out of range");

		return data()[i];
	}

	constexpr reference       front() {
		if constexpr(is_safe == memory_safety::safe) {
			if(NODECPP_UNLIKELY(empty()))
				ThrowRangeException("array::front -- empty array");
		}

		return data()[0];
	}

	constexpr const_reference front() const {
		if constexpr(is_safe == memory_safety::safe) {
			if(NODECPP_UNLIKELY(empty()))
				ThrowRangeException("array::front -- empty array");
		}

		return data()[0];
	}

	constexpr reference       back() {
		if constexpr(is_safe == memory_safety::safe) {
			if(NODECPP_UNLIKELY(empty()))
				ThrowRangeException("array::front -- empty array");
		}

		return data()[size() -1];
	}

	constexpr const_reference back() const {
		if constexpr(is_safe == memory_safety::safe) {
			if(NODECPP_UNLIKELY(empty()))
				ThrowRangeException("array::front -- empty array");
		}

		return data()[size() - 1];
	}

	bool validate() const { return true; }
	int  validate_iterator(const_pointer i) const {
		if(i >= begin_unsafe())
		{
			if(i < end_unsafe())
				return (eastl::isf_valid | eastl::isf_current | eastl::isf_can_dereference);

			if(i <= end_unsafe())
				return (eastl::isf_valid | eastl::isf_current);
		}

		return eastl::isf_none;
	}

	int  validate_iterator(const const_stack_only_iterator& i) const { return validate_iterator(toBase(i)); }
	int  validate_iterator(const const_heap_safe_iterator& i) const { return validate_iterator(toBase(i)); }

	iterator_safe make_safe(const iterator& position) { return makeSafeIt(toBase(position)); }
	const_iterator_safe make_safe(const const_iterator& position) const { return makeSafeIt(toBase(position)); }

protected:
	[[noreturn]] static void ThrowRangeException(const char* msg) { throw std::out_of_range(msg); }
	[[noreturn]] static void ThrowInvalidArgumentException(const char* msg) { throw std::invalid_argument(msg); }


	pointer toBase(pointer it) const { return it; }
	const_pointer toBase(const_pointer it) const { return it; }

	pointer toBase(const stack_only_iterator& it) const { return it.toRaw(begin_unsafe()); }
	const_pointer toBase(const const_stack_only_iterator& it) const { return it.toRaw(begin_unsafe()); }

	pointer toBase(const heap_safe_iterator& it) const { return it.toRaw(begin_unsafe()); }
	const_pointer toBase(const const_heap_safe_iterator& it) const { return it.toRaw(begin_unsafe()); }

	constexpr iterator makeIt(pointer it) {
		if constexpr (use_base_iterator)
			return it;
		else
			return iterator::makePtr(data(), it, size());
	}
	constexpr const_iterator makeIt(const_pointer it) const {
		if constexpr (use_base_iterator)
			return it;
		else
			return const_iterator::makePtr(const_cast<T*>(data()), it, size());
	}

	iterator_safe makeSafeIt(pointer it) {
		return iterator_safe::makePtr(this_ptr.getSoftPtr(this), it, size());
	}
	
	const_iterator_safe makeSafeIt(const_pointer it) const {
		return const_iterator_safe::makePtr(this_ptr.getSoftPtr(const_cast<this_type*>(this)), it, size());
	}

}; // class array


template <typename T, eastl_size_t N = 1, memory_safety Safety = safeness_declarator<T>::is_safe>
struct array_safe : public array<T, N, Safety>
{
public:
	typedef array_safe<T, N, Safety>                      this_type;
	typedef array<T, N, Safety>                           base_type;
	typedef typename base_type::value_type                value_type;
	typedef typename base_type::reference                 reference;
	typedef typename base_type::const_reference           const_reference;
	typedef typename base_type::pointer                   pointer;
	typedef typename base_type::const_pointer             const_pointer;
	typedef typename base_type::size_type                 size_type;
	typedef typename base_type::difference_type           difference_type;

	// typedef soft_ptr<this_type, Safety>                                             soft_ptr_this_type;

	// typedef typename detail::array_of_iterator_stack<T>                             stack_only_iterator;
	// typedef typename detail::const_array_of_iterator_stack<T>                       const_stack_only_iterator;
	// typedef typename detail::array_of_iterator<T, false, soft_ptr_this_type, true>  heap_safe_iterator;
	// typedef typename detail::array_of_iterator<T, true, soft_ptr_this_type, true>   const_heap_safe_iterator;

	// static constexpr bool use_base_iterator = Safety == memory_safety::none;
	
	typedef typename base_type::iterator_safe                      iterator;
	typedef typename base_type::const_iterator_safe                const_iterator;
	typedef typename base_type::reverse_iterator_safe              reverse_iterator;
	typedef typename base_type::const_reverse_iterator_safe        const_reverse_iterator;

	typedef typename base_type::iterator_safe                      iterator_safe;
	typedef typename base_type::const_iterator_safe                const_iterator_safe;
	typedef typename base_type::reverse_iterator_safe              reverse_iterator_safe;
	typedef typename base_type::const_reverse_iterator_safe        const_reverse_iterator_safe;

	// static_assert(N != 0, "Empty safe_memory::array not supported yet!");

public:
	using base_type::is_safe;
	using base_type::count;

// private:
// 	soft_this_ptr2<is_safe> this_ptr;
// 	// use char array as we don't want default construct of elements
// 	alignas(value_type) char mValue[sizeof(value_type) * N];

public:
	constexpr array_safe() {}	
	constexpr array_safe(std::initializer_list<value_type> init) : base_type(init) {}
	/* constexpr */
	~array_safe() {}

	using base_type::empty;
	using base_type::size;
	using base_type::max_size;

	constexpr iterator       begin() noexcept { return base_type::begin_safe(); }
	constexpr const_iterator begin() const noexcept { return base_type::begin_safe(); }
	constexpr const_iterator cbegin() const noexcept { return base_type::cbegin_safe(); }

	constexpr iterator       end() noexcept { return base_type::end_safe(); }
	constexpr const_iterator end() const noexcept { return base_type::end_safe(); }
	constexpr const_iterator cend() const noexcept { return base_type::cend_safe(); }

	constexpr reverse_iterator       rbegin() noexcept { return base_type::rbegin_safe(); }
	constexpr const_reverse_iterator rbegin() const noexcept { return base_type::rbegin_safe(); }
	constexpr const_reverse_iterator crbegin() const noexcept { return base_type::crbegin_safe(); }

	constexpr reverse_iterator       rend() noexcept { return base_type::rend_safe(); }
	constexpr const_reverse_iterator rend() const noexcept { return base_type::rend_safe(); }
	constexpr const_reverse_iterator crend() const noexcept { return base_type::crend_safe(); }

	using base_type::begin_safe;
	using base_type::cbegin_safe;
	using base_type::end_safe;
	using base_type::cend_safe;
	using base_type::rbegin_safe;
	using base_type::crbegin_safe;
	using base_type::rend_safe;
	using base_type::crend_safe;

	using base_type::fill;
	using base_type::swap;

	using base_type::operator[];
	using base_type::at;
	using base_type::front;
	using base_type::back;
	using base_type::validate;
	using base_type::validate_iterator;
	
}; // class array_safe


///////////////////////////////////////////////////////////////////////////
// template deduction guides
///////////////////////////////////////////////////////////////////////////
#ifdef __cpp_deduction_guides
	template <class T, class... U> array(T, U...) -> array<T, 1 + sizeof...(U)>;
#endif


///////////////////////////////////////////////////////////////////////
// global operators
///////////////////////////////////////////////////////////////////////

template <typename T, eastl_size_t N, memory_safety S>
constexpr inline bool operator==(const array<T, N, S>& a, const array<T, N, S>& b)
{
	return eastl::equal(a.begin_unsafe(), a.end_unsafe(), b.begin_unsafe());
}


template <typename T, eastl_size_t N, memory_safety S>
constexpr inline bool operator<(const array<T, N, S>& a, const array<T, N, S>& b)
{
	return eastl::lexicographical_compare(a.begin_unsafe(), a.end_unsafe(), b.begin_unsafe(), b.end_unsafe());
}


template <typename T, eastl_size_t N, memory_safety S>
constexpr inline bool operator!=(const array<T, N, S>& a, const array<T, N, S>& b)
{
	return !operator==(a, b);
}


template <typename T, eastl_size_t N, memory_safety S>
constexpr inline bool operator>(const array<T, N, S>& a, const array<T, N, S>& b)
{
	return operator<(b, a);
}


template <typename T, eastl_size_t N, memory_safety S>
constexpr inline bool operator<=(const array<T, N, S>& a, const array<T, N, S>& b)
{
	return !operator<(b, a);
}


template <typename T, eastl_size_t N, memory_safety S>
constexpr inline bool operator>=(const array<T, N, S>& a, const array<T, N, S>& b)
{
	return !operator<(a, b);
}


///////////////////////////////////////////////////////////////////////
// to_array
///////////////////////////////////////////////////////////////////////
namespace detail
{
	template<memory_safety Safety, class T, eastl_size_t N, eastl_size_t... I>
	constexpr auto to_array(T (&a)[N], eastl::index_sequence<I...>)
	{
		return safe_memory::array<eastl::remove_cv_t<T>, N, Safety>{{a[I]...}};
	}

	template<memory_safety Safety, class T, eastl_size_t N, eastl_size_t... I>
	constexpr auto to_array(T (&&a)[N], eastl::index_sequence<I...>)
	{
		return safe_memory::array<eastl::remove_cv_t<T>, N, Safety>{{std::move(a[I])...}};
	}
}

template<class T, eastl_size_t N, memory_safety Safety>
constexpr safe_memory::array<eastl::remove_cv_t<T>, N> to_array(T (&a)[N])
{
	static_assert(eastl::is_constructible_v<T, T&>, "element type T must be copy-initializable");
	static_assert(!eastl::is_array_v<T>, "passing multidimensional arrays to to_array is ill-formed");
	return detail::to_array<Safety>(a, eastl::make_index_sequence<N>{});
}

template<class T, eastl_size_t N, memory_safety Safety>
constexpr safe_memory::array<eastl::remove_cv_t<T>, N> to_array(T (&&a)[N])
{
	static_assert(eastl::is_move_constructible_v<T>, "element type T must be move-constructible");
	static_assert(!eastl::is_array_v<T>, "passing multidimensional arrays to to_array is ill-formed");
	return detail::to_array<Safety>(std::move(a), eastl::make_index_sequence<N>{});
}


} // namespace safe_memory


#endif // Header include guard


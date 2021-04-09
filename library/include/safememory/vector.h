/* -------------------------------------------------------------------------------
* Copyright (c) 2019, OLogN Technologies AG
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

// Initial vesion from:
// https://github.com/electronicarts/EASTL/blob/3.15.00/include/EASTL/vector.h


#ifndef SAFE_MEMORY_VECTOR_H
#define SAFE_MEMORY_VECTOR_H

#include <EASTL/vector.h>
#include <safememory/detail/allocator_to_eastl.h>
#include <safememory/detail/array_iterator.h>
#include <safe_memory_error.h>

namespace safememory
{
	template <typename T, memory_safety Safety = safeness_declarator<T>::is_safe>
	class SAFEMEMORY_DEEP_CONST_WHEN_PARAMS vector : protected eastl::vector<T, detail::allocator_to_eastl_vector<Safety>>
	{
		typedef vector<T, Safety> 										this_type;
		typedef eastl::vector<T, detail::allocator_to_eastl_vector<Safety>>    base_type;

		template <typename TT, memory_safety SS>
		friend bool operator==(const vector<TT, SS>& a, const vector<TT, SS>& b);

		template <typename TT, memory_safety SS>
		friend bool operator!=(const vector<TT, SS>& a, const vector<TT, SS>& b);

		template <typename TT, memory_safety SS>
		friend bool operator<(const vector<TT, SS>& a, const vector<TT, SS>& b);

		template <typename TT, memory_safety SS>
		friend bool operator>(const vector<TT, SS>& a, const vector<TT, SS>& b);

		template <typename TT, memory_safety SS>
		friend bool operator<=(const vector<TT, SS>& a, const vector<TT, SS>& b);

		template <typename TT, memory_safety SS>
		friend bool operator>=(const vector<TT, SS>& a, const vector<TT, SS>& b);

	public:
		typedef typename base_type::value_type                             value_type;
		typedef typename base_type::pointer                                pointer;
		typedef typename base_type::const_pointer                          const_pointer;
		typedef typename base_type::reference                              reference;
		typedef typename base_type::const_reference                        const_reference;

		typedef typename base_type::iterator 						       iterator_base;
		typedef typename base_type::const_iterator 					       const_iterator_base;
		typedef typename base_type::reverse_iterator 				       reverse_iterator_base;
		typedef typename base_type::const_reverse_iterator 			       const_reverse_iterator_base;
		typedef typename base_type::size_type                              size_type;
		typedef typename base_type::difference_type                        difference_type;

		typedef typename base_type::allocator_type                         allocator_type;
		typedef typename base_type::array_type                             array_type;

		typedef typename allocator_type::template soft_array_pointer<T>            soft_ptr_type;
		typedef typename detail::array_stack_only_iterator<T, false, T*>           stack_only_iterator;
		typedef typename detail::array_stack_only_iterator<T, true, T*>            const_stack_only_iterator;
		typedef typename detail::array_heap_safe_iterator<T, false, soft_ptr_type> heap_safe_iterator;
		typedef typename detail::array_heap_safe_iterator<T, true, soft_ptr_type>  const_heap_safe_iterator;

		static constexpr bool use_base_iterator = Safety == memory_safety::none;
		
		typedef std::conditional_t<use_base_iterator, iterator_base, stack_only_iterator>               iterator;
		typedef std::conditional_t<use_base_iterator, const_iterator_base, const_stack_only_iterator>   const_iterator;
		typedef eastl::reverse_iterator<iterator>                                                 reverse_iterator;
		typedef eastl::reverse_iterator<const_iterator>                                           const_reverse_iterator;
	
		// typedef std::conditional_t<use_base_iterator, reverse_iterator_base,
		// 							eastl::reverse_iterator<iterator>>                                  reverse_iterator;
		// typedef std::conditional_t<use_base_iterator, const_reverse_iterator_base,
		// 							eastl::reverse_iterator<const_iterator>>                            const_reverse_iterator;

		typedef heap_safe_iterator                                         iterator_safe;
		typedef const_heap_safe_iterator                                   const_iterator_safe;
		typedef eastl::reverse_iterator<iterator_safe>                     reverse_iterator_safe;
		typedef eastl::reverse_iterator<const_iterator_safe>               const_reverse_iterator_safe;

		// TODO improve when pass by-ref and when by-value
		typedef std::conditional_t<use_base_iterator, const_iterator, const const_iterator&>           const_iterator_arg;
		

		using base_type::npos;
		static constexpr memory_safety is_safe = Safety;

	public:
		vector() : base_type(allocator_type()) {}
		explicit vector(size_type n) : base_type(n, allocator_type()) {}
		vector(size_type n, const value_type& value) : base_type(n, value, allocator_type()) {}
		vector(const this_type& x) : base_type(x) {}
		vector(this_type&& x) noexcept : base_type(std::move(x)) {}
		vector(std::initializer_list<value_type> ilist) : base_type(ilist, allocator_type()) {}
//		vector(const_iterator_arg first, const_iterator_arg last);

	   ~vector() {}

		this_type& operator=(const this_type& x) { base_type::operator=(x); return *this; }
		this_type& operator=(std::initializer_list<value_type> ilist) { base_type::operator=(ilist); return *this; }
		this_type& operator=(this_type&& x) noexcept { base_type::operator=(std::move(x)); return *this; }

		void swap(this_type& x) noexcept { base_type::swap(x); }

		void assign(size_type n, const value_type& value) { base_type::assign(n, value); }

		template <typename InputIterator>
		void assign_unsafe(InputIterator first, InputIterator last) { base_type::assign(first, last); }
		
		void assign(const_iterator_arg first, const_iterator_arg last);
		void assign_safe(const const_iterator_safe& first, const const_iterator_safe& last);

		void assign(std::initializer_list<value_type> ilist) { base_type::assign(ilist); }

		pointer       begin_unsafe() noexcept { return base_type::begin(); }
		const_pointer begin_unsafe() const noexcept { return base_type::begin(); }
		const_pointer cbegin_unsafe() const noexcept { return base_type::cbegin(); }

		pointer       end_unsafe() noexcept { return base_type::end(); }
		const_pointer end_unsafe() const noexcept { return base_type::end(); }
		const_pointer cend_unsafe() const noexcept { return base_type::cend(); }

		reverse_iterator_base       rbegin_unsafe() noexcept { return base_type::rbegin(); }
		const_reverse_iterator_base rbegin_unsafe() const noexcept { return base_type::rbegin(); }
		const_reverse_iterator_base crbegin_unsafe() const noexcept { return base_type::crbegin(); }

		reverse_iterator_base       rend_unsafe() noexcept { return base_type::rend(); }
		const_reverse_iterator_base rend_unsafe() const noexcept { return base_type::rend(); }
		const_reverse_iterator_base crend_unsafe() const noexcept { return base_type::crend(); }

		iterator       begin() noexcept { return makeIt(base_type::begin()); }
		const_iterator begin() const noexcept { return makeIt(base_type::begin()); }
		const_iterator cbegin() const noexcept { return makeIt(base_type::cbegin()); }

		iterator       end() noexcept { return makeIt(base_type::end()); }
		const_iterator end() const noexcept { return makeIt(base_type::end()); }
		const_iterator cend() const noexcept { return makeIt(base_type::cend()); }

		reverse_iterator       rbegin() noexcept { return reverse_iterator(makeIt(end_unsafe())); }
		const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(makeIt(end_unsafe())); }
		const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(makeIt(end_unsafe())); }

		reverse_iterator       rend() noexcept { return reverse_iterator(makeIt(begin_unsafe())); }
		const_reverse_iterator rend() const noexcept { return const_reverse_iterator(makeIt(begin_unsafe())); }
		const_reverse_iterator crend() const noexcept { return const_reverse_iterator(makeIt(begin_unsafe())); }

		iterator_safe       begin_safe() noexcept { return makeSafeIt(base_type::begin()); }
		const_iterator_safe begin_safe() const noexcept { return makeSafeIt(base_type::begin()); }
		const_iterator_safe cbegin_safe() const noexcept { return makeSafeIt(base_type::cbegin()); }

		iterator_safe       end_safe() noexcept { return makeSafeIt(base_type::end()); }
		const_iterator_safe end_safe() const noexcept { return makeSafeIt(base_type::end()); }
		const_iterator_safe cend_safe() const noexcept { return makeSafeIt(base_type::cend()); }

		reverse_iterator_safe       rbegin_safe() noexcept { return reverse_iterator_safe(makeSafeIt(end_unsafe())); }
		const_reverse_iterator_safe rbegin_safe() const noexcept { return const_reverse_iterator_safe(makeSafeIt(end_unsafe())); }
		const_reverse_iterator_safe crbegin_safe() const noexcept { return const_reverse_iterator_safe(makeSafeIt(end_unsafe())); }

		reverse_iterator_safe       rend_safe() noexcept { return reverse_iterator_safe(makeSafeIt(begin_unsafe())); }
		const_reverse_iterator_safe rend_safe() const noexcept { return const_reverse_iterator_safe(makeSafeIt(begin_unsafe())); }
		const_reverse_iterator_safe crend_safe() const noexcept { return const_reverse_iterator_safe(makeSafeIt(begin_unsafe())); }

		using base_type::empty;
		using base_type::size;
		using base_type::capacity;

		//TODO: mb this is not really well checked at eastl::vector
		size_type max_size() const { return base_type::kMaxSize; }

		using base_type::resize;
		using base_type::reserve;
		using base_type::set_capacity;
		using base_type::shrink_to_fit;

		pointer       data_unsafe() noexcept { return base_type::data(); }
		const_pointer data_unsafe() const noexcept { return base_type::data(); }

		reference       operator[](size_type n);
		const_reference operator[](size_type n) const;

		reference       at(size_type n);
		const_reference at(size_type n) const;

		reference       front();
		const_reference front() const;

		reference       back();
		const_reference back() const;

		using base_type::push_back;

		void pop_back();

		//not allowed
//		void*     push_back_uninitialized();

		template<class... Args>
		pointer emplace_unsafe(const_pointer position, Args&&... args) {
			return base_type::emplace(position, std::forward<Args>(args)...);
		}
		
		template<class... Args>
		iterator emplace(const_iterator_arg position, Args&&... args) {
		return makeIt(base_type::emplace(toBase(position), std::forward<Args>(args)...));
		}

		template<class... Args>
		iterator_safe emplace_safe(const const_iterator_safe& position, Args&&... args) {
			return makeSafeIt(base_type::emplace(toBase(position), std::forward<Args>(args)...));
		}


		using base_type::emplace_back;

		pointer insert_unsafe(const_pointer position, const value_type& value) { return base_type::insert(position, value); }
		pointer insert_unsafe(const_pointer position, size_type n, const value_type& value) { return base_type::insert(position, n, value); }
		pointer insert_unsafe(const_pointer position, value_type&& value) { return base_type::insert(position, std::move(value)); }
		pointer insert_unsafe(const_pointer position, std::initializer_list<value_type> ilist) { return base_type::insert(position, ilist); }

		template <typename InputIterator>
		pointer insert_unsafe(const_pointer position, InputIterator first, InputIterator last) { return base_type::insert(position, first, last); }

		iterator insert(const_iterator_arg position, const value_type& value) {
			return makeIt(base_type::insert(toBase(position), value));
		}

		iterator insert(const_iterator_arg position, size_type n, const value_type& value) {
			return makeIt(base_type::insert(toBase(position), n, value));
		}

		iterator insert(const_iterator_arg position, value_type&& value) {
			return makeIt(base_type::insert(toBase(position), std::move(value)));
		}

		iterator insert(const_iterator_arg position, std::initializer_list<value_type> ilist) {
			return makeIt(base_type::insert(toBase(position), ilist));
		}


		// template <typename InputIterator>
		iterator insert(const_iterator_arg position, const_iterator_arg first, const_iterator_arg last) {
			auto other = toBaseOther(first, last);
			return makeIt(base_type::insert(toBase(position), other.first, other.second));
		}


		iterator_safe insert_safe(const const_iterator_safe& position, const value_type& value) {
			return makeSafeIt(base_type::insert(toBase(position), value));
		}

		iterator_safe insert_safe(const const_iterator_safe& position, size_type n, const value_type& value)	{
			return makeSafeIt(base_type::insert(toBase(position), n, value));
		}

		iterator_safe insert_safe(const const_iterator_safe& position, value_type&& value) {
			return makeSafeIt(base_type::insert(toBase(position), std::move(value)));
		}

		iterator_safe insert_safe(const const_iterator_safe& position, std::initializer_list<value_type> ilist) {
			return makeSafeIt(base_type::insert(toBase(position), ilist));
		}


		// template <typename InputIterator>
		iterator_safe insert_safe(const const_iterator_safe& position, const const_iterator_safe& first, const const_iterator_safe& last) {
			auto other = toBaseOther(first, last);
			return makeSafeIt(base_type::insert(toBase(position), other.first, other.second));
		}


		// iterator erase_first(const T& value);
		// iterator erase_first_unsorted(const T& value); // Same as erase, except it doesn't preserve order, but is faster because it simply copies the last item in the vector over the erased position.
		// reverse_iterator erase_last(const T& value);
		// reverse_iterator erase_last_unsorted(const T& value); // Same as erase, except it doesn't preserve order, but is faster because it simply copies the last item in the vector over the erased position.


		pointer erase_unsafe(const_pointer position) { return base_type::erase(position); }
		pointer erase_unsafe(const_pointer first, const_pointer last) { return base_type::erase(first, last); }
		pointer erase_unsorted_unsafe(const_pointer position) { return base_type::erase_unsorted(position); }

		reverse_iterator_base erase_unsafe(const_reverse_iterator_base position) { return base_type::erase(position); }
		reverse_iterator_base erase_unsafe(const_reverse_iterator_base first, const_reverse_iterator_base last) { return base_type::erase(first, last); }
		reverse_iterator_base erase_unsorted_unsafe(const_reverse_iterator_base position) { return base_type::erase_unsorted(position); }

		iterator erase(const_iterator_arg position) 	{
		return makeIt(base_type::erase(toBase(position)));
	}

		iterator erase(const_iterator_arg first, const_iterator_arg last) 	{
		auto p = toBase(first, last);
		return makeIt(base_type::erase(p.first, p.second));
	}

		iterator erase_unsorted(const_iterator_arg position) {
			return makeIt(base_type::erase_unsorted(toBase(position)));
		}

		iterator_safe erase_safe(const const_iterator_safe& position) 	{
			return makeSafeIt(base_type::erase(toBase(position)));
		}

		iterator_safe erase_safe(const const_iterator_safe& first, const const_iterator_safe& last) 	{
			auto p = toBase(first, last);
			return makeSafeIt(base_type::erase(p.first, p.second));
		}

		iterator_safe erase_unsorted_safe(const const_iterator_safe& position) {
			return makeSafeIt(base_type::erase_unsorted(toBase(position)));
		}

		using base_type::clear;
		//not allowed
		// void reset_lose_memory() noexcept;


		using base_type::validate;
		int validate_iterator(const_iterator_base it) const noexcept { return base_type::validate_iterator(it); }
		//TODO: custom validation for safe iterators
		int validate_iterator(const const_stack_only_iterator& it) const noexcept { return base_type::validate_iterator(toBase(it)); }
		int validate_iterator(const const_heap_safe_iterator& it) const noexcept { return base_type::validate_iterator(toBase(it)); }

		iterator_safe make_safe(const iterator& position) { return makeSafeIt(toBase(position)); }
		const_iterator_safe make_safe(const const_iterator_arg& position) const { return makeSafeIt(toBase(position)); }

	protected:
		[[noreturn]] static void ThrowRangeException(const char* msg) { throw nodecpp::error::out_of_range; }
		[[noreturn]] static void ThrowInvalidArgumentException(const char* msg) { throw nodecpp::error::out_of_range; }
		[[noreturn]] static void ThrowMaxSizeException(const char* msg) { throw nodecpp::error::out_of_range; }


        const base_type& toBase() const noexcept { return *this; }

		// Safety == none
		iterator_base toBase(iterator_base it) const { return it; }
		const_iterator_base toBase(const_iterator_base it) const { return it; }
		std::pair<const_iterator_base, const_iterator_base> toBase(const_iterator_base it, const_iterator_base it2) const { return { it, it2 }; }
		std::pair<const_iterator_base, const_iterator_base> toBaseOther(const_iterator_base it, const_iterator_base it2) const { return { it, it2 }; }
		
		// Safety == safe
		iterator_base toBase(const stack_only_iterator& it) const {
			return it.toRaw(begin_unsafe());
		}

		const_iterator_base toBase(const const_stack_only_iterator& it) const {
			return it.toRaw(begin_unsafe());
		}

		std::pair<const_iterator_base, const_iterator_base> toBase(const const_stack_only_iterator& it, const const_stack_only_iterator& it2) const {
			return it.toRaw(begin_unsafe(), it2);
		}

		std::pair<const_iterator_base, const_iterator_base> toBaseOther(const const_stack_only_iterator& it, const const_stack_only_iterator& it2) const {
			return it.toRawOther(it2);
		}

		iterator_base toBase(const heap_safe_iterator& it) const {
			return it.toRaw(begin_unsafe());
		}

		const_iterator_base toBase(const const_heap_safe_iterator& it) const {
			return it.toRaw(begin_unsafe());
		}

		std::pair<const_iterator_base, const_iterator_base> toBase(const const_heap_safe_iterator& it, const const_heap_safe_iterator& it2) const {
			return it.toRaw(begin_unsafe(), it2);
		}

		std::pair<const_iterator_base, const_iterator_base> toBaseOther(const const_heap_safe_iterator& it, const const_heap_safe_iterator& it2) const {
			return it.toRawOther(it2);
		}


		iterator makeIt(iterator_base it) {
			if constexpr (use_base_iterator)
				return it;
			else {
				return iterator::makePtr(base_type::data(), it, capacity(), static_cast<const base_type*>(this));
			}
		}
		const_iterator makeIt(const_iterator_base it) const {
			if constexpr (use_base_iterator)
				return it;
			else
				return const_iterator::makePtr(const_cast<T*>(base_type::data()), it, capacity(), static_cast<const base_type*>(this));
		}

		// reverse_iterator makeIt(const reverse_iterator_base& it) {
		// 	if constexpr (use_base_iterator)
		// 		return it;
		// 	else
		// 		return reverse_iterator(makeIt(it.base()));
		// }
		// const_reverse_iterator makeIt(const const_reverse_iterator_base& it) const {
		// 	if constexpr (use_base_iterator)
		// 		return it;
		// 	else
		// 		return const_reverse_iterator(makeIt(it.base()));
		// }



		iterator_safe makeSafeIt(iterator_base it) {
			// if constexpr (use_base_iterator)
			// 	return it;
			// else
				return iterator_safe::makePtr(allocator_type::to_soft(base_type::mpBegin), it, capacity(), static_cast<const base_type*>(this));
		}
		
		const_iterator_safe makeSafeIt(const_iterator_base it) const {
			// if constexpr (use_base_iterator)
			// 	return it;
			// else
				return const_iterator_safe::makePtr(allocator_type::to_soft(base_type::mpBegin), it, capacity(), static_cast<const base_type*>(this));
		}

		// reverse_iterator_safe makeSafeIt(const typename base_type::reverse_iterator& it) {
		// 	// if constexpr (use_base_iterator)
		// 	// 	return it;
		// 	// else
		// 		return reverse_iterator_safe(makeSafeIt(it.base()));
		// }
		// const_reverse_iterator_safe makeSafeIt(const typename base_type::const_reverse_iterator& it) const {
		// 	// if constexpr (use_base_iterator)
		// 	// 	return it;
		// 	// else
		// 		return const_reverse_iterator_safe(makeSafeIt(it.base()));
		// }

	}; // class vector


	// template <typename T, memory_safety Safety>
	// inline vector<T, Safety>::vector(const_iterator_arg first, const_iterator_arg last)
	// 	: base_type()
	// {
	// 	const_pointer_pair p = CheckAndGet(first, last);
	// 	size_type sz = static_cast<size_type>(p.second - p.first);

	// 	SetNewHeap(DoAllocate(sz));
	// 	mpEnd = std::uninitialized_copy(p.first, p.second, mpBegin);
	// }



	template <typename T, memory_safety Safety>
	inline void vector<T, Safety>::assign(const_iterator_arg first, const_iterator_arg last)
	{
		auto p = toBaseOther(first, last);
		base_type::assign(p.first, p.second);
	}

	template <typename T, memory_safety Safety>
	inline void vector<T, Safety>::assign_safe(const const_iterator_safe& first, const const_iterator_safe& last)
	{
		auto p = toBaseOther(first, last);
		base_type::assign(p.first, p.second);
	}

	template <typename T, memory_safety Safety>
	inline typename vector<T, Safety>::reference
	vector<T, Safety>::operator[](size_type n)
	{
		if constexpr(is_safe == memory_safety::safe) {
			if(NODECPP_UNLIKELY(n >= size()))
				ThrowRangeException("vector::operator[] -- out of range");
		}

		return base_type::operator[](n);
	}


	template <typename T, memory_safety Safety>
	inline typename vector<T, Safety>::const_reference
	vector<T, Safety>::operator[](size_type n) const
	{
		if constexpr(is_safe == memory_safety::safe) {
			if(NODECPP_UNLIKELY(n >= size()))
				ThrowRangeException("vector::operator[] -- out of range");
		}

		return base_type::operator[](n);
	}

	template <typename T, memory_safety Safety>
	inline typename vector<T, Safety>::reference
	vector<T, Safety>::at(size_type n)
	{
		if constexpr(is_safe == memory_safety::safe) {
			if(NODECPP_UNLIKELY(n >= size()))
				ThrowRangeException("vector::at -- out of range");

			//now return unckeched
			return base_type::operator[](n);
		}
		else
			return base_type::at(n);
	}


	template <typename T, memory_safety Safety>
	inline typename vector<T, Safety>::const_reference
	vector<T, Safety>::at(size_type n) const
	{
		if constexpr(is_safe == memory_safety::safe) {
			if(NODECPP_UNLIKELY(n >= size()))
				ThrowRangeException("vector::at -- out of range");

			//now return unckeched
			return base_type::operator[](n);
		}
		else
			return base_type::at(n);
	}


	template <typename T, memory_safety Safety>
	inline typename vector<T, Safety>::reference
	vector<T, Safety>::front()
	{
		if constexpr(is_safe == memory_safety::safe) {
			if(NODECPP_UNLIKELY(empty()))
				ThrowRangeException("vector::front -- empty vector");
		}

		return base_type::front();
	}


	template <typename T, memory_safety Safety>
	inline typename vector<T, Safety>::const_reference
	vector<T, Safety>::front() const
	{
		if constexpr(is_safe == memory_safety::safe) {
			if(NODECPP_UNLIKELY(empty()))
				ThrowRangeException("vector::front -- empty vector");
		}

		return base_type::front();
	}


	template <typename T, memory_safety Safety>
	inline typename vector<T, Safety>::reference
	vector<T, Safety>::back()
	{
		if constexpr(is_safe == memory_safety::safe) {
			if(NODECPP_UNLIKELY(empty()))
				ThrowRangeException("vector::front -- empty vector");
		}

		return base_type::back();
	}


	template <typename T, memory_safety Safety>
	inline typename vector<T, Safety>::const_reference
	vector<T, Safety>::back() const
	{
		if constexpr(is_safe == memory_safety::safe) {
			if(NODECPP_UNLIKELY(empty()))
				ThrowRangeException("vector::front -- empty vector");
		}

		return base_type::back();
	}


	template <typename T, memory_safety Safety>
	inline void vector<T, Safety>::pop_back()
	{
		if constexpr(is_safe == memory_safety::safe) {
			if(NODECPP_UNLIKELY(empty()))
				ThrowRangeException("vector::pop_back -- empty vector");
		}

		return base_type::pop_back();
	}


	///////////////////////////////////////////////////////////////////////
	// global operators
	///////////////////////////////////////////////////////////////////////

	template <typename T, memory_safety Safety>
	inline bool operator==(const vector<T, Safety>& a, const vector<T, Safety>& b)
	{
		return operator==(a.toBase(), b.toBase());
	}


	template <typename T, memory_safety Safety>
	inline bool operator!=(const vector<T, Safety>& a, const vector<T, Safety>& b)
	{
		return operator!=(a.toBase(), b.toBase());
	}


	template <typename T, memory_safety Safety>
	inline bool operator<(const vector<T, Safety>& a, const vector<T, Safety>& b)
	{
		return operator<(a.toBase(), b.toBase());
	}


	template <typename T, memory_safety Safety>
	inline bool operator>(const vector<T, Safety>& a, const vector<T, Safety>& b)
	{
		return operator>(a.toBase(), b.toBase());
	}


	template <typename T, memory_safety Safety>
	inline bool operator<=(const vector<T, Safety>& a, const vector<T, Safety>& b)
	{
		return operator<=(a.toBase(), b.toBase());
	}


	template <typename T, memory_safety Safety>
	inline bool operator>=(const vector<T, Safety>& a, const vector<T, Safety>& b)
	{
		return operator>=(a.toBase(), b.toBase());
	}


	template <typename T, memory_safety Safety>
	inline void swap(vector<T, Safety>& a, vector<T, Safety>& b) noexcept
	{
		a.swap(b);
	}



	template <typename T, memory_safety Safety = safeness_declarator<T>::is_safe>
	class SAFEMEMORY_DEEP_CONST_WHEN_PARAMS vector_safe : public vector<T, Safety>
	{
		typedef vector_safe<T, Safety> 										this_type;
		typedef vector<T, Safety>                                           base_type;


	public:
		using typename base_type::value_type;
		using typename base_type::pointer;
		using typename base_type::const_pointer;
		using typename base_type::reference;
		using typename base_type::const_reference;

		typedef typename base_type::iterator_safe 						  	iterator;
		typedef typename base_type::const_iterator_safe                   	const_iterator;
		typedef typename base_type::reverse_iterator_safe 					reverse_iterator;
		typedef typename base_type::const_reverse_iterator_safe 			const_reverse_iterator;

		using typename base_type::iterator_safe;
		using typename base_type::const_iterator_safe;
		using typename base_type::reverse_iterator_safe;
		using typename base_type::const_reverse_iterator_safe;

		using typename base_type::size_type;
		using typename base_type::difference_type;

		using typename base_type::allocator_type;
		using typename base_type::array_type;

		typedef const const_iterator&                                    	const_iterator_arg;
		
		using base_type::npos;
		static constexpr memory_safety is_safe = Safety;

	public:
		vector_safe() : base_type() {}
		explicit vector_safe(size_type n) : base_type(n) {}
		vector_safe(size_type n, const value_type& value) : base_type(n, value) {}
		vector_safe(const this_type& x) : base_type(x) {}
		vector_safe(this_type&& x) noexcept : base_type(std::move(x)) {}
		vector_safe(std::initializer_list<value_type> ilist) : base_type(ilist) {}
//		vector(const_iterator_arg first, const_iterator_arg last);

	   ~vector_safe() {}

		this_type& operator=(const this_type& x) { base_type::operator=(x); return *this; }
		this_type& operator=(std::initializer_list<value_type> ilist) { base_type::operator=(ilist); return *this; }
		this_type& operator=(this_type&& x) noexcept { base_type::operator=(std::move(x)); return *this; }

		// void swap(this_type& x) noexcept { base_type::swap(x); }

		 void assign(size_type n, const value_type& value) { base_type::assign(n, value); }

		// template <typename InputIterator>
		// void assign_unsafe(InputIterator first, InputIterator last) { base_type::assign(first, last); }
		
		void assign(const_iterator_arg first, const_iterator_arg last) { base_type::assign_safe(first, last); }
		void assign(std::initializer_list<value_type> ilist) { base_type::assign(ilist); }

		// pointer       begin_unsafe() noexcept { return base_type::begin(); }
		// const_pointer begin_unsafe() const noexcept { return base_type::begin(); }
		// const_pointer cbegin_unsafe() const noexcept { return base_type::cbegin(); }

		// pointer       end_unsafe() noexcept { return base_type::end(); }
		// const_pointer end_unsafe() const noexcept { return base_type::end(); }
		// const_pointer cend_unsafe() const noexcept { return base_type::cend(); }

		// reverse_iterator_base       rbegin_unsafe() noexcept { return base_type::rbegin(); }
		// const_reverse_iterator_base rbegin_unsafe() const noexcept { return base_type::rbegin(); }
		// const_reverse_iterator_base crbegin_unsafe() const noexcept { return base_type::crbegin(); }

		// reverse_iterator_base       rend_unsafe() noexcept { return base_type::rend(); }
		// const_reverse_iterator_base rend_unsafe() const noexcept { return base_type::rend(); }
		// const_reverse_iterator_base crend_unsafe() const noexcept { return base_type::crend(); }

		iterator       begin() noexcept { return base_type::begin_safe(); }
		const_iterator begin() const noexcept { return base_type::begin_safe(); }
		const_iterator cbegin() const noexcept { return base_type::cbegin_safe(); }

		iterator       end() noexcept { return base_type::end_safe(); }
		const_iterator end() const noexcept { return base_type::end_safe(); }
		const_iterator cend() const noexcept { return base_type::cend_safe(); }

		reverse_iterator       rbegin() noexcept { return base_type::rbegin_safe(); }
		const_reverse_iterator rbegin() const noexcept { return base_type::rbegin_safe(); }
		const_reverse_iterator crbegin() const noexcept { return base_type::crbegin_safe(); }

		reverse_iterator       rend() noexcept { return base_type::rend_safe(); }
		const_reverse_iterator rend() const noexcept { return base_type::rend_safe(); }
		const_reverse_iterator crend() const noexcept { return base_type::crend_safe(); }

		using base_type::begin_safe;
		using base_type::cbegin_safe;
		using base_type::end_safe;
		using base_type::cend_safe;
		using base_type::rbegin_safe;
		using base_type::crbegin_safe;
		using base_type::rend_safe;
		using base_type::crend_safe;


		// using base_type::empty;
		// using base_type::size;
		// using base_type::capacity;

		// //TODO: mb this is not really well checked at eastl::vector
		// using base_type::max_size;

		// using base_type::resize;
		// using base_type::reserve;
		// using base_type::set_capacity;
		// using base_type::shrink_to_fit;

		// pointer       data_unsafe() noexcept { return base_type::data(); }
		// const_pointer data_unsafe() const noexcept { return base_type::data(); }

		// reference       operator[](size_type n);
		// const_reference operator[](size_type n) const;

		// reference       at(size_type n);
		// const_reference at(size_type n) const;

		// reference       front();
		// const_reference front() const;

		// reference       back();
		// const_reference back() const;

		// using base_type::push_back;

		// void pop_back();

		//not allowed
//		void*     push_back_uninitialized();

		// template<class... Args>
		// pointer emplace_unsafe(const_pointer position, Args&&... args) {
		// 	return base_type::emplace(position, std::forward<Args>(args)...);
		// }
		
		template<class... Args>
		iterator emplace(const_iterator_arg position, Args&&... args) {
			return base_type::emplace_safe(position, std::forward<Args>(args)...);
		}

		// template<class... Args>
		// iterator emplace_safe(const const_iterator_safe& position, Args&&... args) {
		// 	return makeSafeIt(base_type::emplace(toBase(position), std::forward<Args>(args)...));
		// }


		// using base_type::emplace_back;

		// pointer insert_unsafe(const_pointer position, const value_type& value) { return base_type::insert(position, value); }
		// pointer insert_unsafe(const_pointer position, size_type n, const value_type& value) { return base_type::insert(position, n, value); }
		// pointer insert_unsafe(const_pointer position, value_type&& value) { return base_type::insert(position, std::move(value)); }
		// pointer insert_unsafe(const_pointer position, std::initializer_list<value_type> ilist) { return base_type::insert(position, ilist); }

		// template <typename InputIterator>
		// pointer insert_unsafe(const_pointer position, InputIterator first, InputIterator last) { return base_type::insert(position, first, last); }

		iterator insert(const_iterator_arg position, const value_type& value) {
			return base_type::insert_safe(position, value);
		}

		iterator insert(const_iterator_arg position, size_type n, const value_type& value) {
			return base_type::insert_safe(position, n, value);
		}

		iterator insert(const_iterator_arg position, value_type&& value) {
			return base_type::insert_safe(position, std::move(value));
		}

		iterator insert(const_iterator_arg position, std::initializer_list<value_type> ilist) {
			return base_type::insert_safe(position, ilist);
		}


		// template <typename InputIterator>
		iterator insert(const_iterator_arg position, const_iterator_arg first, const_iterator_arg last) {
			return base_type::insert_safe(position, first, last);
		}


		// iterator_safe insert_safe(const const_iterator_safe& position, const value_type& value) {
		// 	return makeSafeIt(base_type::insert(toBase(position), value));
		// }

		// iterator_safe insert_safe(const const_iterator_safe& position, size_type n, const value_type& value)	{
		// 	return makeSafeIt(base_type::insert(toBase(position), n, value));
		// }

		// iterator_safe insert_safe(const const_iterator_safe& position, value_type&& value) {
		// 	return makeSafeIt(base_type::insert(toBase(position), std::move(value)));
		// }

		// iterator_safe insert_safe(const const_iterator_safe& position, std::initializer_list<value_type> ilist) {
		// 	return makeSafeIt(base_type::insert(toBase(position), ilist));
		// }


		// // template <typename InputIterator>
		// iterator_safe insert_safe(const const_iterator_safe& position, const const_iterator_safe& first, const const_iterator_safe& last) {
		// 	auto other = toBaseOther(first, last);
		// 	return makeSafeIt(base_type::insert(toBase(position), other.first, other.second));
		// }


		// iterator erase_first(const T& value);
		// iterator erase_first_unsorted(const T& value); // Same as erase, except it doesn't preserve order, but is faster because it simply copies the last item in the vector over the erased position.
		// reverse_iterator erase_last(const T& value);
		// reverse_iterator erase_last_unsorted(const T& value); // Same as erase, except it doesn't preserve order, but is faster because it simply copies the last item in the vector over the erased position.


		// pointer erase_unsafe(const_pointer position) { return base_type::erase(position); }
		// pointer erase_unsafe(const_pointer first, const_pointer last) { return base_type::erase(first, last); }
		// pointer erase_unsorted_unsafe(const_pointer position) { return base_type::erase_unsorted(position); }

		// reverse_iterator_base erase_unsafe(const_reverse_iterator_base position) { return base_type::erase(position); }
		// reverse_iterator_base erase_unsafe(const_reverse_iterator_base first, const_reverse_iterator_base last) { return base_type::erase(first, last); }
		// reverse_iterator_base erase_unsorted_unsafe(const_reverse_iterator_base position) { return base_type::erase_unsorted(position); }

		iterator erase(const_iterator_arg position) 	{
			return base_type::erase_safe(position);
		}

		iterator erase(const_iterator_arg first, const_iterator_arg last) 	{
			return base_type::erase_safe(first, last);
		}

		iterator erase_unsorted(const_iterator_arg position) {
			return base_type::erase_unsorted_safe(position);
		}

		// iterator_safe erase_safe(const const_iterator_safe& position) 	{
		// 	return makeSafeIt(base_type::erase(toBase(position)));
		// }

		// iterator_safe erase_safe(const const_iterator_safe& first, const const_iterator_safe& last) 	{
		// 	auto p = toBase(first, last);
		// 	return makeSafeIt(base_type::erase(p.first, p.second));
		// }

		// iterator_safe erase_unsorted_safe(const const_iterator_safe& position) {
		// 	return makeSafeIt(base_type::erase_unsorted(toBase(position)));
		// }

		// using base_type::clear;
		//not allowed
		// void reset_lose_memory() noexcept;

		// iterator_safe make_safe(const iterator& position) {
		// 	return makeSafeIt(toBase(position));
		// }

		// const_iterator_safe make_safe(const const_iterator_arg& position) const {
		// 	return makeSafeIt(toBase(position));
		// }


		// using base_type::validate;
		// using base_type::validate_iterator;

		// detail::iterator_validity  validate_iterator2(const_iterator_arg i) const noexcept;

	}; // class vector_safe


} // namespace safememory

#endif // Header include guard

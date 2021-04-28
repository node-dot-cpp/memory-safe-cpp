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

#ifndef SAFE_MEMORY_UNORDERED_MAP_H
#define SAFE_MEMORY_UNORDERED_MAP_H

#include <utility>
#include <typeindex>
#include <stdexcept> // before EASTL/unordered_map.h
#include <EASTL/unordered_map.h>
#include <EASTL/unordered_set.h>
#include <safememory/functional.h>
#include <safememory/detail/allocator_to_eastl.h>
#include <safememory/detail/hashtable_iterator.h>


namespace safememory
{
	template <typename Key, typename T, typename Hash = hash<Key>, typename Predicate = equal_to<Key>, 
			  memory_safety Safety = safeness_declarator<Key>::is_safe>
	class SAFEMEMORY_DEEP_CONST_WHEN_PARAMS unordered_map
		: protected eastl::unordered_map<Key, T, Hash, Predicate, detail::allocator_to_eastl_hashtable<Safety>>
	{
	public:
		typedef eastl::unordered_map<Key, T, Hash, Predicate, detail::allocator_to_eastl_hashtable<Safety>> base_type;
		typedef unordered_map<Key, T, Hash, Predicate, Safety>                    this_type;
		typedef typename base_type::size_type                                     size_type;
		typedef typename base_type::key_type                                      key_type;
		typedef typename base_type::mapped_type                                   mapped_type;
		typedef typename base_type::value_type                                    value_type;     // NOTE: 'value_type = pair<const key_type, mapped_type>'.
		typedef typename base_type::allocator_type                                allocator_type;
		typedef typename base_type::node_type                                     node_type;
		typedef typename base_type::iterator                                      iterator_base;
		typedef typename base_type::const_iterator                                const_iterator_base;
		typedef typename base_type::local_iterator                                local_iterator_base;
		typedef typename base_type::const_local_iterator                          const_local_iterator_base;
		typedef typename base_type::insert_return_type                            insert_return_type_base;

		typedef typename detail::hashtable_stack_only_iterator<iterator_base, iterator_base, allocator_type>       stack_only_iterator;
		typedef typename detail::hashtable_stack_only_iterator<const_iterator_base, iterator_base, allocator_type>  const_stack_only_iterator;
		typedef typename detail::hashtable_heap_safe_iterator2<iterator_base, iterator_base, allocator_type>        heap_safe_iterator;
		typedef typename detail::hashtable_heap_safe_iterator2<const_iterator_base, iterator_base, allocator_type>   const_heap_safe_iterator;

		typedef typename detail::hashtable_stack_only_iterator<local_iterator_base, local_iterator_base, allocator_type>       stack_only_local_iterator;
		typedef typename detail::hashtable_stack_only_iterator<const_local_iterator_base, local_iterator_base, allocator_type>  const_stack_only_local_iterator;


	    static constexpr memory_safety is_safe = allocator_type::is_safe;
		// mb: for 'memory_safety::none' we can boil down to use the base (eastl) iterator,
		// or use the same iterator as 'safe' but passing the 'memory_safety::none' parameter
		// down the line 
		static constexpr bool use_base_iterator = is_safe == memory_safety::none;
		
		typedef std::conditional_t<use_base_iterator, iterator_base, stack_only_iterator>               iterator;
		typedef std::conditional_t<use_base_iterator, const_iterator_base, const_stack_only_iterator>   const_iterator;
		typedef eastl::pair<iterator, bool>                                                             insert_return_type;

		typedef heap_safe_iterator                                                    iterator_safe;
		typedef const_heap_safe_iterator                                              const_iterator_safe;
		typedef eastl::pair<iterator_safe, bool>                                      insert_return_type_safe;

		typedef std::conditional_t<use_base_iterator, local_iterator_base, stack_only_local_iterator>               local_iterator;
		typedef std::conditional_t<use_base_iterator, const_local_iterator_base, const_stack_only_local_iterator>   const_local_iterator;


	public:
		explicit unordered_map(): base_type(allocator_type()) {}
	 	explicit unordered_map(size_type nBucketCount, const Hash& hashFunction = Hash(), 
						  const Predicate& predicate = Predicate())
			: base_type(nBucketCount, hashFunction, predicate, allocator_type())
		    {}
		unordered_map(const this_type& x) = default;
		unordered_map(this_type&& x) = default;
		unordered_map(std::initializer_list<value_type> ilist, size_type nBucketCount = 0, const Hash& hashFunction = Hash(), 
				   const Predicate& predicate = Predicate())
			: base_type(ilist, nBucketCount, hashFunction, predicate, allocator_type())
            {}

		~unordered_map() = default;

		this_type& operator=(const this_type& x) = default;
		this_type& operator=(this_type&& x) = default;
		this_type& operator=(std::initializer_list<value_type> ilist) {
			checkNotNull();
			base_type::operator=(ilist);
			return *this;
		}

		void swap(this_type& x) noexcept { base_type::swap(x); }

		iterator       begin() { checkNotNull(); return makeIt(base_type::begin()); }
		const_iterator begin() const { checkNotNull(); return makeIt(base_type::begin()); }
		const_iterator cbegin() const { checkNotNull(); return makeIt(base_type::cbegin()); }

		iterator       end() { checkNotNull(); return makeIt(base_type::end()); }
		const_iterator end() const { checkNotNull(); return makeIt(base_type::end()); }
		const_iterator cend() const { checkNotNull(); return makeIt(base_type::cend()); }

		iterator_safe       begin_safe() { checkNotNull(); return makeSafeIt(base_type::begin()); }
		const_iterator_safe begin_safe() const { checkNotNull(); return makeSafeIt(base_type::begin()); }
		const_iterator_safe cbegin_safe() const { checkNotNull(); return makeSafeIt(base_type::cbegin()); }

		iterator_safe       end_safe() { checkNotNull(); return makeSafeIt(base_type::end()); }
		const_iterator_safe end_safe() const { checkNotNull(); return makeSafeIt(base_type::end()); }
		const_iterator_safe cend_safe() const { checkNotNull(); return makeSafeIt(base_type::cend()); }

		local_iterator begin(size_type n) { checkNotNull(); checkBucketIx(n); return makeLocalIt(base_type::begin(n)); }
		const_local_iterator begin(size_type n) const { checkNotNull(); checkBucketIx(n); return makeLocalIt(base_type::begin(n)); }
		const_local_iterator cbegin(size_type n) const { checkNotNull(); checkBucketIx(n); return makeLocalIt(base_type::cbegin(n)); }

		local_iterator end(size_type n) noexcept { checkNotNull(); checkBucketIx(n); return makeLocalIt(base_type::end(n)); }
		const_local_iterator end(size_type n) const noexcept { checkNotNull(); checkBucketIx(n); return makeLocalIt(base_type::end(n)); }
		const_local_iterator cend(size_type n) const noexcept { checkNotNull(); checkBucketIx(n); return makeLocalIt(base_type::cend(n)); }

		T& at(const key_type& k) { checkNotNull(); return base_type::at(k); }
		const T& at(const key_type& k) const { checkNotNull(); return base_type::at(k); }
		mapped_type& operator[](const key_type& key) { checkNotNull(); return base_type::operator[](key); }
		mapped_type& operator[](key_type&& key) { checkNotNull(); return base_type::operator[](std::move(key)); }

        using base_type::empty;
        using base_type::size;
        using base_type::bucket_count;
		size_type bucket_size(size_type n) const {
			checkNotNull();
			checkBucketIx(n);
			return base_type::bucket_size(n);
		}

		float load_factor() const { checkNotNull(); return base_type::load_factor(); }
		float get_max_load_factor() const { checkNotNull(); return base_type::get_max_load_factor(); }
		void set_max_load_factor(float fMaxLoadFactor) { checkNotNull(); base_type::set_max_load_factor(fMaxLoadFactor); }

		template <class... Args>
		insert_return_type emplace(Args&&... args) {
			checkNotNull();
            return makeIt(base_type::emplace(std::forward<Args>(args)...));
        }

		template <class... Args>
		insert_return_type_safe emplace_safe(Args&&... args) {
			checkNotNull();
            return makeSafeIt(base_type::emplace(std::forward<Args>(args)...));
        }

		template <class... Args>
		iterator emplace_hint(const const_iterator& hint, Args&&... args) {
			checkNotNull();
            return makeIt(base_type::emplace_hint(toBase(hint), std::forward<Args>(args)...));
        }

		template <class... Args>
		iterator_safe emplace_hint_safe(const const_iterator_safe& hint, Args&&... args) {
			checkNotNull();
            return makeSafeIt(base_type::emplace_hint(toBase(hint), std::forward<Args>(args)...));
        }

		template <class... Args>
        insert_return_type try_emplace(const key_type& k, Args&&... args) {
			checkNotNull();
            return makeIt(base_type::try_emplace(k, std::forward<Args>(args)...));
        }

		template <class... Args>
        insert_return_type_safe try_emplace_safe(const key_type& k, Args&&... args) {
			checkNotNull();
            return makeSafeIt(base_type::try_emplace(k, std::forward<Args>(args)...));
        }

		template <class... Args>
        insert_return_type try_emplace(key_type&& k, Args&&... args) {
			checkNotNull();
            return makeIt(base_type::try_emplace(std::move(k), std::forward<Args>(args)...));
        }

		template <class... Args>
        insert_return_type_safe try_emplace_safe(key_type&& k, Args&&... args) {
			checkNotNull();
            return makeSafeIt(base_type::try_emplace(std::move(k), std::forward<Args>(args)...));
        }

		template <class... Args> 
        iterator try_emplace(const const_iterator& hint, const key_type& k, Args&&... args) {
			checkNotNull();
            return makeIt(base_type::try_emplace(toBase(hint), k, std::forward<Args>(args)...));
        }

		template <class... Args> 
        iterator_safe try_emplace_safe(const const_iterator_safe& hint, const key_type& k, Args&&... args) {
			checkNotNull();
            return makeSafeIt(base_type::try_emplace(toBase(hint), k, std::forward<Args>(args)...));
        }

		template <class... Args>
        iterator try_emplace(const const_iterator& hint, key_type&& k, Args&&... args) {
			checkNotNull();
            return makeIt(base_type::try_emplace(toBase(hint), std::move(k), std::forward<Args>(args)...));
        }

		template <class... Args>
        iterator_safe try_emplace_safe(const const_iterator_safe& hint, key_type&& k, Args&&... args) {
			checkNotNull();
            return makeSafeIt(base_type::try_emplace(toBase(hint), std::move(k), std::forward<Args>(args)...));
        }

		insert_return_type insert(const value_type& value) {
			checkNotNull();
            return makeIt(base_type::insert(value));
        }

		insert_return_type_safe insert_safe(const value_type& value) {
			checkNotNull();
            return makeSafeIt(base_type::insert(value));
        }

		insert_return_type insert(value_type&& value) {
			checkNotNull();
            return makeIt(base_type::insert(std::move(value)));
        }

		insert_return_type_safe insert_safe(value_type&& value) {
			checkNotNull();
            return makeSafeIt(base_type::insert(std::move(value)));
        }

		iterator insert(const const_iterator& hint, const value_type& value) {
			checkNotNull();
            return makeIt(base_type::insert(toBase(hint), value));
        }

		iterator_safe insert_safe(const const_iterator_safe& hint, const value_type& value) {
			checkNotNull();
            return makeSafeIt(base_type::insert(toBase(hint), value));
        }

		iterator insert(const const_iterator& hint, value_type&& value) {
			checkNotNull();
            return makeIt(base_type::insert(toBase(hint), std::move(value)));
        }

		iterator_safe insert_safe(const const_iterator_safe& hint, value_type&& value) {
			checkNotNull();
            return makeIt(base_type::insert(toBase(hint), std::move(value)));
        }

		void insert(std::initializer_list<value_type> ilist) {
			checkNotNull();
            base_type::insert(ilist);
        }

		template <typename InputIterator>
        void insert_unsafe(InputIterator first, InputIterator last) {
			checkNotNull();
            base_type::insert(first, last);
        }

		template <class M>
        insert_return_type insert_or_assign(const key_type& k, M&& obj) {
			checkNotNull();
            return makeIt(base_type::insert_or_assign(k, std::forward<M>(obj)));
        }

		template <class M>
        insert_return_type_safe insert_or_assign_safe(const key_type& k, M&& obj) {
			checkNotNull();
            return makeSafeIt(base_type::insert_or_assign(k, std::forward<M>(obj)));
        }

		template <class M>
        insert_return_type insert_or_assign(key_type&& k, M&& obj) {
			checkNotNull();
            return makeIt(base_type::insert_or_assign(std::move(k), std::forward<M>(obj)));
        }

		template <class M>
        insert_return_type_safe insert_or_assign_safe(key_type&& k, M&& obj) {
			checkNotNull();
            return makeSafeIt(base_type::insert_or_assign(std::move(k), std::forward<M>(obj)));
        }

		template <class M>
        iterator insert_or_assign(const const_iterator& hint, const key_type& k, M&& obj) {
			checkNotNull();
            return makeIt(base_type::insert_or_assign(toBase(hint), k, std::forward<M>(obj)));
        }

		template <class M>
        iterator_safe insert_or_assign_safe(const const_iterator_safe& hint, const key_type& k, M&& obj) {
			checkNotNull();
            return makeSafeIt(base_type::insert_or_assign(toBase(hint), k, std::forward<M>(obj)));
        }

		template <class M>
        iterator insert_or_assign(const const_iterator& hint, key_type&& k, M&& obj) {
			checkNotNull();
            return makeIt(base_type::insert_or_assign(toBase(hint), std::move(k), std::forward<M>(obj)));
        }

		template <class M>
        iterator_safe insert_or_assign_safe(const const_iterator_safe& hint, key_type&& k, M&& obj) {
			checkNotNull();
            return makeSafeIt(base_type::insert_or_assign(toBase(hint), std::move(k), std::forward<M>(obj)));
        }

		iterator erase(const const_iterator& position) {
			checkNotNull();
            return makeIt(base_type::erase(toBase(position)));
        }

		iterator_safe erase_safe(const const_iterator_safe& position) {
			checkNotNull();
            return makeSafeIt(base_type::erase(toBase(position)));
        }

		iterator erase(const const_iterator& first, const const_iterator& last) {
			checkNotNull();
            return makeIt(base_type::erase(toBase(first), toBase(last)));
        }

		iterator_safe erase_safe(const const_iterator_safe& first, const const_iterator_safe& last) {
			checkNotNull();
            return makeSafeIt(base_type::erase(toBase(first), toBase(last)));
        }

		size_type erase(const key_type& k) {
			checkNotNull();
			return base_type::erase(k);
		}

        void clear() { checkNotNull(); base_type::clear(); }
        void rehash(size_type nBucketCount) { checkNotNull(); base_type::rehash(nBucketCount); }
        void reserve(size_type nElementCount) { checkNotNull(); base_type::reserve(nElementCount); }

		iterator       find(const key_type& key) { checkNotNull(); return makeIt(base_type::find(key)); }
		iterator_safe       find_safe(const key_type& key) { checkNotNull(); return makeSafeIt(base_type::find(key)); }

		const_iterator find(const key_type& key) const { checkNotNull(); return makeIt(base_type::find(key)); }
		const_iterator_safe find_safe(const key_type& key) const { checkNotNull(); return makeSafeIt(base_type::find(key)); }

		size_type count(const key_type& k) const { checkNotNull(); return base_type::count(k); }

		eastl::pair<iterator, iterator> equal_range(const key_type& k) {
 			checkNotNull();
            auto p = base_type::equal_range(k);
            return { makeIt(p.first), makeIt(p.second) };
        }

		eastl::pair<iterator_safe, iterator_safe> equal_range_safe(const key_type& k) {
 			checkNotNull();
            auto p = base_type::equal_range(k);
            return { makeSafeIt(p.first), makeSafeIt(p.second) };
        }

		eastl::pair<const_iterator, const_iterator> equal_range(const key_type& k) const {
 			checkNotNull();
            auto p = base_type::equal_range(k);
            return { makeIt(p.first), makeIt(p.second) };
        }

		eastl::pair<const_iterator_safe, const_iterator_safe> equal_range_safe(const key_type& k) const {
 			checkNotNull();
            auto p = base_type::equal_range(k);
            return { makeSafeIt(p.first), makeSafeIt(p.second) };
        }

		using base_type::validate;
		int validate_iterator(const_iterator_base it) const noexcept { return base_type::validate_iterator(it); }
		//TODO: custom validation for safe iterators
		int validate_iterator(const const_stack_only_iterator& it) const noexcept { return base_type::validate_iterator(toBase(it)); }
		int validate_iterator(const const_heap_safe_iterator& it) const noexcept { return base_type::validate_iterator(toBase(it)); }

		bool operator==(const this_type& other) const {
 			checkNotNull();
			other.checkNotNull();
			return eastl::operator==(static_cast<const base_type&>(*this), static_cast<const base_type&>(other));
		}
		bool operator!=(const this_type& other) const {
 			checkNotNull();
			other.checkNotNull();
			return eastl::operator!=(static_cast<const base_type&>(*this), static_cast<const base_type&>(other));
		}

		iterator_safe make_safe(const iterator& it) const {	return makeSafeIt(toBase(it)); }
		const_iterator_safe make_safe(const const_iterator& it) const {	return makeSafeIt(toBase(it)); }

    protected:
		[[noreturn]] static void ThrowRangeException() { throw nodecpp::error::out_of_range; }
		[[noreturn]] static void ThrowNullException() { throw nodecpp::error::zero_pointer_access; }

		void checkNotNull() const {
			if constexpr (is_safe == memory_safety::safe) {
				if (!base_type::mpBucketArray)
					ThrowNullException();
			}
		}

		void checkBucketIx(size_type n) {
			if constexpr (is_safe == memory_safety::safe) {
				if(n >= base_type::mnBucketCount)
					ThrowRangeException();
			}
		}


		const iterator_base& toBase(const iterator_base& it) const { return it; }
		const const_iterator_base& toBase(const const_iterator_base& it) const { return it; }
		const iterator_base& toBase(const stack_only_iterator& it) const { return it.toBase(); }
		const const_iterator_base& toBase(const const_stack_only_iterator& it) const { return it.toBase(); }
		iterator_base toBase(const heap_safe_iterator& it) const { return it.toBase(); }
		const_iterator_base toBase(const const_heap_safe_iterator& it) const { return it.toBase(); }

        
		iterator makeIt(const iterator_base& it) const {
			if constexpr(use_base_iterator)
				return it;
			else
				return iterator::fromBase(it);
        }

        const_iterator makeIt(const const_iterator_base& it) const {
			if constexpr(use_base_iterator)
				return it;
			else
				return const_iterator::fromBase(it);
        }

        insert_return_type makeIt(const insert_return_type_base& r) const {
			if constexpr(use_base_iterator)
				return r;
			else
	            return { makeIt(r.first), r.second };
        }

        iterator_safe makeSafeIt(const iterator_base& it) const {
			return iterator_safe::makeIt(it, base_type::mpBucketArray, base_type::mnBucketCount);
        }

        const_iterator_safe makeSafeIt(const const_iterator_base& it) const {
			return const_iterator_safe::makeIt(it, base_type::mpBucketArray, base_type::mnBucketCount);
        }

        insert_return_type_safe makeSafeIt(const insert_return_type_base& r) const {
			return { makeSafeIt(r.first), r.second };
        }

		local_iterator makeLocalIt(const local_iterator_base& it) const {
			if constexpr(use_base_iterator)
				return it;
			else
				return local_iterator::fromBase(it);
        }

        const_local_iterator makeLocalIt(const const_local_iterator_base& it) const {
			if constexpr(use_base_iterator)
				return it;
			else
				return const_local_iterator::fromBase(it);
        }

		
	}; // unordered_map


	// unordered_map_safe is kind of wrapper that forwards calls to their 'safe' counterpart.
	// i.e. 'begin' -> 'begin_safe', 'end' -> 'end_safe' and so and so.
	// this is useful for benchmarks and for tests
 	template <typename Key, typename T, typename Hash = hash<Key>, typename Predicate = equal_to<Key>, 
			  memory_safety Safety = safeness_declarator<Key>::is_safe>
	class SAFEMEMORY_DEEP_CONST_WHEN_PARAMS unordered_map_safe
		: public unordered_map<Key, T, Hash, Predicate, Safety>
	{
	public:
		typedef unordered_map<Key, T, Hash, Predicate, Safety>                    base_type;
		typedef unordered_map_safe<Key, T, Hash, Predicate, Safety>               this_type;
		using typename base_type::size_type;
		using typename base_type::key_type;
		using typename base_type::mapped_type;
		using typename base_type::value_type;
		// typedef typename base_type::allocator_type                                allocator_type;
		// typedef typename base_type::node_type                                     node_type;

		typedef typename base_type::iterator_safe                                 iterator;
		typedef typename base_type::const_iterator_safe                           const_iterator;
		using typename base_type::local_iterator;
		using typename base_type::const_local_iterator;
		typedef typename base_type::insert_return_type_safe                       insert_return_type;


	public:
		explicit unordered_map_safe(): base_type() {}
	 	explicit unordered_map_safe(size_type nBucketCount, const Hash& hashFunction = Hash(), 
						  const Predicate& predicate = Predicate())
			: base_type(nBucketCount, hashFunction, predicate) {}
		unordered_map_safe(const this_type& x) = default;
		unordered_map_safe(this_type&& x) = default;
		unordered_map_safe(std::initializer_list<value_type> ilist, size_type nBucketCount = 0, const Hash& hashFunction = Hash(), 
				   const Predicate& predicate = Predicate())
			: base_type(ilist, nBucketCount, hashFunction, predicate) {}

		this_type& operator=(const this_type& x) = default;
		this_type& operator=(this_type&& x) = default;
		this_type& operator=(std::initializer_list<value_type> ilist) { 
			base_type::operator=(ilist); return *this;
		}

		// using base_type::swap;

		iterator       begin() { return base_type::begin_safe(); }
		const_iterator begin() const { return base_type::begin_safe(); }
		const_iterator cbegin() const { return base_type::cbegin_safe(); }

		iterator       end() { return base_type::end_safe(); }
		const_iterator end() const { return base_type::end_safe(); }
		const_iterator cend() const { return base_type::cend_safe(); }

		// local_iterator begin(size_type n) noexcept { return base_type::begin(n); }
		// const_local_iterator begin(size_type n) const noexcept { return base_type::begin(n); }
		// const_local_iterator cbegin(size_type n) const noexcept { return base_type::cbegin(n); }

		// local_iterator end(size_type n) noexcept { return base_type::end(n); }
		// const_local_iterator end(size_type n) const noexcept { return base_type::end(n); }
		// const_local_iterator cend(size_type n) const noexcept { return base_type::cend(n); }

        // using base_type::at;
        // using base_type::operator[];

        // using base_type::empty;
        // using base_type::size;
        // using base_type::bucket_count;
        // using base_type::bucket_size;

//        using base_type::bucket;
        // using base_type::load_factor;
        // using base_type::get_max_load_factor;
        // using base_type::set_max_load_factor;
        // using base_type::rehash_policy;

		template <class... Args>
		insert_return_type emplace(Args&&... args) {
			return base_type::emplace_safe(std::forward<Args>(args)...);
		}

		template <class... Args>
		iterator emplace_hint(const_iterator hint, Args&&... args) {
            return base_type::emplace_hint_safe(hint, std::forward<Args>(args)...);
        }

		template <class... Args>
        insert_return_type try_emplace(const key_type& k, Args&&... args) {
            return base_type::try_emplace_safe(k, std::forward<Args>(args)...);
        }

		template <class... Args>
        insert_return_type try_emplace(key_type&& k, Args&&... args) {
            return base_type::try_emplace_safe(std::move(k), std::forward<Args>(args)...);
        }

		template <class... Args> 
        iterator try_emplace(const_iterator hint, const key_type& k, Args&&... args) {
            return base_type::try_emplace_safe(hint, k, std::forward<Args>(args)...);
        }

		template <class... Args>
        iterator try_emplace(const_iterator hint, key_type&& k, Args&&... args) {
            return base_type::try_emplace_safe(hint, std::move(k), std::forward<Args>(args)...);
        }

		insert_return_type insert(const value_type& value) {
            return base_type::insert_safe(value);
        }

		insert_return_type insert(value_type&& value) {
            return base_type::insert_safe(std::move(value));
        }

		iterator insert(const_iterator hint, const value_type& value) {
            return base_type::insert_safe(hint, value);
        }

		iterator insert(const_iterator hint, value_type&& value) {
            return base_type::insert_safe(hint, std::move(value));
        }

		void insert(std::initializer_list<value_type> ilist) { base_type::insert(ilist); }

		// template <typename InputIterator>
        // void insert_unsafe(InputIterator first, InputIterator last) {
		// 	base_type::insert_unsafe(first, last);
		// }

		template <class M>
        insert_return_type insert_or_assign(const key_type& k, M&& obj) {
            return base_type::insert_or_assign_safe(k, std::forward<M>(obj));
        }

		template <class M>
        insert_return_type insert_or_assign(key_type&& k, M&& obj) {
            return base_type::insert_or_assign_safe(std::move(k), std::forward<M>(obj));
        }

		template <class M>
        iterator insert_or_assign(const_iterator hint, const key_type& k, M&& obj) {
            return base_type::insert_or_assign_safe(hint, k, std::forward<M>(obj));
        }

		template <class M>
        iterator insert_or_assign(const_iterator hint, key_type&& k, M&& obj) {
            return base_type::insert_or_assign_safe(hint, std::move(k), std::forward<M>(obj));
        }

		iterator erase(const_iterator position) { return base_type::erase_safe(position); }
		iterator erase(const_iterator first, const_iterator last) { return base_type::erase_safe(first, last); }
		size_type erase(const key_type& k) { return base_type::erase(k); }

        // using base_type::clear;
        // using base_type::rehash;
        // using base_type::reserve;

		iterator       find(const key_type& key) { return base_type::find_safe(key); }
		const_iterator find(const key_type& key) const { return base_type::find_safe(key); }

        // using base_type::count;

		eastl::pair<iterator, iterator> equal_range(const key_type& k) { return base_type::equal_range_safe(k); }
		eastl::pair<const_iterator, const_iterator> equal_range(const key_type& k) const { return base_type::equal_range_safe(k); }

		// using base_type::validate;
		// using base_type::validate_iterator;

		// using base_type::operator==;
	}; // unordered_map_safe


	template <typename Key, typename T, typename Hash = hash<Key>, typename Predicate = equal_to<Key>, 
			  memory_safety Safety = safeness_declarator<Key>::is_safe>
	class SAFEMEMORY_DEEP_CONST_WHEN_PARAMS unordered_multimap
		: private eastl::unordered_multimap<Key, T, Hash, Predicate, detail::allocator_to_eastl_hashtable<Safety>>
	{
	public:
		typedef eastl::unordered_multimap<Key, T, Hash, Predicate, detail::allocator_to_eastl_hashtable<Safety>> base_type;
		typedef unordered_multimap<Key, T, Hash, Predicate, Safety>                    this_type;
		typedef typename base_type::size_type                                     size_type;
		typedef typename base_type::key_type                                      key_type;
		typedef typename base_type::mapped_type                                   mapped_type;
		typedef typename base_type::value_type                                    value_type;     // NOTE: 'value_type = pair<const key_type, mapped_type>'.
		typedef typename base_type::allocator_type                                allocator_type;
		typedef typename base_type::node_type                                     node_type;
		typedef typename base_type::iterator                                      iterator_base;
		typedef typename base_type::const_iterator                                const_iterator_base;
		typedef typename base_type::local_iterator                                local_iterator_base;
		typedef typename base_type::const_local_iterator                          const_local_iterator_base;
		typedef typename base_type::insert_return_type                            insert_return_type_base;

		typedef typename detail::hashtable_stack_only_iterator<iterator_base, iterator_base, allocator_type>       stack_only_iterator;
		typedef typename detail::hashtable_stack_only_iterator<const_iterator_base, iterator_base, allocator_type>  const_stack_only_iterator;
		typedef typename detail::hashtable_heap_safe_iterator2<iterator_base, iterator_base, allocator_type>        heap_safe_iterator;
		typedef typename detail::hashtable_heap_safe_iterator2<const_iterator_base, iterator_base, allocator_type>   const_heap_safe_iterator;

		typedef typename detail::hashtable_stack_only_iterator<local_iterator_base, local_iterator_base, allocator_type>       stack_only_local_iterator;
		typedef typename detail::hashtable_stack_only_iterator<const_local_iterator_base, local_iterator_base, allocator_type>  const_stack_only_local_iterator;

	    static constexpr memory_safety is_safe = allocator_type::is_safe;
		static constexpr bool use_base_iterator = is_safe == memory_safety::none;
		
		typedef std::conditional_t<use_base_iterator, iterator_base, stack_only_iterator>               iterator;
		typedef std::conditional_t<use_base_iterator, const_iterator_base, const_stack_only_iterator>   const_iterator;
		typedef iterator                                                          insert_return_type;

		typedef heap_safe_iterator                                                    iterator_safe;
		typedef const_heap_safe_iterator                                              const_iterator_safe;
		typedef iterator_safe                                                     insert_return_type_safe;

		typedef std::conditional_t<use_base_iterator, local_iterator_base, stack_only_local_iterator>               local_iterator;
		typedef std::conditional_t<use_base_iterator, const_local_iterator_base, const_stack_only_local_iterator>   const_local_iterator;


	public:
		explicit unordered_multimap(): base_type(allocator_type()) {}
	 	explicit unordered_multimap(size_type nBucketCount, const Hash& hashFunction = Hash(), 
						  const Predicate& predicate = Predicate())
			: base_type(nBucketCount, hashFunction, predicate, allocator_type())
		    {}
		unordered_multimap(const this_type& x) = default;
		unordered_multimap(this_type&& x) = default;
		unordered_multimap(std::initializer_list<value_type> ilist, size_type nBucketCount = 0, const Hash& hashFunction = Hash(), 
				   const Predicate& predicate = Predicate())
			: base_type(ilist, nBucketCount, hashFunction, predicate, allocator_type())
            {}

		this_type& operator=(const this_type& x) = default;
		this_type& operator=(this_type&& x) = default;
		this_type& operator=(std::initializer_list<value_type> ilist) {
			checkNotNull();
			base_type::operator=(ilist);
			return *this;
		}

		void swap(this_type& x) noexcept { base_type::swap(x); }

		iterator       begin() { checkNotNull(); return makeIt(base_type::begin()); }
		const_iterator begin() const { checkNotNull(); return makeIt(base_type::begin()); }
		const_iterator cbegin() const { checkNotNull(); return makeIt(base_type::cbegin()); }

		iterator       end() { checkNotNull(); return makeIt(base_type::end()); }
		const_iterator end() const { checkNotNull(); return makeIt(base_type::end()); }
		const_iterator cend() const { checkNotNull(); return makeIt(base_type::cend()); }

		iterator_safe       begin_safe() { checkNotNull(); return makeSafeIt(base_type::begin()); }
		const_iterator_safe begin_safe() const { checkNotNull(); return makeSafeIt(base_type::begin()); }
		const_iterator_safe cbegin_safe() const { checkNotNull(); return makeSafeIt(base_type::cbegin()); }

		iterator_safe       end_safe() { checkNotNull(); return makeSafeIt(base_type::end()); }
		const_iterator_safe end_safe() const { checkNotNull(); return makeSafeIt(base_type::end()); }
		const_iterator_safe cend_safe() const { checkNotNull(); return makeSafeIt(base_type::cend()); }

		local_iterator begin(size_type n) { checkNotNull(); checkBucketIx(n); return makeLocalIt(base_type::begin(n)); }
		const_local_iterator begin(size_type n) const { checkNotNull(); checkBucketIx(n); return makeLocalIt(base_type::begin(n)); }
		const_local_iterator cbegin(size_type n) const { checkNotNull(); checkBucketIx(n); return makeLocalIt(base_type::cbegin(n)); }

		local_iterator end(size_type n) noexcept { checkNotNull(); checkBucketIx(n); return makeLocalIt(base_type::end(n)); }
		const_local_iterator end(size_type n) const noexcept { checkNotNull(); checkBucketIx(n); return makeLocalIt(base_type::end(n)); }
		const_local_iterator cend(size_type n) const noexcept { checkNotNull(); checkBucketIx(n); return makeLocalIt(base_type::cend(n)); }

		// T& at(const key_type& k) { checkNotNull(); return base_type::at(k); }
		// const T& at(const key_type& k) const { checkNotNull(); return base_type::at(k); }
		// mapped_type& operator[](const key_type& key) { checkNotNull(); return base_type::operator[](key); }
		// mapped_type& operator[](key_type&& key) { checkNotNull(); return base_type::operator[](std::move(key)); }

        using base_type::empty;
        using base_type::size;
        using base_type::bucket_count;
		size_type bucket_size(size_type n) const {
			checkNotNull();
			checkBucketIx(n);
			return base_type::bucket_size(n);
		}

		float load_factor() const { checkNotNull(); return base_type::load_factor(); }
		float get_max_load_factor() const { checkNotNull(); return base_type::get_max_load_factor(); }
		void set_max_load_factor(float fMaxLoadFactor) { checkNotNull(); base_type::set_max_load_factor(fMaxLoadFactor); }

		template <class... Args>
		insert_return_type emplace(Args&&... args) {
			checkNotNull();
            return makeIt(base_type::emplace(std::forward<Args>(args)...));
        }

		template <class... Args>
		insert_return_type_safe emplace_safe(Args&&... args) {
			checkNotNull();
            return makeSafeIt(base_type::emplace(std::forward<Args>(args)...));
        }

		template <class... Args>
		iterator emplace_hint(const const_iterator& hint, Args&&... args) {
			checkNotNull();
            return makeIt(base_type::emplace_hint(toBase(hint), std::forward<Args>(args)...));
        }

		template <class... Args>
		iterator_safe emplace_hint_safe(const const_iterator_safe& hint, Args&&... args) {
			checkNotNull();
            return makeSafeIt(base_type::emplace_hint(toBase(hint), std::forward<Args>(args)...));
        }

		template <class... Args>
        insert_return_type try_emplace(const key_type& k, Args&&... args) {
			checkNotNull();
            return makeIt(base_type::try_emplace(k, std::forward<Args>(args)...));
        }

		template <class... Args>
        insert_return_type_safe try_emplace_safe(const key_type& k, Args&&... args) {
			checkNotNull();
            return makeSafeIt(base_type::try_emplace(k, std::forward<Args>(args)...));
        }

		template <class... Args>
        insert_return_type try_emplace(key_type&& k, Args&&... args) {
			checkNotNull();
            return makeIt(base_type::try_emplace(std::move(k), std::forward<Args>(args)...));
        }

		template <class... Args>
        insert_return_type_safe try_emplace_safe(key_type&& k, Args&&... args) {
			checkNotNull();
            return makeSafeIt(base_type::try_emplace(std::move(k), std::forward<Args>(args)...));
        }

		template <class... Args> 
        iterator try_emplace(const const_iterator& hint, const key_type& k, Args&&... args) {
			checkNotNull();
            return makeIt(base_type::try_emplace(toBase(hint), k, std::forward<Args>(args)...));
        }

		template <class... Args> 
        iterator_safe try_emplace_safe(const const_iterator_safe& hint, const key_type& k, Args&&... args) {
			checkNotNull();
            return makeSafeIt(base_type::try_emplace(toBase(hint), k, std::forward<Args>(args)...));
        }

		template <class... Args>
        iterator try_emplace(const const_iterator& hint, key_type&& k, Args&&... args) {
			checkNotNull();
            return makeIt(base_type::try_emplace(toBase(hint), std::move(k), std::forward<Args>(args)...));
        }

		template <class... Args>
        iterator_safe try_emplace_safe(const const_iterator_safe& hint, key_type&& k, Args&&... args) {
			checkNotNull();
            return makeSafeIt(base_type::try_emplace(toBase(hint), std::move(k), std::forward<Args>(args)...));
        }

		insert_return_type insert(const value_type& value) {
			checkNotNull();
            return makeIt(base_type::insert(value));
        }

		insert_return_type_safe insert_safe(const value_type& value) {
			checkNotNull();
            return makeSafeIt(base_type::insert(value));
        }

		insert_return_type insert(value_type&& value) {
			checkNotNull();
            return makeIt(base_type::insert(std::move(value)));
        }

		insert_return_type_safe insert_safe(value_type&& value) {
			checkNotNull();
            return makeSafeIt(base_type::insert(std::move(value)));
        }

		iterator insert(const const_iterator& hint, const value_type& value) {
			checkNotNull();
            return makeIt(base_type::insert(toBase(hint), value));
        }

		iterator_safe insert_safe(const const_iterator_safe& hint, const value_type& value) {
			checkNotNull();
            return makeSafeIt(base_type::insert(toBase(hint), value));
        }

		iterator insert(const const_iterator& hint, value_type&& value) {
			checkNotNull();
            return makeIt(base_type::insert(toBase(hint), std::move(value)));
        }

		iterator_safe insert_safe(const const_iterator_safe& hint, value_type&& value) {
			checkNotNull();
            return makeIt(base_type::insert(toBase(hint), std::move(value)));
        }

		void insert(std::initializer_list<value_type> ilist) {
			checkNotNull();
            base_type::insert(ilist);
        }

		template <typename InputIterator>
        void insert_unsafe(InputIterator first, InputIterator last) {
			checkNotNull();
            base_type::insert(first, last);
        }

		template <class M>
        insert_return_type insert_or_assign(const key_type& k, M&& obj) {
			checkNotNull();
            return makeIt(base_type::insert_or_assign(k, std::forward<M>(obj)));
        }

		template <class M>
        insert_return_type_safe insert_or_assign_safe(const key_type& k, M&& obj) {
			checkNotNull();
            return makeSafeIt(base_type::insert_or_assign(k, std::forward<M>(obj)));
        }

		template <class M>
        insert_return_type insert_or_assign(key_type&& k, M&& obj) {
			checkNotNull();
            return makeIt(base_type::insert_or_assign(std::move(k), std::forward<M>(obj)));
        }

		template <class M>
        insert_return_type_safe insert_or_assign_safe(key_type&& k, M&& obj) {
			checkNotNull();
            return makeSafeIt(base_type::insert_or_assign(std::move(k), std::forward<M>(obj)));
        }

		template <class M>
        iterator insert_or_assign(const const_iterator& hint, const key_type& k, M&& obj) {
			checkNotNull();
            return makeIt(base_type::insert_or_assign(toBase(hint), k, std::forward<M>(obj)));
        }

		template <class M>
        iterator_safe insert_or_assign_safe(const const_iterator_safe& hint, const key_type& k, M&& obj) {
			checkNotNull();
            return makeSafeIt(base_type::insert_or_assign(toBase(hint), k, std::forward<M>(obj)));
        }

		template <class M>
        iterator insert_or_assign(const const_iterator& hint, key_type&& k, M&& obj) {
			checkNotNull();
            return makeIt(base_type::insert_or_assign(toBase(hint), std::move(k), std::forward<M>(obj)));
        }

		template <class M>
        iterator_safe insert_or_assign_safe(const const_iterator_safe& hint, key_type&& k, M&& obj) {
			checkNotNull();
            return makeSafeIt(base_type::insert_or_assign(toBase(hint), std::move(k), std::forward<M>(obj)));
        }

		iterator erase(const const_iterator& position) {
			checkNotNull();
            return makeIt(base_type::erase(toBase(position)));
        }

		iterator_safe erase_safe(const const_iterator_safe& position) {
			checkNotNull();
            return makeSafeIt(base_type::erase(toBase(position)));
        }

		iterator erase(const const_iterator& first, const const_iterator& last) {
			checkNotNull();
            return makeIt(base_type::erase(toBase(first), toBase(last)));
        }

		iterator_safe erase_safe(const const_iterator_safe& first, const const_iterator_safe& last) {
			checkNotNull();
            return makeSafeIt(base_type::erase(toBase(first), toBase(last)));
        }

		size_type erase(const key_type& k) {
			checkNotNull();
			return base_type::erase(k);
		}

        void clear() { checkNotNull(); base_type::clear(); }
        void rehash(size_type nBucketCount) { checkNotNull(); base_type::rehash(nBucketCount); }
        void reserve(size_type nElementCount) { checkNotNull(); base_type::reserve(nElementCount); }

		iterator       find(const key_type& key) { checkNotNull(); return makeIt(base_type::find(key)); }
		iterator_safe       find_safe(const key_type& key) { checkNotNull(); return makeSafeIt(base_type::find(key)); }

		const_iterator find(const key_type& key) const { checkNotNull(); return makeIt(base_type::find(key)); }
		const_iterator_safe find_safe(const key_type& key) const { checkNotNull(); return makeSafeIt(base_type::find(key)); }

		size_type count(const key_type& k) const { checkNotNull(); return base_type::count(k); }

		eastl::pair<iterator, iterator> equal_range(const key_type& k) {
 			checkNotNull();
            auto p = base_type::equal_range(k);
            return { makeIt(p.first), makeIt(p.second) };
        }

		eastl::pair<iterator_safe, iterator_safe> equal_range_safe(const key_type& k) {
 			checkNotNull();
            auto p = base_type::equal_range(k);
            return { makeSafeIt(p.first), makeSafeIt(p.second) };
        }

		eastl::pair<const_iterator, const_iterator> equal_range(const key_type& k) const {
 			checkNotNull();
            auto p = base_type::equal_range(k);
            return { makeIt(p.first), makeIt(p.second) };
        }

		eastl::pair<const_iterator_safe, const_iterator_safe> equal_range_safe(const key_type& k) const {
 			checkNotNull();
            auto p = base_type::equal_range(k);
            return { makeSafeIt(p.first), makeSafeIt(p.second) };
        }

		using base_type::validate;
		int validate_iterator(const_iterator_base it) const noexcept { return base_type::validate_iterator(it); }
		//TODO: custom validation for safe iterators
		int validate_iterator(const const_stack_only_iterator& it) const noexcept { return base_type::validate_iterator(toBase(it)); }
		int validate_iterator(const const_heap_safe_iterator& it) const noexcept { return base_type::validate_iterator(toBase(it)); }

		bool operator==(const this_type& other) const {
 			checkNotNull();
			other.checkNotNull();
			return eastl::operator==(static_cast<const base_type&>(*this), static_cast<const base_type&>(other));
		}
		bool operator!=(const this_type& other) const {
 			checkNotNull();
			other.checkNotNull();
			return eastl::operator!=(static_cast<const base_type&>(*this), static_cast<const base_type&>(other));
		}

		iterator_safe make_safe(const iterator& it) const {	return makeSafeIt(toBase(it)); }
		const_iterator_safe make_safe(const const_iterator& it) const {	return makeSafeIt(toBase(it)); }

    protected:
		[[noreturn]] static void ThrowRangeException() { throw nodecpp::error::out_of_range; }
		[[noreturn]] static void ThrowNullException() { throw nodecpp::error::zero_pointer_access; }

		void checkNotNull() const {
			if constexpr (is_safe == memory_safety::safe) {
				if (!base_type::mpBucketArray)
					ThrowNullException();
			}
		}

		void checkBucketIx(size_type n) {
			if constexpr (is_safe == memory_safety::safe) {
				if(n >= base_type::mnBucketCount)
					ThrowRangeException();
			}
		}


		const iterator_base& toBase(const iterator_base& it) const { return it; }
		const const_iterator_base& toBase(const const_iterator_base& it) const { return it; }
		const iterator_base& toBase(const stack_only_iterator& it) const { return it.toBase(); }
		const const_iterator_base& toBase(const const_stack_only_iterator& it) const { return it.toBase(); }
		iterator_base toBase(const heap_safe_iterator& it) const { return it.toBase(); }
		const_iterator_base toBase(const const_heap_safe_iterator& it) const { return it.toBase(); }

        
		iterator makeIt(const iterator_base& it) const {
			if constexpr(use_base_iterator)
				return it;
			else
				return iterator::fromBase(it);
        }

        const_iterator makeIt(const const_iterator_base& it) const {
			if constexpr(use_base_iterator)
				return it;
			else
				return const_iterator::fromBase(it);
        }

        // insert_return_type makeIt(const insert_return_type_base& r) const {
		// 	if constexpr(use_base_iterator)
		// 		return r;
		// 	else
	    //         return { makeIt(r.first), r.second };
        // }

        iterator_safe makeSafeIt(const iterator_base& it) const {
			return iterator_safe::makeIt(it, base_type::mpBucketArray, base_type::mnBucketCount);
        }

        const_iterator_safe makeSafeIt(const const_iterator_base& it) const {
			return const_iterator_safe::makeIt(it, base_type::mpBucketArray, base_type::mnBucketCount);
        }

        // insert_return_type_safe makeSafeIt(const insert_return_type_base& r) const {
		// 	return { makeSafeIt(r.first), r.second };
        // }

		local_iterator makeLocalIt(const local_iterator_base& it) const {
			if constexpr(use_base_iterator)
				return it;
			else
				return local_iterator::fromBase(it);
        }

        const_local_iterator makeLocalIt(const const_local_iterator_base& it) const {
			if constexpr(use_base_iterator)
				return it;
			else
				return const_local_iterator::fromBase(it);
        }
	}; // unordered_multimap

	///////////////////////////////////////////////////////////////////////
	// global operators
	///////////////////////////////////////////////////////////////////////

	template <typename K, typename T, typename H, typename P, memory_safety S>
	inline void swap(const unordered_map<K, T, H, P, S>& a, 
					 const unordered_map<K, T, H, P, S>& b)
	{
		a.swap(b);
	}

	template <typename K, typename T, typename H, typename P, memory_safety S>
	inline void swap(const unordered_multimap<K, T, H, P, S>& a, 
					 const unordered_multimap<K, T, H, P, S>& b)
	{
		a.swap(b);
	}


} // namespace safememory


#endif //SAFE_MEMORY_UNORDERED_MAP_H

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

#ifndef SAFE_MEMORY_UNORDERED_SET_H
#define SAFE_MEMORY_UNORDERED_SET_H

#include <utility>
#include <EASTL/unordered_set.h>
#include <safe_memory/detail/allocator_to_eastl.h>
#include <safe_memory/detail/hashtable_iterator.h>


namespace safe_memory
{


	template <typename Value, typename Hash = eastl::hash<Value>, typename Predicate = eastl::equal_to<Value>, 
			  memory_safety Safety = safeness_declarator<Value>::is_safe>
	class SAFE_MEMORY_DEEP_CONST_WHEN_PARAMS unordered_set
		: private eastl::unordered_set<Value, Hash, Predicate, detail::allocator_to_eastl<Safety>>
	{
	public:
		typedef eastl::unordered_set<Value, Hash, Predicate,
                                        detail::allocator_to_eastl<Safety>>       base_type;
		typedef unordered_set<Value, Hash, Predicate, Safety>                     this_type;
		typedef typename base_type::size_type                                     size_type;
		typedef typename base_type::key_type                                      key_type;
		// typedef T                                                                 mapped_type;
		typedef typename base_type::value_type                                    value_type;     // NOTE: 'value_type = pair<const key_type, mapped_type>'.
		typedef typename base_type::allocator_type                                allocator_type;
		typedef typename base_type::node_type                                     node_type;
		typedef typename base_type::iterator                                      base_iterator;
		typedef typename base_type::const_iterator                                const_base_iterator;
		typedef typename base_type::local_iterator                                local_iterator;
		typedef typename base_type::const_local_iterator                          const_local_iterator;

		// typedef typename detail::hashtable_heap_safe_iterator<base_iterator, base_iterator, Safety>        heap_safe_iterator;
		typedef typename detail::hashtable_heap_safe_iterator<const_base_iterator, base_iterator, Safety>   const_heap_safe_iterator;
		// typedef typename detail::hashtable_stack_only_iterator<base_iterator, base_iterator, Safety>       stack_only_iterator;
		typedef typename detail::hashtable_stack_only_iterator<const_base_iterator, base_iterator, Safety>  const_stack_only_iterator;

		static constexpr bool default_iterator_is_heap_safe = false;
		typedef eastl::type_select_t<default_iterator_is_heap_safe,
			const_heap_safe_iterator, const_stack_only_iterator>                  const_iterator;
		typedef const_iterator                                                    iterator;

		typedef typename eastl::pair<iterator, bool>                              insert_return_type;
		typedef const_iterator                                             const_cit_ref;

        typedef typename base_type::bucket_array_type                             bucket_array_type;

	public:
		explicit unordered_set(): base_type(allocator_type()) {}
		explicit unordered_set(size_type nBucketCount, const Hash& hashFunction = Hash(), 
						  const Predicate& predicate = Predicate())
			: base_type(nBucketCount, hashFunction, predicate, allocator_type())
		    {}
		unordered_set(const this_type& x) = default;
		unordered_set(this_type&& x) = default;
		unordered_set(std::initializer_list<value_type> ilist, size_type nBucketCount = 0, const Hash& hashFunction = Hash(), 
				   const Predicate& predicate = Predicate())
			: base_type(ilist, nBucketCount, hashFunction, predicate, allocator_type())
            {}

		this_type& operator=(const this_type& x) = default;
		this_type& operator=(this_type&& x) = default;
		this_type& operator=(std::initializer_list<value_type> ilist)
            { return static_cast<this_type&>(base_type::operator=(ilist)); }

		void swap(this_type& x) { base_type::swap(x); }

		iterator       begin() noexcept { return makeSafeIt(base_type::begin()); }
		const_iterator begin() const noexcept { return makeSafeIt(base_type::begin()); }
		const_iterator cbegin() const noexcept { return makeSafeIt(base_type::cbegin()); }

		iterator       end() noexcept { return makeSafeIt(base_type::end()); }
		const_iterator end() const noexcept { return makeSafeIt(base_type::end()); }
		const_iterator cend() const noexcept { return makeSafeIt(base_type::cend()); }

		local_iterator begin(size_type n) noexcept { return base_type::begin(n); }
		const_local_iterator begin(size_type n) const noexcept { return base_type::begin(n); }
		const_local_iterator cbegin(size_type n) const noexcept { return base_type::cbegin(n); }

		local_iterator end(size_type n) noexcept { return base_type::end(n); }
		const_local_iterator end(size_type n) const noexcept { return base_type::end(n); }
		const_local_iterator cend(size_type n) const noexcept { return base_type::cend(n); }

        // using base_type::at;
        // using base_type::operator[];

        using base_type::empty;
        using base_type::size;
        using base_type::bucket_count;
        using base_type::bucket_size;

//        using base_type::bucket;
        using base_type::load_factor;
        using base_type::get_max_load_factor;
        using base_type::set_max_load_factor;
        using base_type::rehash_policy;

		template <class... Args>
		eastl::pair<iterator, bool> emplace(Args&&... args) {
            return makeSafeIt(base_type::emplace(std::forward<Args>(args)...));
        }

		template <class... Args>
		iterator emplace_hint(const_cit_ref position, Args&&... args) {
            return makeSafeIt(base_type::emplace_hint(toBaseIt(position), std::forward<Args>(args)...));
        }

		template <class... Args>
        eastl::pair<iterator, bool> try_emplace(const key_type& k, Args&&... args) {
            return makeSafeIt(base_type::try_emplace(k, std::forward<Args>(args)...));
        }

		template <class... Args>
        eastl::pair<iterator, bool> try_emplace(key_type&& k, Args&&... args) {
            return makeSafeIt(base_type::try_emplace(std::move(k), std::forward<Args>(args)...));
        }

		template <class... Args> 
        iterator try_emplace(const_cit_ref position, const key_type& k, Args&&... args) {
            return makeSafeIt(base_type::try_emplace(toBaseIt(position), k, std::forward<Args>(args)...));
        }

		template <class... Args>
        iterator try_emplace(const_cit_ref position, key_type&& k, Args&&... args) {
            return makeSafeIt(base_type::try_emplace(toBaseIt(position), std::move(k), std::forward<Args>(args)...));
        }

		eastl::pair<iterator, bool> insert(const value_type& value) {
            return makeSafeIt(base_type::insert(value));
        }

		eastl::pair<iterator, bool> insert(value_type&& value) {
            return makeSafeIt(base_type::insert(std::move(value)));
        }

		iterator insert(const_cit_ref hint, const value_type& value) {
            return makeSafeIt(base_type::insert(toBaseIt(hint), value));
        }

		iterator insert(const_cit_ref hint, value_type&& value) {
            return makeSafeIt(base_type::insert(toBaseIt(hint), std::move(value)));
        }

		void insert(std::initializer_list<value_type> ilist) {
            base_type::insert(ilist);
        }

		template <typename InputIterator>
        void insert_unsafe(InputIterator first, InputIterator last) {
            base_type::insert(first, last);
        }

		template <class M>
        eastl::pair<iterator, bool> insert_or_assign(const key_type& k, M&& obj) {
            return makeSafeIt(base_type::insert_or_assign(k, std::forward<M>(obj)));
        }

		template <class M>
        eastl::pair<iterator, bool> insert_or_assign(key_type&& k, M&& obj) {
            return makeSafeIt(base_type::insert_or_assign(std::move(k), std::forward<M>(obj)));
        }

		template <class M>
        iterator insert_or_assign(const_cit_ref hint, const key_type& k, M&& obj) {
            return makeSafeIt(base_type::insert_or_assign(toBaseIt(hint), k, std::forward<M>(obj)));
        }

		template <class M>
        iterator insert_or_assign(const_cit_ref hint, key_type&& k, M&& obj) {
            return makeSafeIt(base_type::insert_or_assign(toBaseIt(hint), std::move(k), std::forward<M>(obj)));
        }

		iterator erase(const_cit_ref position) {
            return makeSafeIt(base_type::erase(toBaseIt(position)));
        }

		iterator erase(const_cit_ref first, const_cit_ref last) {
            return makeSafeIt(base_type::erase(toBaseIt(first), toBaseIt(last)));
        }

		size_type erase(const key_type& k) { return base_type::erase(k); }

        using base_type::clear;
        using base_type::rehash;
        using base_type::reserve;

		iterator       find(const key_type& key) { return makeSafeIt(base_type::find(key)); }
		const_iterator find(const key_type& key) const { return makeSafeIt(base_type::find(key)); }

        using base_type::count;

		eastl::pair<iterator, iterator> equal_range(const key_type& k) {
            auto p = base_type::equal_range(k);
            return { makeSafeIt(p.first), makeSafeIt(p.second) };
        }

		eastl::pair<const_iterator, const_iterator> equal_range(const key_type& k) const {
            auto p = base_type::equal_range(k);
            return { makeSafeIt(p.first), makeSafeIt(p.second) };
        }

		using base_type::validate;
		using base_type::validate_iterator;

		// bool validate() const;
		// iterator_validity  validate_iterator(const_iterator i) const;

		bool operator==(const this_type& other) const {
			return eastl::operator==(static_cast<const base_type&>(*this), static_cast<const base_type&>(other));
		}

		bool operator!=(const this_type& other) const {
			return eastl::operator!=(static_cast<const base_type&>(*this), static_cast<const base_type&>(other));
		}



		const_heap_safe_iterator make_heap_safe(const const_stack_only_iterator& it) const {
			return const_heap_safe_iterator::makeIt(allocator_type::to_soft(it.asBase().mpNode), GetHeapPtr(), it.asBase().mpBucket);
		}

		const const_heap_safe_iterator& make_heap_safe(const const_heap_safe_iterator& it) const {
			return it;
		}

    private:
        const const_base_iterator& toBaseIt(const const_stack_only_iterator& it) {
            return it.asBase();
        }

        const_base_iterator toBaseIt(const const_heap_safe_iterator& it) {
            return const_base_iterator(allocator_type::to_zero(it.mpNode), it.mpBucket.get_raw_ptr());
        }

        // iterator makeSafeIt(base_iterator it) const {
		// 	if constexpr(default_iterator_is_heap_safe)
	    //         return iterator::makeIt(it.mpNode, GetHeapPtr(), it.mpBucket);
		// 	else
		// 		return iterator::fromBase(it);
        // }

        iterator makeSafeIt(base_iterator it) const {
			if constexpr(default_iterator_is_heap_safe)
	            return iterator::makeIt(it.mpNode, GetHeapPtr(), it.mpBucket);
			else
				return iterator::fromBase(it);
        }

        eastl::pair<iterator, bool> makeSafeIt(eastl::pair<base_iterator, bool> r) const {
            return eastl::pair<iterator, bool>(makeSafeIt(r.first), r.second);
        }

        const bucket_array_type& GetHeapPtr() const {
            return base_type::mpBucketArray;
        }
	}; // unordered_set

	template <typename Value, typename Hash = eastl::hash<Value>, typename Predicate = eastl::equal_to<Value>, 
			  memory_safety Safety = safeness_declarator<Value>::is_safe>
	using unordered_multiset = unordered_set<Value, Hash, Predicate, Safety>;

} // namespace safe_memory


#endif //SAFE_MEMORY_UNORDERED_SET_H
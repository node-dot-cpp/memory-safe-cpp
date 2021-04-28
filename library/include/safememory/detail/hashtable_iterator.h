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

#ifndef SAFE_MEMORY_DETAIL_HASHTABLE_ITERATOR
#define SAFE_MEMORY_DETAIL_HASHTABLE_ITERATOR

#include <safememory/detail/instrument.h>
#include <safe_memory_error.h>

namespace safememory::detail {

	/**
	 * \brief Iterator wrapper for \c hashmap stack only iterators
	 */

	template <typename BaseIt, typename BaseNonConstIt, typename Allocator>
	class hashtable_stack_only_iterator : protected BaseIt
	{
	public:
		typedef BaseIt                                                   base_type;
		typedef Allocator                                                allocator_type;
		typedef hashtable_stack_only_iterator<BaseIt, BaseNonConstIt, Allocator>    this_type;
		typedef hashtable_stack_only_iterator<BaseNonConstIt, BaseNonConstIt, Allocator>     this_type_non_const;

		typedef typename base_type::node_type                            node_type;
		typedef typename base_type::value_type                           value_type;
		typedef typename base_type::pointer                              pointer;
		typedef typename base_type::reference                            reference;
		typedef typename base_type::difference_type                      difference_type;
		typedef typename base_type::iterator_category                    iterator_category;

	    static constexpr memory_safety is_safe = allocator_type::is_safe;

	    static constexpr bool is_const = !std::is_same_v<this_type, this_type_non_const>;

		template <typename, typename, typename>
		friend class hashtable_stack_only_iterator;

		template<typename TT>
		static constexpr bool sfinae = is_const && std::is_same_v<TT, this_type_non_const>;

		// typedef typename allocator_type::template pointer_types<node_type>::pointer   node_pointer;

		[[noreturn]] static void ThrowRangeException() { throw nodecpp::error::out_of_range; }
		[[noreturn]] static void ThrowNullException() { throw nodecpp::error::zero_pointer_access; }


    public:
		hashtable_stack_only_iterator() :base_type() { }

		hashtable_stack_only_iterator(const this_type&) = default;
		hashtable_stack_only_iterator& operator=(const hashtable_stack_only_iterator& ri) = default;

		hashtable_stack_only_iterator(hashtable_stack_only_iterator&& ri) = default; 
		hashtable_stack_only_iterator& operator=(hashtable_stack_only_iterator&& ri) = default;

		template<typename Other, std::enable_if_t<sfinae<Other>, bool> = true>
		hashtable_stack_only_iterator(const Other& other)
			: base_type(other) { }

		template<typename Other, std::enable_if_t<sfinae<Other>, bool> = true>
		hashtable_stack_only_iterator& operator=(const Other& other) {
			base_type::operator=(static_cast<const typename Other::base_type&>(other));
			return *this;
		}

		reference operator*() const {
			checkDerefenceable();
			return base_type::operator*();
		}

		pointer operator->() const {
			checkDerefenceable();
			return base_type::operator->();
		}

		this_type& operator++() {
			checkDerefenceable();
			base_type::operator++();
			return *this;
		}

		this_type operator++(int) {
			this_type temp(*this);
			operator++();
			return temp;
		}

		bool operator==(const this_type& other) const {
			return eastl::operator==(static_cast<const base_type&>(*this), static_cast<const base_type&>(other));
		}
		bool operator!=(const this_type& other) const {
			return eastl::operator!=(static_cast<const base_type&>(*this), static_cast<const base_type&>(other));
		}

		void checkDerefenceable() const {
			if(NODECPP_UNLIKELY(!base_type::mpNode))
				ThrowNullException();
			else if(NODECPP_UNLIKELY(allocator_type::is_hashtable_sentinel(base_type::mpNode)))
				ThrowRangeException();

#ifdef SAFEMEMORY_DEZOMBIEFY_ITERATORS
			checkNotZombie(base_type::mpBucket);
			checkNotZombie(allocator_type::to_raw(base_type::mpNode));
#endif
		}


		const base_type& toBase() const {
			// base iterator can't be null
			if(NODECPP_UNLIKELY(!base_type::mpNode))
				ThrowNullException();
			
			return *this;
		}

		static this_type& fromBase(base_type& b) { return static_cast<this_type&>(b); }
		static const this_type& fromBase(const base_type& b) { return static_cast<const this_type&>(b); }
	}; // hashtable_stack_only_iterator


	/**
	 * \brief Iterator for \c hashmap heap safe iterators
	 */
	template <typename BaseIt, typename BaseNonConstIt, typename Allocator>
	class hashtable_heap_safe_iterator2 : protected BaseIt
	{
	public:
		typedef BaseIt                                                   base_type;
		typedef Allocator                                                allocator_type;
		typedef hashtable_heap_safe_iterator2<BaseIt, BaseNonConstIt, Allocator>    this_type;
		typedef hashtable_heap_safe_iterator2<BaseNonConstIt, BaseNonConstIt, Allocator>     this_type_non_const;

		typedef typename base_type::node_type                            node_type;
		typedef typename base_type::value_type                           value_type;
		typedef typename base_type::pointer                              pointer;
		typedef typename base_type::reference                            reference;
		typedef typename base_type::difference_type                      difference_type;
		typedef typename base_type::iterator_category                    iterator_category;

	    static constexpr memory_safety is_safe = allocator_type::is_safe;

	    static constexpr bool is_const = !std::is_same_v<this_type, this_type_non_const>;

		template <typename, typename, typename>
		friend class hashtable_heap_safe_iterator2;

		template<typename TT>
		static constexpr bool sfinae = is_const && std::is_same_v<TT, this_type_non_const>;

		// typedef typename allocator_type::template pointer_types<node_type>::pointer   node_pointer;

		typedef typename allocator_type::template pointer<node_type>                  zero_node_ptr;
		typedef typename allocator_type::template array_pointer<zero_node_ptr>        zero_bucket_arr;
        typedef typename allocator_type::template soft_pointer<node_type>             soft_node_ptr;
		typedef typename allocator_type::template soft_array_pointer<zero_node_ptr>   soft_bucket_arr;

		soft_bucket_arr  mpSoftBucketArr;
		soft_node_ptr    mpSoftNode;

		[[noreturn]] static void ThrowRangeException() { throw nodecpp::error::out_of_range; }
		[[noreturn]] static void ThrowNullException() { throw nodecpp::error::zero_pointer_access; }

		// used on empty hashtable (always end)
		hashtable_heap_safe_iterator2(const BaseIt& it)
			: base_type(it), mpSoftNode(), mpSoftBucketArr() { }

		// used on end of any hashtable
		hashtable_heap_safe_iterator2(const BaseIt& it, const zero_bucket_arr& bucketArr)
			: base_type(it), mpSoftBucketArr(allocator_type::to_soft(bucketArr)) { }

		hashtable_heap_safe_iterator2(const BaseIt& it, const zero_bucket_arr& bucketArr, const zero_node_ptr& node)
			: base_type(it), mpSoftBucketArr(allocator_type::to_soft(bucketArr)), mpSoftNode(allocator_type::to_soft(node)) { }

    public:

        static this_type makeIt(const BaseIt& it, const zero_bucket_arr& bucketArr, uint32_t sz) {
			using nodecpp::assert::AssertLevel;
			NODECPP_ASSERT(module_id, AssertLevel::regular, it.get_node() != nullptr);
			NODECPP_ASSERT(module_id, AssertLevel::regular, it.get_bucket() != nullptr);

			if(allocator_type::is_empty_hashtable(bucketArr)) {
				NODECPP_ASSERT(module_id, AssertLevel::regular, it.get_bucket() == &(bucketArr[1]));
				NODECPP_ASSERT(module_id, AssertLevel::regular, allocator_type::is_hashtable_sentinel(it.get_node()));

				return {it};
			}

			if(allocator_type::is_hashtable_sentinel(it.get_node())) {
				auto ix = it.get_bucket() - bucketArr.get_raw_begin();
				NODECPP_ASSERT(module_id, AssertLevel::regular, ix == sz);
				return { it, bucketArr };
			}

			return { it, bucketArr, it.get_node() };
        }


		hashtable_heap_safe_iterator2() :base_type() { }

		hashtable_heap_safe_iterator2(const this_type&) = default;
		hashtable_heap_safe_iterator2& operator=(const hashtable_heap_safe_iterator2& ri) = default;

		hashtable_heap_safe_iterator2(hashtable_heap_safe_iterator2&& ri) = default; 
		hashtable_heap_safe_iterator2& operator=(hashtable_heap_safe_iterator2&& ri) = default;

		template<typename Other, std::enable_if_t<sfinae<Other>, bool> = true>
		hashtable_heap_safe_iterator2(const Other& other)
			: base_type(other), mpSoftBucketArr(other.mpSoftBucketArr), mpSoftNode(other.mpSoftNode) { }

		template<typename Other, std::enable_if_t<sfinae<Other>, bool> = true>
		hashtable_heap_safe_iterator2& operator=(const Other& other) {
			base_type::operator=(static_cast<const typename Other::base_type&>(other));

			this->mpSoftBucketArr = other.mpSoftBucketArr;
			this->mpSoftNode = other.mpSoftNode;

			return *this;
		}

		reference operator*() const {
			checkDerefenceable();
			return base_type::operator*();
		}

		pointer operator->() const {
			checkDerefenceable();
			return base_type::operator->();
		}

		this_type& operator++() {
			checkDerefenceable();
			base_type::operator++();
			setSoftNode();

			return *this;
		}

		this_type operator++(int) {
			this_type temp(*this);
			operator++();
			return temp;
		}

		bool operator==(const this_type& other) const {
			return eastl::operator==(static_cast<const base_type&>(*this), static_cast<const base_type&>(other));
		}
		bool operator!=(const this_type& other) const {
			return eastl::operator!=(static_cast<const base_type&>(*this), static_cast<const base_type&>(other));
		}

		void checkDerefenceable() const {
			if(NODECPP_UNLIKELY(!base_type::mpNode))
				ThrowNullException();
			else if(NODECPP_UNLIKELY(allocator_type::is_hashtable_sentinel(base_type::mpNode)))
				ThrowRangeException();
			
			checkNotInvalidated(mpSoftBucketArr);
			checkNotInvalidated(mpSoftNode);
		}

		void setSoftNode() {
			using nodecpp::assert::AssertLevel;
			NODECPP_ASSERT(module_id, AssertLevel::regular, base_type::mpNode != nullptr);
			if(allocator_type::is_hashtable_sentinel(base_type::mpNode))
				mpSoftNode = nullptr;
			else
				mpSoftNode = allocator_type::to_soft(base_type::mpNode);
		}

		const base_type& toBase() const {
			// base iterator can't be null
			if(NODECPP_UNLIKELY(!base_type::mpNode))
				ThrowNullException();
			
			return *this;
		}

		static this_type& fromBase(base_type& b) { return static_cast<this_type&>(b); }
		static const this_type& fromBase(const base_type& b) { return static_cast<const this_type&>(b); }
	}; // hashtable_heap_safe_iterator2


} // namespace safememory::detail 

#endif // SAFE_MEMORY_DETAIL_HASHTABLE_ITERATOR

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

#include <safememory/safe_ptr.h>
#include <safememory/detail/array_iterator.h>
#include <safememory/detail/instrument.h>
#include <safe_memory_error.h>

namespace safememory::detail {

	template <typename BaseIt, typename BaseNonConstIt, typename Allocator>
	class hashtable_heap_safe_iterator
	{
	public:
		typedef BaseIt                                                   base_type;
		typedef Allocator                                                allocator_type;
		typedef hashtable_heap_safe_iterator<BaseIt, BaseNonConstIt, Allocator>     this_type;
		typedef hashtable_heap_safe_iterator<BaseNonConstIt, BaseNonConstIt, Allocator>      this_type_non_const;
		typedef typename base_type::node_type                            node_type;
		typedef typename base_type::value_type                           value_type;
		typedef typename base_type::pointer                              pointer;
		typedef typename base_type::reference                            reference;
		typedef typename base_type::difference_type                      difference_type;
		typedef typename base_type::iterator_category                    iterator_category;

	    static constexpr memory_safety is_safe = allocator_type::is_safe;
	    static constexpr bool is_const = !std::is_same_v<this_type, this_type_non_const>;

    private:
		template <typename, typename, typename>
		friend class hashtable_heap_safe_iterator;

		template<typename TT>
		static constexpr bool sfinae = is_const && std::is_same_v<TT, this_type_non_const>;

		// template <typename, typename, typename, typename, memory_safety>
		// friend class unordered_map;



        typedef soft_ptr<node_type, allocator_type::is_safe>                        node_ptr;
		typedef typename allocator_type::template pointer<node_type>                base_node_ptr;
		typedef typename allocator_type::template array_pointer<base_node_ptr>                 t2;
		typedef typename allocator_type::template soft_array_pointer<base_node_ptr>            soft_bucket_type;
		// typedef typename detail::array_heap_safe_iterator<base_node_ptr, false, soft_bucket_type> bucket_iterator;

		

		base_node_ptr   mpNodeBase;        // current node, in zero_offset kind
		node_ptr    	mpNode;            // current node, in soft_ptr kind
		soft_bucket_type mpBucketArr;      // soft_ptr to bucket array
		uint32_t		 mCurrBucket = 0;  // index of current bucket


		void increment()
		{
#ifdef SAFEMEMORY_DEZOMBIEFY_ITERATORS
			dezombiefySoftPtr(mpNode);
			dezombiefySoftPtr(mpBucketArr);
#endif

			// mb: *mpBucket will be != nullptr at 'end()' sentinel
			// 		but 'to_soft' will convert it to a null
			mpNodeBase = mpNode->mpNext;

			while(mpNodeBase == NULL) {
				++mCurrBucket;
				mpNodeBase = *(mpBucketArr->data() + mCurrBucket);
			}

			if(allocator_type::is_hashtable_sentinel(mpNodeBase))
				mpNode = nullptr;
			else
				mpNode = allocator_type::to_soft(mpNodeBase);
		}


		// hashtable_heap_safe_iterator(const base_node_ptr& nodeBase, const node_ptr& node, const bucket_iterator& bucket)
		// 	: mpNodeBase(nodeBase), mpNode(node), mpBucket(bucket) { }

		hashtable_heap_safe_iterator(const base_node_ptr& nodeBase, node_ptr&& node, const t2& bucketArr, uint32_t currBucket)
			: mpNodeBase(nodeBase), mpNode(std::move(node)), mpBucketArr(allocator_type::to_soft(bucketArr)), mCurrBucket(currBucket) { }

    public:
		// template<class Ptr>
        static this_type makeIt(const BaseIt& it, const t2& heap_zero_ptr, uint32_t sz) {

			if(allocator_type::is_empty_hashtable(heap_zero_ptr))
				return {};


			auto ix = it.get_bucket() - heap_zero_ptr->data();
			if(allocator_type::is_hashtable_sentinel(it.get_node())) {
				NODECPP_ASSERT(module_id, nodecpp::assert::AssertLevel::regular, ix == sz);
				return { it.get_node(), {nullptr}, heap_zero_ptr, static_cast<uint32_t>(ix) };
			}
			else {
				return { it.get_node(), allocator_type::to_soft(it.get_node()), heap_zero_ptr, static_cast<uint32_t>(ix) };
			}
        }

		// template<class Ptr>
        // static this_type makeIt(const BaseIt& it, const t2& heap_ptr, uint32_t sz) {
		// 	//mb: on empty hashtable, heap_ptr will be != nullptr
		// 	//    but 'to_soft' will convert it to a null
		// 	// auto node = it.get_node();
		// 	// auto curr_bucket = it.get_bucket();
		// 	// auto soft_heap_ptr = allocator_type::to_soft(heap_ptr);
		// 	if(allocator_type::is_empty_hashtable(heap_ptr))
		// 		return this_type();//empty hashtable


		// 	auto safe_it = bucket_iterator::makePtr(allocator_type::to_soft(heap_ptr), it.get_bucket(), sz);
		// 	auto safe_node = allocator_type::to_soft(it.get_node()); 
		// 	return this_type(it.get_node(), safe_node, safe_it);
        // }


		hashtable_heap_safe_iterator() {}

		hashtable_heap_safe_iterator(const this_type&) = default;
		hashtable_heap_safe_iterator& operator=(const hashtable_heap_safe_iterator&) = default;

		hashtable_heap_safe_iterator(hashtable_heap_safe_iterator&&) = default; 
		hashtable_heap_safe_iterator& operator=(hashtable_heap_safe_iterator&&) = default;

		template<typename Other, std::enable_if_t<sfinae<Other>, bool> = true>
		hashtable_heap_safe_iterator(const Other& other)
			: mpNodeBase(other.mpNodeBase), mpNode(other.mpNode), mpBucketArr(other.mpBucketArr),
				mCurrBucket(other.mCurrBucket) { }

		template<typename Other, std::enable_if_t<sfinae<Other>, bool> = true>
		hashtable_heap_safe_iterator& operator=(const Other& other) {
			this->mpNodeBase = other.mpNodeBase;
			this->mpNode = other.mpNode;
			this->mpBucketArr = other.mpBucketArr;
			this->mCurrBucket = other.mCurrBucket;

			return *this;
		}

		reference operator*() const {
#ifdef SAFEMEMORY_DEZOMBIEFY_ITERATORS
			dezombiefySoftPtr(mpNode);
#endif
			return mpNode->mValue;
		}

		pointer operator->() const {
#ifdef SAFEMEMORY_DEZOMBIEFY_ITERATORS
			dezombiefySoftPtr(mpNode);
#endif
			return std::addressof(mpNode->mValue);
		}

		this_type& operator++() {
			increment();
			return *this;
		}

		this_type operator++(int) {
			this_type temp(*this);
			increment();
			return temp;
		}

        bool operator==(const this_type other) const { return mpNode == other.mpNode && mpBucketArr == other.mpBucketArr; }
        bool operator!=(const this_type other) const { return !operator==(other); }

		BaseIt toBase() const noexcept {
			return BaseIt(mpNodeBase, mpBucketArr->data() + mCurrBucket);
		}
	}; // hashtable_heap_safe_iterator

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

		[[noreturn]] static void throwRangeException(const char* msg) { throw nodecpp::error::out_of_range; }

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
			base_type::operator=(other.toBase());
			return *this;
		}

		reference operator*() const {
			if(NODECPP_LIKELY(base_type::mpNode && !allocator_type::is_hashtable_sentinel(base_type::mpNode))) {
#ifdef SAFEMEMORY_DEZOMBIEFY_ITERATORS
				dezombiefyRawPtr(allocator_type::to_raw(base_type::mpNode));
#endif
				return base_type::operator*();
			}
			else
				throwRangeException("hashtable_stack_only_iterator::operator*");
		}

		pointer operator->() const {
			if(NODECPP_LIKELY(base_type::mpNode && !allocator_type::is_hashtable_sentinel(base_type::mpNode))) {
#ifdef SAFEMEMORY_DEZOMBIEFY_ITERATORS
				dezombiefyRawPtr(allocator_type::to_raw(base_type::mpNode));
#endif
				return base_type::operator->();
			}
			else
				throwRangeException("hashtable_stack_only_iterator::operator->");
		}

		this_type& operator++() {
			if(NODECPP_LIKELY(base_type::mpNode && !allocator_type::is_hashtable_sentinel(base_type::mpNode))) {
#ifdef SAFEMEMORY_DEZOMBIEFY_ITERATORS
				dezombiefyRawPtr(base_type::mpBucket);
				dezombiefyRawPtr(allocator_type::to_raw(base_type::mpNode));
#endif
				base_type::operator++();
			}
			return *this;
		}

		this_type operator++(int) { 
			this_type temp(*this);
			operator++();
			return temp;
		}

		bool operator==(const this_type& other) const 
			{ return eastl::operator==(this->toBase(), other.toBase()); }
		bool operator!=(const this_type& other) const 
			{ return eastl::operator!=(this->toBase(), other.toBase()); }


		const base_type& toBase() const noexcept { return *this; }

		static this_type& fromBase(base_type& b) { return static_cast<this_type&>(b); }
		static const this_type& fromBase(const base_type& b) { return static_cast<const this_type&>(b); }
	}; // hashtable_stack_only_iterator



} // namespace safememory::detail 

#endif // SAFE_MEMORY_DETAIL_HASHTABLE_ITERATOR

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

#include <safe_memory/safe_ptr.h>

namespace safe_memory::detail {

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

    private:
		template <typename, typename, typename>
		friend class hashtable_heap_safe_iterator;

		// template <typename, typename, typename, typename, memory_safety>
		// friend class unordered_map;



        typedef soft_ptr<node_type, is_safe>                                   node_ptr;
		typedef typename Allocator::template pointer_types<node_type>::pointer   node_pointer;


		typedef typename Allocator::template pointer_types<node_type>::bucket_iterator   bucket_iterator;
		// typedef eastl::type_select_t<is_safe == memory_safety::safe, 
		// 	detail::array_of_iterator_impl<node_pointer, false, soft_ptr_impl>,
		// 	detail::array_of_iterator_no_checks<node_pointer, false>>             bucket_iterator;

		node_ptr    	mpNode;      // Current node within current bucket.
		bucket_iterator mpBucket;    // Current bucket.

		void increment()
		{
			// mb: *mpBucket will be != nullptr at 'end()' sentinel
			// 		but 'to_soft' will convert it to a null
			auto mpTmp = mpNode->mpNext;

			while(mpTmp == NULL)
				mpTmp = *++mpBucket;

			mpNode = allocator_type::to_soft(mpTmp);
		}


		hashtable_heap_safe_iterator(node_ptr node, bucket_iterator bucket)
			: mpNode(node), mpBucket(bucket) { }

    public:
        template<class HeapPtr>
        static this_type makeIt(const BaseIt& it, const HeapPtr& heap_ptr, uint32_t sz) {
			//mb: on empty hashtable, heap_ptr will be != nullptr
			//    but 'to_soft' will convert it to a null
			// auto node = it.get_node();
			// auto curr_bucket = it.get_bucket();
			// auto soft_heap_ptr = allocator_type::to_soft(heap_ptr);
			if(!allocator_type::is_empty_hashtable(heap_ptr))
				return this_type(allocator_type::to_soft(it.get_node()), 
					bucket_iterator::makePtr(allocator_type::to_soft(heap_ptr), it.get_bucket(), sz));
			else
				return this_type();//empty hashtable
        }



		hashtable_heap_safe_iterator()
			: mpNode(), mpBucket() { }

		hashtable_heap_safe_iterator(const this_type&) = default;
		hashtable_heap_safe_iterator& operator=(const hashtable_heap_safe_iterator&) = default;

		hashtable_heap_safe_iterator(hashtable_heap_safe_iterator&&) = default; 
		hashtable_heap_safe_iterator& operator=(hashtable_heap_safe_iterator&&) = default;

		template<typename B2, typename X = std::enable_if_t<std::is_same_v<B2, BaseNonConstIt> && !std::is_same_v<B2, BaseIt>>>
		hashtable_heap_safe_iterator(const hashtable_heap_safe_iterator<B2, B2, Allocator>& other)
			: mpNode(other.mpNode), mpBucket(other.mpBucket) { }

		template<typename B2, typename X = std::enable_if_t<std::is_same_v<B2, BaseNonConstIt> && !std::is_same_v<B2, BaseIt>>>
		hashtable_heap_safe_iterator& operator=(const hashtable_heap_safe_iterator<B2, B2, Allocator>& other) {
			this->mpNode = other.mpNode;
			this->mpBucket = other.mpBucket;
			return *this;
		}

		reference operator*() const { return mpNode->mValue; }
		pointer operator->() const { return &(mpNode->mValue); }

		this_type& operator++() {
			increment();
			return *this;
		}

		this_type operator++(int) {
			this_type temp(*this);
			increment();
			return temp;
		}

        bool operator==(const this_type other) const { return mpNode == other.mpNode; }
        bool operator!=(const this_type other) const { return mpNode != other.mpNode; }

		// const node_ptr& getNodePtr() const noexcept { return mpNode; }
		// const bucket_iterator& getBucketIt() const noexcept { return mpBucket; }

		BaseIt toBase() const noexcept {
			return BaseIt::make_unsafe(allocator_type::to_zero(mpNode), mpBucket.getRaw());
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

		template <typename, typename, typename>
		friend class hashtable_stack_only_iterator;
		// typedef typename allocator_type::template pointer_types<node_type>::pointer   node_pointer;

		[[noreturn]] static void throwRangeException(const char* msg) { throw std::out_of_range(msg); }

    public:
		hashtable_stack_only_iterator() :base_type() { }

		hashtable_stack_only_iterator(const this_type&) = default;
		hashtable_stack_only_iterator& operator=(const hashtable_stack_only_iterator& ri) = default;

		hashtable_stack_only_iterator(hashtable_stack_only_iterator&& ri) = default; 
		hashtable_stack_only_iterator& operator=(hashtable_stack_only_iterator&& ri) = default;

		template<typename B2, typename X = std::enable_if_t<std::is_same_v<B2, BaseNonConstIt> && !std::is_same_v<B2, BaseIt>>>
		hashtable_stack_only_iterator(const hashtable_stack_only_iterator<B2, B2, Allocator>& other)
			: base_type(other) { }

		template<typename B2, typename X = std::enable_if_t<std::is_same_v<B2, BaseNonConstIt> && !std::is_same_v<B2, BaseIt>>>
		hashtable_stack_only_iterator& operator=(const hashtable_stack_only_iterator<B2, B2, Allocator>& other) {
			base_type::operator=(other.toBase());
			return *this;
		}

		reference operator*() const {
			if(NODECPP_LIKELY(base_type::mpNode && !allocator_type::is_hashtable_sentinel(base_type::mpNode))) {
				return base_type::operator*();
			}
			else
				throwRangeException("hashtable_stack_only_iterator::operator*");
		}

		pointer operator->() const {
			if(NODECPP_LIKELY(base_type::mpNode && !allocator_type::is_hashtable_sentinel(base_type::mpNode))) {
				return base_type::operator->();
			}
			else
				throwRangeException("hashtable_stack_only_iterator::operator->");
		}

		this_type& operator++() {
			if(NODECPP_LIKELY(base_type::mpNode && !allocator_type::is_hashtable_sentinel(base_type::mpNode))) {
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


		// const node_pointer& getNodePtr() const noexcept { return base_type::mpNode; }
		// node_pointer* getBucketIt() const noexcept { return base_type::mpBucket; }

		const base_type& toBase() const noexcept { return *this; }

		static this_type& fromBase(base_type& b) { return static_cast<this_type&>(b); }
		static const this_type& fromBase(const base_type& b) { return static_cast<const this_type&>(b); }
	}; // hashtable_stack_only_iterator



} // namespace safe_memory::detail 

#endif // SAFE_MEMORY_DETAIL_HASHTABLE_ITERATOR

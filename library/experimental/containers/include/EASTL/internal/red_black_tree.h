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
// https://github.com/electronicarts/EASTL/blob/3.15.00/include/EASTL/internal/red_black_tree.h

/////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////


#ifndef EASTL_RED_BLACK_TREE_H
#define EASTL_RED_BLACK_TREE_H


//#include <EABase/eabase.h>
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
	#pragma once
#endif

#include <EASTL/internal/config.h>
#include <EASTL/type_traits.h>
#include <type_traits>
//#include <EASTL/allocator.h>
#include <iterator>
#include <EASTL/utility.h>
#include <utility>
#include <algorithm>
#include <initializer_list>
#include <tuple>

#include "red_black_tree_cpp.h"

#include <safe_ptr_for_map.h>
#include <safe_ptr.h>

// EA_DISABLE_ALL_VC_WARNINGS()
// #include <new>
// #include <stddef.h>
// EA_RESTORE_ALL_VC_WARNINGS()


#ifdef _MSC_VER
	#pragma warning(push)
	#pragma warning(disable: 4512)  // 'class' : assignment operator could not be generated
	#pragma warning(disable: 4530)  // C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc
	#pragma warning(disable: 4571)  // catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught.
#endif


namespace nodecpp
{

	/// EASTL_RBTREE_DEFAULT_NAME
	///
	/// Defines a default container name in the absence of a user-provided name.
	///
	#ifndef EASTL_RBTREE_DEFAULT_NAME
		#define EASTL_RBTREE_DEFAULT_NAME EASTL_DEFAULT_NAME_PREFIX " rbtree" // Unless the user overrides something, this is "EASTL rbtree".
	#endif


	/// EASTL_RBTREE_DEFAULT_ALLOCATOR
	///
	#ifndef EASTL_RBTREE_DEFAULT_ALLOCATOR
		#define EASTL_RBTREE_DEFAULT_ALLOCATOR allocator_type(EASTL_RBTREE_DEFAULT_NAME)
	#endif


	/// EASTL_RBTREE_LEGACY_SWAP_BEHAVIOUR_REQUIRES_COPY_CTOR
	///
	#ifndef EASTL_RBTREE_LEGACY_SWAP_BEHAVIOUR_REQUIRES_COPY_CTOR
		#define EASTL_RBTREE_LEGACY_SWAP_BEHAVIOUR_REQUIRES_COPY_CTOR 0
	#endif





	// rbtree_node_base functions
	//
	// These are the fundamental functions that we use to maintain the 
	// tree. The bulk of the work of the tree maintenance is done in 
	// these functions.
	//
	// EASTL_API rbtree_soft_ptr RBTreeIncrement    (rbtree_soft_ptr pNode);
	// EASTL_API rbtree_soft_ptr RBTreeDecrement    (rbtree_soft_ptr pNode);
	// EASTL_API rbtree_soft_ptr RBTreeGetMinChild  (rbtree_soft_ptr pNode);
	// EASTL_API rbtree_soft_ptr RBTreeGetMaxChild  (rbtree_soft_ptr pNode);
	// EASTL_API size_t            RBTreeGetBlackCount(rbtree_soft_ptr pNodeTop,
	// 												rbtree_soft_ptr pNodeBottom);
	// EASTL_API void              RBTreeInsert       (      rbtree_owning_ptr pNode,
	// 													  rbtree_soft_ptr pNodeParent,
	// 													  rbtree_soft_ptr pNodeAnchor,
	// 													  rbtree_min_max_nodes& pMinMaxNodes,
	// 													  RBTreeSide insertionSide);
	// EASTL_API rbtree_owning_ptr RBTreeErase        (      rbtree_soft_ptr pNode,
	// 													  rbtree_soft_ptr pNodeAnchor,
	// 													  rbtree_min_max_nodes& pMinMaxNodes); 







	/// rbtree_iterator
	///
	template <typename T, typename Pointer, typename Reference>
	struct rbtree_iterator
	{
		typedef rbtree_iterator<T, Pointer, Reference>      this_type;
		typedef rbtree_iterator<T, T*, T&>                  iterator;
		typedef rbtree_iterator<T, const T*, const T&>      const_iterator;
		typedef std::size_t                                 size_type;
		typedef std::ptrdiff_t                              difference_type;
		typedef T                                           value_type;
//		typedef rbtree_node_base                            base_node_type;
		typedef rbtree_node<T>                              node_type;
		typedef Pointer                                     pointer;
		typedef Reference                                   reference;
		typedef std::bidirectional_iterator_tag             iterator_category;
		typedef typename node_type::rbtree_soft_ptr			rbtree_soft_ptr;
		typedef typename node_type::rbtree_soft0_ptr		rbtree_soft0_ptr;

	public:
		rbtree_soft_ptr mpNode;
		rbtree_soft0_ptr mpEndNode = nullptr;

		rbtree_soft_ptr get_node() const { return mpNode; }
		void set_from_base_ptr(rbtree_soft0_ptr pNode);
		const rbtree_soft0_ptr get_end_node() const { return mpEndNode; }
		void assert_alive_and_not_end() const {
			if(mpNode == mpEndNode)
				throw "TODO";

			//TODO: think if end is alive or not
			mpNode->assert_is_alive();
		}

		void assert_alive_or_end() const {
			if(mpNode != mpEndNode)
				mpNode->assert_is_alive();
		}

	public:
		rbtree_iterator();
		explicit rbtree_iterator(rbtree_soft_ptr pNode, rbtree_soft0_ptr pEndNode);
		explicit rbtree_iterator(rbtree_soft0_ptr pNode, rbtree_soft0_ptr pEndNode);
		rbtree_iterator(const iterator& x);
//		rbtree_iterator& operator=(const rbtree_iterator&) = default; //TODO review

		reference operator*() const;
		pointer   operator->() const;

		rbtree_iterator& operator++();
		rbtree_iterator  operator++(int);

		rbtree_iterator& operator--();
		rbtree_iterator  operator--(int);

	}; // rbtree_iterator


	///////////////////////////////////////////////////////////////////////////////
	// rb_base_compare_ebo
	//
	// Utilizes the "empty base-class optimization" to reduce the size of the rbtree
	// when its Compare template argument is an empty class.
	///////////////////////////////////////////////////////////////////////////////

	template <typename Compare, bool /*isEmpty*/ = std::is_empty<Compare>::value>
	struct rb_base_compare_ebo
	{
	protected:
		rb_base_compare_ebo() : mCompare() {}
		rb_base_compare_ebo(const Compare& compare) : mCompare(compare) {}

		Compare& get_compare() { return mCompare; }
		const Compare& get_compare() const { return mCompare; }

		template <typename T>
		bool compare(const T& lhs, const T& rhs) 
		{
			return mCompare(lhs, rhs);
		}

		template <typename T>
		bool compare(const T& lhs, const T& rhs) const
		{
			return mCompare(lhs, rhs);
		}

	private:
		Compare mCompare;
	};

	template <typename Compare>
	struct rb_base_compare_ebo<Compare, true> : private Compare
	{
	protected:
		rb_base_compare_ebo() {}
		rb_base_compare_ebo(const Compare& compare) : Compare(compare) {}

		Compare& get_compare() { return *this; }
		const Compare& get_compare() const { return *this; }

		template <typename T>
		bool compare(const T& lhs, const T& rhs) 
		{
			return Compare::operator()(lhs, rhs);
		}

		template <typename T>
		bool compare(const T& lhs, const T& rhs) const
		{
			return Compare::operator()(lhs, rhs);
		}
	};



	///////////////////////////////////////////////////////////////////////////////
    // rb_base
    //
    // This class allows us to use a generic rbtree as the basis of map, multimap,
    // set, and multiset transparently. The vital template parameters for this are 
    // the ExtractKey and the bUniqueKeys parameters.
    //
    // If the rbtree has a value type of the form pair<T1, T2> (i.e. it is a map or
    // multimap and not a set or multiset) and a key extraction policy that returns 
    // the first part of the pair, the rbtree gets a mapped_type typedef. 
    // If it satisfies those criteria and also has unique keys, then it also gets an 
    // operator[] (which only map and set have and multimap and multiset don't have).
    //
    ///////////////////////////////////////////////////////////////////////////////



	/// rb_base
	/// This specialization is used for 'set'. In this case, Key and Value 
	/// will be the same as each other and ExtractKey will be eastl::use_self.
	///
	template <typename Key, typename Value, typename Compare, typename ExtractKey, bool bUniqueKeys, typename RBTree>
	struct rb_base : public rb_base_compare_ebo<Compare>
	{
		typedef ExtractKey extract_key;

	protected:
		using rb_base_compare_ebo<Compare>::compare;
		using rb_base_compare_ebo<Compare>::get_compare;

	public:
		rb_base() {}
		rb_base(const Compare& compare) : rb_base_compare_ebo<Compare>(compare) {}
	};


	/// rb_base
	/// This class is used for 'multiset'.
	/// In this case, Key and Value will be the same as each 
	/// other and ExtractKey will be eastl::use_self.
	///
	template <typename Key, typename Value, typename Compare, typename ExtractKey, typename RBTree>
	struct rb_base<Key, Value, Compare, ExtractKey, false, RBTree> : public rb_base_compare_ebo<Compare>
	{
		typedef ExtractKey extract_key;

	protected:
		using rb_base_compare_ebo<Compare>::compare;
		using rb_base_compare_ebo<Compare>::get_compare;

	public:
		rb_base() {}
		rb_base(const Compare& compare) : rb_base_compare_ebo<Compare>(compare) {}
	};


	/// rb_base
	/// This specialization is used for 'map'.
	///
	template <typename Key, typename Pair, typename Compare, typename RBTree>
	struct rb_base<Key, Pair, Compare, nodecpp::use_first<Pair>, true, RBTree> : public rb_base_compare_ebo<Compare>
	{
		typedef nodecpp::use_first<Pair> extract_key;

		using rb_base_compare_ebo<Compare>::compare;
		using rb_base_compare_ebo<Compare>::get_compare;

	public:
		rb_base() {}
		rb_base(const Compare& compare) : rb_base_compare_ebo<Compare>(compare) {}
	};


	/// rb_base
	/// This specialization is used for 'multimap'.
	///
	template <typename Key, typename Pair, typename Compare, typename RBTree>
	struct rb_base<Key, Pair, Compare, nodecpp::use_first<Pair>, false, RBTree> : public rb_base_compare_ebo<Compare>
	{
		typedef nodecpp::use_first<Pair> extract_key;

		using rb_base_compare_ebo<Compare>::compare;
		using rb_base_compare_ebo<Compare>::get_compare;

	public:
		rb_base() {}
		rb_base(const Compare& compare) : rb_base_compare_ebo<Compare>(compare) {}
	};


	/// rbtree
	///
	/// rbtree is the red-black tree basis for the map, multimap, set, and multiset 
	/// containers. Just about all the work of those containers is done here, and 
	/// they are merely a shell which sets template policies that govern the code
	/// generation for this rbtree.
	///
	/// This rbtree implementation is pretty much the same as all other modern 
	/// rbtree implementations, as the topic is well known and researched. We may
	/// choose to implement a "relaxed balancing" option at some point in the 
	/// future if it is deemed worthwhile. Most rbtree implementations don't do this.
	///
	/// The primary rbtree member pointer is mNodeEnd, which is a node_type and 
	/// acts as the end node. We do the conventional trick of 
	/// assigning the tree root
	/// node to mpNodeLeft. This makes the end node naturally after the last
	/// node of the tree
	///
	/// We keep references to begin() (left-most rbtree node) and 
	/// 'end() - 1' (a.k.a. rbegin()) at mMinMaxNodes 
	///
	/// Compare (functor): This is a comparison class which defaults to 'less'.
	/// It is a common STL thing which takes two arguments and returns true if  
	/// the first is less than the second.
	///
	/// ExtractKey (functor): This is a class which gets the key from a stored
	/// node. With map and set, the node is a pair, whereas with set and multiset
	/// the node is just the value. ExtractKey will be either eastl::use_first (map and multimap)
	/// or eastl::use_self (set and multiset).
	///
	/// bMutableIterators (bool): true if rbtree::iterator is a mutable
	/// iterator, false if iterator and const_iterator are both const iterators. 
	/// It will be true for map and multimap and false for set and multiset.
	///
	/// bUniqueKeys (bool): true if the keys are to be unique, and false if there
	/// can be multiple instances of a given key. It will be true for set and map 
	/// and false for multiset and multimap.
	///
	/// To consider: Add an option for relaxed tree balancing. This could result 
	/// in performance improvements but would require a more complicated implementation.
	///
	///////////////////////////////////////////////////////////////////////
	/// find_as
	/// In order to support the ability to have a tree of strings but
	/// be able to do efficiently lookups via char pointers (i.e. so they
	/// aren't converted to string objects), we provide the find_as
	/// function. This function allows you to do a find with a key of a
	/// type other than the tree's key type. See the find_as function
	/// for more documentation on this.
	///
	template <typename Key, typename Value, typename Compare, typename Allocator, 
			  typename ExtractKey, bool bMutableIterators, bool bUniqueKeys>
	class rbtree
		: public rb_base<Key, Value, Compare, ExtractKey, bUniqueKeys, 
							rbtree<Key, Value, Compare, Allocator, ExtractKey, bMutableIterators, bUniqueKeys> >
	{
	public:
		typedef std::ptrdiff_t                                                                       difference_type;
		typedef std::size_t                                                                    size_type;     // See config.h for the definition of eastl_size_t, which defaults to size_t.
		typedef Key                                                                             key_type;
		typedef Value                                                                           value_type;
		typedef rbtree_node<value_type>                                                         node_type;
		typedef value_type&                                                                     reference;
		typedef const value_type&                                                               const_reference;
		typedef value_type*                                                                     pointer;
		typedef const value_type*                                                               const_pointer;

		typedef typename type_select<bMutableIterators, 
					rbtree_iterator<value_type, value_type*, value_type&>, 
					rbtree_iterator<value_type, const value_type*, const value_type&> >::type   iterator;
		typedef rbtree_iterator<value_type, const value_type*, const value_type&>               const_iterator;
		typedef std::reverse_iterator<iterator>                                               reverse_iterator;
		typedef std::reverse_iterator<const_iterator>                                         const_reverse_iterator;

		typedef Allocator                                                                       allocator_type;
		typedef Compare                                                                         key_compare;
		typedef typename type_select<bUniqueKeys, std::pair<iterator, bool>, iterator>::type  insert_return_type;  // map/set::insert return a pair, multimap/multiset::iterator return an iterator.
		typedef rbtree<Key, Value, Compare, Allocator, 
						ExtractKey, bMutableIterators, bUniqueKeys>                             this_type;
		typedef rb_base<Key, Value, Compare, ExtractKey, bUniqueKeys, this_type>                base_type;
		typedef std::integral_constant<bool, bUniqueKeys>                                            has_unique_keys_type;
		typedef typename base_type::extract_key                                                 extract_key;

//		typedef typename node_type::rbtree_owning_ptr											node_owning_ptr;
//		typedef typename node_type::rbtree_soft_ptr												node_soft_ptr;

		typedef typename node_type::rbtree_owning_ptr											rbtree_owning_ptr;
		typedef typename node_type::rbtree_soft_ptr												rbtree_soft_ptr;
		typedef typename node_type::rbtree_soft0_ptr											rbtree_soft0_ptr;


	protected:
		using base_type::compare;
		using base_type::get_compare;

	public:
//		rbtree_node_base  mAnchor;      /// This node acts as end() and its mpLeft points to begin(), and mpRight points to rbegin() (the last node on the right).
//		allocator_type    mAllocator;   // To do: Use base class optimization to make this go away.

		// mMinMaxNodex must be initialized before mpEndNode does
		size_type         		mnSize = 0;       /// Stores the count of nodes in the tree (not counting the anchor node).
		rbtree_min_max_nodes<rbtree_soft0_ptr> 	mMinMaxNodes;
		rbtree_owning_ptr        	mpNodeEnd = create_end_node();      /// This node acts as end() and its mpLeft points to begin(), and mpRight points to rbegin() (the last node on the right).

	public:
		// ctor/dtor
		rbtree();
		// rbtree(const allocator_type& allocator);
		rbtree(const Compare& compare/*, const allocator_type& allocator = EASTL_RBTREE_DEFAULT_ALLOCATOR*/);
		rbtree(const this_type& x);
		rbtree(this_type&& x);
		// rbtree(this_type&& x, const allocator_type& allocator);

		template <typename InputIterator>
		rbtree(InputIterator first, InputIterator last, const Compare& compare/*, const allocator_type& allocator = EASTL_RBTREE_DEFAULT_ALLOCATOR*/);

	   ~rbtree();

	public:
		// properties
		// const allocator_type& get_allocator() const EA_NOEXCEPT;
		// allocator_type&       get_allocator() EA_NOEXCEPT;
		// void                  set_allocator(const allocator_type& allocator);

		const key_compare& key_comp() const { return get_compare(); }
		key_compare&       key_comp()       { return get_compare(); }

		this_type& operator=(const this_type& x);
		this_type& operator=(std::initializer_list<value_type> ilist);
		this_type& operator=(this_type&& x);

		void swap(this_type& x);

	public: 
		// iterators
		iterator        begin() EA_NOEXCEPT;
		const_iterator  begin() const EA_NOEXCEPT;
		const_iterator  cbegin() const EA_NOEXCEPT;

		iterator        end() EA_NOEXCEPT;
		const_iterator  end() const EA_NOEXCEPT;
		const_iterator  cend() const EA_NOEXCEPT;

		reverse_iterator        rbegin() EA_NOEXCEPT;
		const_reverse_iterator  rbegin() const EA_NOEXCEPT;
		const_reverse_iterator  crbegin() const EA_NOEXCEPT;

		reverse_iterator        rend() EA_NOEXCEPT;
		const_reverse_iterator  rend() const EA_NOEXCEPT;
		const_reverse_iterator  crend() const EA_NOEXCEPT;

	public:
		bool      empty() const EA_NOEXCEPT;
		size_type size() const EA_NOEXCEPT;

		template <class... Args>
		insert_return_type emplace(Args&&... args);

		template <class... Args> 
		iterator emplace_hint(const_iterator position, Args&&... args);

		template <class... Args> std::pair<iterator, bool> try_emplace(const key_type& k, Args&&... args);
		template <class... Args> std::pair<iterator, bool> try_emplace(key_type&& k, Args&&... args);
		template <class... Args> iterator                    try_emplace(const_iterator position, const key_type& k, Args&&... args);
		template <class... Args> iterator                    try_emplace(const_iterator position, key_type&& k, Args&&... args);

		// Standard conversion overload to avoid the overhead of mismatched 'pair<const Key, Value>' types.
		template <class P, class = typename std::enable_if<std::is_constructible<value_type, P&&>::value>::type> 
		insert_return_type insert(P&& otherValue);

		// Currently limited to value_type instead of P because it collides with insert(InputIterator, InputIterator).
		// To allow this to work with templated P we need to implement a compile-time specialization for the
		// case that P&& is const_iterator and have that specialization handle insert(InputIterator, InputIterator)
		// instead of insert(InputIterator, InputIterator). Curiously, neither libstdc++ nor libc++
		// implement this function either, which suggests they ran into the same problem I did here
		// and haven't yet resolved it (at least as of March 2014, GCC 4.8.1).
		iterator insert(const_iterator hint, value_type&& value);

		/// map::insert and set::insert return a pair, while multimap::insert and
		/// multiset::insert return an iterator.
		insert_return_type insert(const value_type& value);

		// C++ standard: inserts value if and only if there is no element with 
		// key equivalent to the key of t in containers with unique keys; always 
		// inserts value in containers with equivalent keys. Always returns the 
		// iterator pointing to the element with key equivalent to the key of value. 
		// iterator position is a hint pointing to where the insert should start
		// to search. However, there is a potential defect/improvement report on this behaviour:
		// LWG issue #233 (http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2005/n1780.html)
		// We follow the same approach as SGI STL/STLPort and use the position as
		// a forced insertion position for the value when possible.
		iterator insert(const_iterator position, const value_type& value);

		void insert(std::initializer_list<value_type> ilist);

		template <typename InputIterator>
		void insert(InputIterator first, InputIterator last);

		// TODO(rparolin):
		// insert_return_type insert(node_type&& nh);
		// iterator insert(const_iterator hint, node_type&& nh);

		template <class M> std::pair<iterator, bool> insert_or_assign(const key_type& k, M&& obj);
		template <class M> std::pair<iterator, bool> insert_or_assign(key_type&& k, M&& obj);
		template <class M> iterator             insert_or_assign(const_iterator hint, const key_type& k, M&& obj);
		template <class M> iterator             insert_or_assign(const_iterator hint, key_type&& k, M&& obj);

		iterator         erase(const_iterator position);
		iterator         erase(const_iterator first, const_iterator last);
		reverse_iterator erase(const_reverse_iterator position);
		reverse_iterator erase(const_reverse_iterator first, const_reverse_iterator last);

		// For some reason, multiple STL versions make a specialization 
		// for erasing an array of key_types. I'm pretty sure we don't
		// need this, but just to be safe we will follow suit. 
		// The implementation is trivial. Returns void because the values
		// could well be randomly distributed throughout the tree and thus
		// a return value would be nearly meaningless.
		void erase(const key_type* first, const key_type* last);

		void clear();
		// void reset_lose_memory(); // This is a unilateral reset to an initially empty state. No destructors are called, no deallocation occurs.

		iterator       find(const key_type& key);
		const_iterator find(const key_type& key) const;

		/// Implements a find whereby the user supplies a comparison of a different type
		/// than the tree's value_type. A useful case of this is one whereby you have
		/// a container of string objects but want to do searches via passing in char pointers.
		/// The problem is that without this kind of find, you need to do the expensive operation
		/// of converting the char pointer to a string so it can be used as the argument to the
		/// find function.
		///
		/// Example usage (note that the compare uses string as first type and char* as second):
		///     set<string> strings;
		///     strings.find_as("hello", less_2<string, const char*>());
		///
		template <typename U, typename Compare2> iterator       find_as(const U& u, Compare2 compare2);
		template <typename U, typename Compare2> const_iterator find_as(const U& u, Compare2 compare2) const;

		iterator       lower_bound(const key_type& key);
		const_iterator lower_bound(const key_type& key) const;

		iterator       upper_bound(const key_type& key);
		const_iterator upper_bound(const key_type& key) const;

		bool validate() const;
		int  validate_iterator(const_iterator i) const;

	protected:
		// static
//		node_owning_ptr DoAllocateNode();
		void       DoFreeNode(rbtree_owning_ptr pNode);

		rbtree_owning_ptr DoCreateNodeFromKey(const key_type& key);

		template<class... Args>
		rbtree_owning_ptr DoCreateNode(Args&&... args);
		rbtree_owning_ptr DoCreateNode(const value_type& value);
		rbtree_owning_ptr DoCreateNode(value_type&& value);
		rbtree_owning_ptr DoCreateNode(const rbtree_soft0_ptr pNodeSource, rbtree_soft0_ptr pNodeParent);

		rbtree_owning_ptr DoCopySubtree(const rbtree_soft0_ptr pNodeSource, rbtree_soft0_ptr pNodeDest);
		void       DoNukeSubtree(rbtree_owning_ptr pNode);

		template <class... Args>
		std::pair<iterator, bool> DoInsertValue(std::true_type, Args&&... args);

		template <class... Args>
		iterator DoInsertValue(std::false_type, Args&&... args);

		std::pair<iterator, bool> DoInsertValue(std::true_type, value_type&& value);
		iterator DoInsertValue(std::false_type, value_type&& value);

		template <class... Args>
		iterator DoInsertValueImpl(rbtree_soft0_ptr pNodeParent, bool bForceToLeft, const key_type& key, Args&&... args);
		iterator DoInsertValueImpl(rbtree_soft0_ptr pNodeParent, bool bForceToLeft, const key_type& key, rbtree_owning_ptr pNodeNew);

		std::pair<iterator, bool> DoInsertKey(std::true_type, const key_type& key);
		iterator                    DoInsertKey(std::false_type, const key_type& key);

		iterator DoInsertValueHint(std::true_type, const_iterator position, const value_type& value);
		iterator DoInsertValueHint(std::false_type, const_iterator position, const value_type& value);

		iterator DoInsertKey(std::true_type, const_iterator position, const key_type& key);  // By design we return iterator and not a pair.
		iterator DoInsertKey(std::false_type, const_iterator position, const key_type& key);
		iterator DoInsertKeyImpl(rbtree_soft0_ptr pNodeParent, bool bForceToLeft, const key_type& key);

		rbtree_soft0_ptr DoGetKeyInsertionPositionUniqueKeys(bool& canInsert, const key_type& key);
		rbtree_soft0_ptr DoGetKeyInsertionPositionNonuniqueKeys(const key_type& key);

		rbtree_soft0_ptr DoGetKeyInsertionPositionUniqueKeysHint(const_iterator position, bool& bForceToLeft, const key_type& key);
		rbtree_soft0_ptr DoGetKeyInsertionPositionNonuniqueKeysHint(const_iterator position, bool& bForceToLeft, const key_type& key);

		rbtree_owning_ptr create_end_node();
//		static
//		void reset_end_node(node_soft_ptr anchor, rbtree_min_max_nodes& all_nodes);
		void check_iterator(const const_iterator& position) const;
		rbtree_soft0_ptr get_root_node() const { return mpNodeEnd->get_node_left(); }
		rbtree_owning_ptr take_root_node() {
			return mpNodeEnd->take_node_left();
		}
		rbtree_soft0_ptr get_end_node() const { return mpNodeEnd; }
		// node_owning_ptr take_end_node() { 
		// 	node_owning_ptr pNode = std::move(mpNodeEnd);
		// 	mpNodeEnd = nullptr;
		// 	return pNode;
		// }
		rbtree_soft0_ptr get_begin_node() const { return mMinMaxNodes.mpMinChild; }
		rbtree_soft0_ptr get_last_node() const { return mMinMaxNodes.mpMaxChild; }
//		node_type* get_last_node() const { return mpNodeEnd; }
	}; // rbtree





	///////////////////////////////////////////////////////////////////////
	// rbtree_iterator functions
	///////////////////////////////////////////////////////////////////////

	template <typename T, typename Pointer, typename Reference>
	void
	rbtree_iterator<T, Pointer, Reference>::set_from_base_ptr(rbtree_soft0_ptr pNode) {
		if(!pNode)
			throw "TODO";

		mpNode = pNode.get();
	}

	template <typename T, typename Pointer, typename Reference>
	rbtree_iterator<T, Pointer, Reference>::rbtree_iterator()
		/*: mpNode(nullptr)*/ { }


	template <typename T, typename Pointer, typename Reference>
	rbtree_iterator<T, Pointer, Reference>::rbtree_iterator(rbtree_soft_ptr pNode, const rbtree_soft0_ptr pEndNode)
		: mpNode(pNode), mpEndNode(pEndNode) { }

	template <typename T, typename Pointer, typename Reference>
	rbtree_iterator<T, Pointer, Reference>::rbtree_iterator(rbtree_soft0_ptr pNode, const rbtree_soft0_ptr pEndNode)
		: mpNode(pNode.get()), mpEndNode(pEndNode) { }

	template <typename T, typename Pointer, typename Reference>
	rbtree_iterator<T, Pointer, Reference>::rbtree_iterator(const iterator& x)
		: mpNode(x.mpNode), mpEndNode(x.mpEndNode) { }


	template <typename T, typename Pointer, typename Reference>
	typename rbtree_iterator<T, Pointer, Reference>::reference
	rbtree_iterator<T, Pointer, Reference>::operator*() const
		{ assert_alive_and_not_end(); return mpNode->mValue; }


	template <typename T, typename Pointer, typename Reference>
	typename rbtree_iterator<T, Pointer, Reference>::pointer
	rbtree_iterator<T, Pointer, Reference>::operator->() const
		{ assert_alive_and_not_end(); return &mpNode->mValue; }


	template <typename T, typename Pointer, typename Reference>
	typename rbtree_iterator<T, Pointer, Reference>::this_type&
	rbtree_iterator<T, Pointer, Reference>::operator++()
	{
		assert_alive_and_not_end();
		set_from_base_ptr(RBTreeIncrement<rbtree_soft0_ptr, rbtree_soft_ptr>(mpNode));
		return *this;
	}


	template <typename T, typename Pointer, typename Reference>
	typename rbtree_iterator<T, Pointer, Reference>::this_type
	rbtree_iterator<T, Pointer, Reference>::operator++(int)
	{
		assert_alive_and_not_end();
		this_type temp(*this);
		set_from_base_ptr(RBTreeIncrement<rbtree_soft0_ptr, rbtree_soft_ptr>(mpNode));
		return temp;
	}


	template <typename T, typename Pointer, typename Reference>
	typename rbtree_iterator<T, Pointer, Reference>::this_type&
	rbtree_iterator<T, Pointer, Reference>::operator--()
	{
		assert_alive_or_end();
		set_from_base_ptr(RBTreeDecrement<rbtree_soft0_ptr, rbtree_soft_ptr>(mpNode));
		return *this;
	}


	template <typename T, typename Pointer, typename Reference>
	typename rbtree_iterator<T, Pointer, Reference>::this_type
	rbtree_iterator<T, Pointer, Reference>::operator--(int)
	{
		assert_alive_or_end();
		this_type temp(*this);
		set_from_base_ptr(RBTreeDecrement<rbtree_soft0_ptr, rbtree_soft_ptr>(mpNode));
		return temp;
	}


	// The C++ defect report #179 requires that we support comparisons between const and non-const iterators.
	// Thus we provide additional template paremeters here to support this. The defect report does not
	// require us to support comparisons between reverse_iterators and const_reverse_iterators.
	template <typename T, typename PointerA, typename ReferenceA, typename PointerB, typename ReferenceB>
	inline bool operator==(const rbtree_iterator<T, PointerA, ReferenceA>& a, 
						   const rbtree_iterator<T, PointerB, ReferenceB>& b)
	{
		return a.mpNode == b.mpNode;
	}


	template <typename T, typename PointerA, typename ReferenceA, typename PointerB, typename ReferenceB>
	inline bool operator!=(const rbtree_iterator<T, PointerA, ReferenceA>& a, 
						   const rbtree_iterator<T, PointerB, ReferenceB>& b)
	{
		return a.mpNode != b.mpNode;
	}


	// We provide a version of operator!= for the case where the iterators are of the 
	// same type. This helps prevent ambiguity errors in the presence of rel_ops.
	template <typename T, typename Pointer, typename Reference>
	inline bool operator!=(const rbtree_iterator<T, Pointer, Reference>& a, 
						   const rbtree_iterator<T, Pointer, Reference>& b)
	{
		return a.mpNode != b.mpNode;
	}




	///////////////////////////////////////////////////////////////////////
	// rbtree functions
	///////////////////////////////////////////////////////////////////////

	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline rbtree<K, V, C, A, E, bM, bU>::rbtree()
		// : mAnchormAnchor(),
		//   mnSize(0),
		//   mAllocator(EASTL_RBTREE_DEFAULT_NAME)
	{
		// reset_lose_memory();
	}


	// template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	// inline rbtree<K, V, C, A, E, bM, bU>::rbtree(const allocator_type& allocator)
	// 	: mAnchor(),
	// 	  mnSize(0),
	// 	  mAllocator(allocator)
	// {
	// 	reset_lose_memory();
	// }


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline rbtree<K, V, C, A, E, bM, bU>::rbtree(const C& compare/*, const allocator_type& allocator*/)
		: base_type(compare)/*,
		  mAnchor(),
		  mnSize(0),
		  mAllocator(allocator)*/
	{
//		reset_lose_memory();
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline rbtree<K, V, C, A, E, bM, bU>::rbtree(const this_type& x)
		: base_type(x.get_compare()),
		//   mAnchor(),
		//   mnSize(0),
		//   mAllocator(x.mAllocator)
	{
		// reset_lose_memory();

		if(x.get_root_node()) // mAnchor.mpNodeParent is the rb_tree root node.
		{
			mpNodeEnd->mpNodeLeft = DoCopySubtree(x.get_root_node(), get_end_node()); 

			mMinMaxNodes.mpMinChild = RBTreeGetMinChild<rbtree_soft0_ptr>(get_root_node());
			mMinMaxNodes.mpMaxChild = RBTreeGetMaxChild<rbtree_soft0_ptr>(get_root_node());

			mnSize               = x.mnSize;
		}
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline rbtree<K, V, C, A, E, bM, bU>::rbtree(this_type&& x)
		: base_type(x.get_compare()),
		//   mAnchor(),
		//   mnSize(0),
		//   mAllocator(x.mAllocator)
	{
		// reset_lose_memory();
		swap(x);
	}

	// template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	// inline rbtree<K, V, C, A, E, bM, bU>::rbtree(this_type&& x, const allocator_type& allocator)
	// 	: base_type(x.get_compare()),
	// 	  mAnchor(),
	// 	  mnSize(0),
	// 	  mAllocator(allocator)
	// {
	// 	reset_lose_memory();
	// 	swap(x); // swap will directly or indirectly handle the possibility that mAllocator != x.mAllocator.
	// }


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	template <typename InputIterator>
	inline rbtree<K, V, C, A, E, bM, bU>::rbtree(InputIterator first, InputIterator last, const C& compare/*, const allocator_type& allocator*/)
		: base_type(compare)//,
		//   mAnchor(),
		//   mnSize(0),
		//   mAllocator(allocator)
	{
//		reset_lose_memory();

		#if EASTL_EXCEPTIONS_ENABLED
			try
			{
		#endif
				for(; first != last; ++first)
					insert(std::move(*first));
		#if EASTL_EXCEPTIONS_ENABLED
			}
			catch(...)
			{
				clear();
				throw;
			}
		#endif
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline rbtree<K, V, C, A, E, bM, bU>::~rbtree()
	{
		// Erase the entire tree. DoNukeSubtree is not a 
		// conventional erase function, as it does no rebalancing.
		DoNukeSubtree(take_root_node());

		mpNodeEnd->set_as_deleted(); //not really needed as mpNodeEnd is already as a deleted
		// mb: mpNodeEnd will be automatically dealocated by owning_ptr
	}


	// template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	// inline const typename rbtree<K, V, C, A, E, bM, bU>::allocator_type&
	// rbtree<K, V, C, A, E, bM, bU>::get_allocator() const EA_NOEXCEPT
	// {
	// 	return mAllocator;
	// }


	// template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	// inline typename rbtree<K, V, C, A, E, bM, bU>::allocator_type&
	// rbtree<K, V, C, A, E, bM, bU>::get_allocator() EA_NOEXCEPT
	// {
	// 	return mAllocator;
	// }


	// template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	// inline void rbtree<K, V, C, A, E, bM, bU>::set_allocator(const allocator_type& allocator)
	// {
	// 	mAllocator = allocator;
	// }


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline typename rbtree<K, V, C, A, E, bM, bU>::size_type
	rbtree<K, V, C, A, E, bM, bU>::size() const EA_NOEXCEPT
		{ return mnSize; }


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline bool rbtree<K, V, C, A, E, bM, bU>::empty() const EA_NOEXCEPT
		{ return (mnSize == 0); }


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline typename rbtree<K, V, C, A, E, bM, bU>::iterator
	rbtree<K, V, C, A, E, bM, bU>::begin() EA_NOEXCEPT
		{ return iterator(get_begin_node(), get_end_node()); }


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline typename rbtree<K, V, C, A, E, bM, bU>::const_iterator
	rbtree<K, V, C, A, E, bM, bU>::begin() const EA_NOEXCEPT
		{ return const_iterator(get_begin_node(), get_end_node()); }


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline typename rbtree<K, V, C, A, E, bM, bU>::const_iterator
	rbtree<K, V, C, A, E, bM, bU>::cbegin() const EA_NOEXCEPT
		{ return const_iterator(get_begin_node(), get_end_node()); }


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline typename rbtree<K, V, C, A, E, bM, bU>::iterator
	rbtree<K, V, C, A, E, bM, bU>::end() EA_NOEXCEPT
		{ return iterator(get_end_node(), get_end_node()); }


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline typename rbtree<K, V, C, A, E, bM, bU>::const_iterator
	rbtree<K, V, C, A, E, bM, bU>::end() const EA_NOEXCEPT
		{ return const_iterator(get_end_node(), get_end_node()); }


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline typename rbtree<K, V, C, A, E, bM, bU>::const_iterator
	rbtree<K, V, C, A, E, bM, bU>::cend() const EA_NOEXCEPT
		{ return const_iterator(get_end_node(), get_end_node()); }


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline typename rbtree<K, V, C, A, E, bM, bU>::reverse_iterator
	rbtree<K, V, C, A, E, bM, bU>::rbegin() EA_NOEXCEPT
		{ return reverse_iterator(end()); }


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline typename rbtree<K, V, C, A, E, bM, bU>::const_reverse_iterator
	rbtree<K, V, C, A, E, bM, bU>::rbegin() const EA_NOEXCEPT
		{ return const_reverse_iterator(end()); }


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline typename rbtree<K, V, C, A, E, bM, bU>::const_reverse_iterator
	rbtree<K, V, C, A, E, bM, bU>::crbegin() const EA_NOEXCEPT
		{ return const_reverse_iterator(end()); }


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline typename rbtree<K, V, C, A, E, bM, bU>::reverse_iterator
	rbtree<K, V, C, A, E, bM, bU>::rend() EA_NOEXCEPT
		{ return reverse_iterator(begin()); }


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline typename rbtree<K, V, C, A, E, bM, bU>::const_reverse_iterator
	rbtree<K, V, C, A, E, bM, bU>::rend() const EA_NOEXCEPT
		{ return const_reverse_iterator(begin()); }


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline typename rbtree<K, V, C, A, E, bM, bU>::const_reverse_iterator
	rbtree<K, V, C, A, E, bM, bU>::crend() const EA_NOEXCEPT
		{ return const_reverse_iterator(begin()); }


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline typename rbtree<K, V, C, A, E, bM, bU>::this_type&
	rbtree<K, V, C, A, E, bM, bU>::operator=(const this_type& x)
	{
		if(this != &x)
		{
			clear();

			// #if EASTL_ALLOCATOR_COPY_ENABLED
			// 	mAllocator = x.mAllocator;
			// #endif

			get_compare() = x.get_compare();

			if(x.get_root_node()) // mAnchor.mpNodeParent is the rb_tree root node.
			{
				mpNodeEnd->mpNodeLeft = DoCopySubtree(x.get_root_node(), get_end_node())

				mMinMaxNodes.mpMinChild = RBTreeGetMinChild<rbtree_soft0_ptr>(get_root_node());
				mMinMaxNodes.mpMaxChild = RBTreeGetMaxChild<rbtree_soft0_ptr>(get_root_node());

				mnSize               = x.mnSize;
			}
		}
		return *this;
	}

	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline typename rbtree<K, V, C, A, E, bM, bU>::this_type&
	rbtree<K, V, C, A, E, bM, bU>::operator=(this_type&& x)
	{
		if(this != &x)
		{
			clear();        // To consider: Are we really required to clear here? x is going away soon and will clear itself in its dtor.
			swap(x);        // member swap handles the case that x has a different allocator than our allocator by doing a copy.
		}
		return *this; 
	}

	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline typename rbtree<K, V, C, A, E, bM, bU>::this_type&
	rbtree<K, V, C, A, E, bM, bU>::operator=(std::initializer_list<value_type> ilist)
	{
		// The simplest means of doing this is to clear and insert. There probably isn't a generic
		// solution that's any more efficient without having prior knowledge of the ilist contents.
		clear();

		for(typename std::initializer_list<value_type>::iterator it = ilist.begin(), itEnd = ilist.end(); it != itEnd; ++it)
			DoInsertValue(has_unique_keys_type(), std::move(*it));

		return *this;
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	void rbtree<K, V, C, A, E, bM, bU>::swap(this_type& x)
	{
		using namespace std;

	// #if EASTL_RBTREE_LEGACY_SWAP_BEHAVIOUR_REQUIRES_COPY_CTOR
	// 	if(mAllocator == x.mAllocator) // If allocators are equivalent...
	// #endif
	// 	{
			// Most of our members can be exchaged by a basic swap:
			// We leave mAllocator as-is.
			swap(mnSize,        x.mnSize);
			swap(get_compare(), x.get_compare());
		// #if !EASTL_RBTREE_LEGACY_SWAP_BEHAVIOUR_REQUIRES_COPY_CTOR
		// 	swap(mAllocator,          x.mAllocator);
		// #endif

			//mb: we swap also the anchor.
			// first, our anchor in on the heap so we can freely swap it.
			// second, the anchor is also used to match an iterator to its tree of origin
			// so we actually MUST swap it 
			swap(mpNodeEnd, x.mpNodeEnd);
			swap(mMinMaxNodes, x.mMinMaxNodes);

			// // However, because our anchor node is a part of our class instance and not 
			// // dynamically allocated, we can't do a swap of it but must do a more elaborate
			// // procedure. This is the downside to having the mAnchor be like this, but 
			// // otherwise we consider it a good idea to avoid allocating memory for a 
			// // nominal container instance.

			// // We optimize for the expected most common case: both pointers being non-null.
			// if(mAnchor.mpNodeParent && x.mAnchor.mpNodeParent) // If both pointers are non-null...
			// {
			// 	swap(mAnchor.mpNodeRight,  x.mAnchor.mpNodeRight);
			// 	swap(mAnchor.mpNodeLeft,   x.mAnchor.mpNodeLeft);
			// 	swap(mAnchor.mpNodeParent, x.mAnchor.mpNodeParent);

			// 	// We need to fix up the anchors to point to themselves (we can't just swap them).
			// 	mAnchor.mpNodeParent->mpNodeParent   = &mAnchor;
			// 	x.mAnchor.mpNodeParent->mpNodeParent = &x.mAnchor;
			// }
			// else if(mAnchor.mpNodeParent)
			// {
			// 	x.mAnchor.mpNodeRight  = mAnchor.mpNodeRight;
			// 	x.mAnchor.mpNodeLeft   = mAnchor.mpNodeLeft;
			// 	x.mAnchor.mpNodeParent = mAnchor.mpNodeParent;
			// 	x.mAnchor.mpNodeParent->mpNodeParent = &x.mAnchor;

			// 	// We need to fix up our anchor to point it itself (we can't have it swap with x).
			// 	mAnchor.mpNodeRight  = &mAnchor;
			// 	mAnchor.mpNodeLeft   = &mAnchor;
			// 	mAnchor.mpNodeParent = NULL;
			// }
			// else if(x.mAnchor.mpNodeParent)
			// {
			// 	mAnchor.mpNodeRight  = x.mAnchor.mpNodeRight;
			// 	mAnchor.mpNodeLeft   = x.mAnchor.mpNodeLeft;
			// 	mAnchor.mpNodeParent = x.mAnchor.mpNodeParent;
			// 	mAnchor.mpNodeParent->mpNodeParent = &mAnchor;

			// 	// We need to fix up x's anchor to point it itself (we can't have it swap with us).
			// 	x.mAnchor.mpNodeRight  = &x.mAnchor;
			// 	x.mAnchor.mpNodeLeft   = &x.mAnchor;
			// 	x.mAnchor.mpNodeParent = NULL;
			// } // Else both are NULL and there is nothing to do.

		// }
	// #if EASTL_RBTREE_LEGACY_SWAP_BEHAVIOUR_REQUIRES_COPY_CTOR
	// 	else
	// 	{
	// 		const this_type temp(*this); // Can't call eastl::swap because that would
	// 		*this = x;                   // itself call this member swap function.
	// 		x     = temp;
	// 	}
	// #endif
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	template <class... Args>
	inline typename rbtree<K, V, C, A, E, bM, bU>::insert_return_type // map/set::insert return a pair, multimap/multiset::iterator return an iterator.
	rbtree<K, V, C, A, E, bM, bU>::emplace(Args&&... args)
	{
		return DoInsertValue(has_unique_keys_type(), std::forward<Args>(args)...);
	}

	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	template <class... Args> 
	typename rbtree<K, V, C, A, E, bM, bU>::iterator
	rbtree<K, V, C, A, E, bM, bU>::emplace_hint(const_iterator position, Args&&... args)
	{
		return DoInsertValueHint(has_unique_keys_type(), position, std::forward<Args>(args)...);
	}

	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	template <class... Args>
	inline std::pair<typename rbtree<K, V, C, A, E, bM, bU>::iterator, bool>
	rbtree<K, V, C, A, E, bM, bU>::try_emplace(const key_type& key, Args&&... args)
	{
		return DoInsertValue(has_unique_keys_type(), piecewise_construct, forward_as_tuple(key), forward_as_tuple(forward<Args>(args)...));
	}

	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	template <class... Args>
	inline std::pair<typename rbtree<K, V, C, A, E, bM, bU>::iterator, bool>
	rbtree<K, V, C, A, E, bM, bU>::try_emplace(key_type&& key, Args&&... args)
	{
		return DoInsertValue(has_unique_keys_type(), piecewise_construct, forward_as_tuple(std::move(key)), forward_as_tuple(forward<Args>(args)...));
	}

	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	template <class... Args>
	inline typename rbtree<K, V, C, A, E, bM, bU>::iterator
	rbtree<K, V, C, A, E, bM, bU>::try_emplace(const_iterator position, const key_type& key, Args&&... args)
	{
		return DoInsertValueHint(
		    has_unique_keys_type(), position,
		    value_type(piecewise_construct, forward_as_tuple(key), forward_as_tuple(forward<Args>(args)...)));
	}

	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	template <class... Args>
	inline typename rbtree<K, V, C, A, E, bM, bU>::iterator
	rbtree<K, V, C, A, E, bM, bU>::try_emplace(const_iterator position, key_type&& key, Args&&... args)
	{
		return DoInsertValueHint(
		    has_unique_keys_type(), position,
		    value_type(piecewise_construct, forward_as_tuple(key), forward_as_tuple(forward<Args>(args)...)));
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	template <class P, class>
	inline typename rbtree<K, V, C, A, E, bM, bU>::insert_return_type // map/set::insert return a pair, multimap/multiset::iterator return an iterator.
	rbtree<K, V, C, A, E, bM, bU>::insert(P&& otherValue)
	{ 
		// Need to use forward instead of move because P&& is a "universal reference" instead of an rvalue reference.
		return emplace(std::forward<P>(otherValue));
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline typename rbtree<K, V, C, A, E, bM, bU>::iterator 
	rbtree<K, V, C, A, E, bM, bU>::insert(const_iterator position, value_type&& value)
	{
		return DoInsertValueHint(has_unique_keys_type(), position, value_type(std::move(value)));
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline typename rbtree<K, V, C, A, E, bM, bU>::insert_return_type // map/set::insert return a pair, multimap/multiset::iterator return an iterator.
	rbtree<K, V, C, A, E, bM, bU>::insert(const value_type& value)
	{
		return DoInsertValue(has_unique_keys_type(), value);
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	typename rbtree<K, V, C, A, E, bM, bU>::iterator
	rbtree<K, V, C, A, E, bM, bU>::insert(const_iterator position, const value_type& value)
	{
		return DoInsertValueHint(has_unique_keys_type(), position, value);
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	template <class M>
	std::pair<typename rbtree<K, V, C, A, E, bM, bU>::iterator, bool>
	rbtree<K, V, C, A, E, bM, bU>::insert_or_assign(const key_type& k, M&& obj)
	{
		auto iter = find(k);

		if(iter == end())
		{
			return insert(value_type(piecewise_construct, forward_as_tuple(k), forward_as_tuple(std::forward<M>(obj))));
		}
		else
		{
			iter->second = std::forward<M>(obj);
			return {iter, false};
		}
	}

	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	template <class M>
	std::pair<typename rbtree<K, V, C, A, E, bM, bU>::iterator, bool>
	rbtree<K, V, C, A, E, bM, bU>::insert_or_assign(key_type&& k, M&& obj)
	{
		auto iter = find(k);

		if(iter == end())
		{
			return insert(value_type(std::piecewise_construct, std::forward_as_tuple(std::move(k)), std::forward_as_tuple(std::forward<M>(obj))));
		}
		else
		{
			iter->second = std::forward<M>(obj);
			return {iter, false};
		}
	}

	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	template <class M>
	typename rbtree<K, V, C, A, E, bM, bU>::iterator
	rbtree<K, V, C, A, E, bM, bU>::insert_or_assign(const_iterator hint, const key_type& k, M&& obj)
	{
		auto iter = find(k);

		if(iter == end())
		{
			return insert(hint, value_type(std::piecewise_construct, std::forward_as_tuple(k), std::forward_as_tuple(std::forward<M>(obj))));
		}
		else
		{
			iter->second = std::forward<M>(obj);
			return iter;
		}
	}

	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	template <class M>
	typename rbtree<K, V, C, A, E, bM, bU>::iterator
	rbtree<K, V, C, A, E, bM, bU>::insert_or_assign(const_iterator hint, key_type&& k, M&& obj)
	{
		auto iter = find(k);

		if(iter == end())
		{
			return insert(hint, value_type(std::piecewise_construct, std::forward_as_tuple(std::move(k)), std::forward_as_tuple(std::forward<M>(obj))));
		}
		else
		{
			iter->second = std::forward<M>(obj);
			return iter;
		}
	}

	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	typename rbtree<K, V, C, A, E, bM, bU>::rbtree_soft0_ptr
	rbtree<K, V, C, A, E, bM, bU>::DoGetKeyInsertionPositionUniqueKeys(bool& canInsert, const key_type& key)
	{
		// This code is essentially a slightly modified copy of the the rbtree::insert 
		// function whereby this version takes a key and not a full value_type.
		extract_key extractKey;

		rbtree_soft0_ptr pCurrent    = get_root_node(); // Start with the root node.
		rbtree_soft0_ptr pLowerBound = get_end_node();             // Set it to the container end for now.
		rbtree_soft0_ptr pParent;                                        // This will be where we insert the new node.

		bool bValueLessThanNode = true; // If the tree is empty, this will result in an insertion at the front.

		// Find insertion position of the value. This will either be a position which 
		// already contains the value, a position which is greater than the value or
		// end(), which we treat like a position which is greater than the value.
		while(EASTL_LIKELY(pCurrent)) // Do a walk down the tree.
		{
			bValueLessThanNode = compare(key, extractKey(pCurrent->mValue));
			pLowerBound        = pCurrent;

			if(bValueLessThanNode)
			{
				EASTL_VALIDATE_COMPARE(!compare(extractKey(pCurrent->mValue), key)); // Validate that the compare function is sane.
				pCurrent = pCurrent->get_node_left();
			}
			else
				pCurrent = pCurrent->get_node_right();
		}

		pParent = pLowerBound; // pLowerBound is actually upper bound right now (i.e. it is > value instead of <=), but we will make it the lower bound below.

		if(bValueLessThanNode) // If we ended up on the left side of the last parent node...
		{
			if(EASTL_LIKELY(pLowerBound != get_begin_node())) // If the tree was empty or if we otherwise need to insert at the very front of the tree...
			{
				// At this point, pLowerBound points to a node which is > than value.
				// Move it back by one, so that it points to a node which is <= value.
				pLowerBound = RBTreeDecrement<rbtree_soft0_ptr, rbtree_soft_ptr>(pLowerBound.get());
			}
			else
			{
				canInsert = true;
				return pLowerBound;
			}
		}

		// Since here we require values to be unique, we will do nothing if the value already exists.
		if(compare(extractKey(pLowerBound->mValue), key)) // If the node is < the value (i.e. if value is >= the node)...
		{
			EASTL_VALIDATE_COMPARE(!compare(key, extractKey(pLowerBound->mValue))); // Validate that the compare function is sane.
			canInsert = true;
			return pParent;
		}

		// The item already exists (as found by the compare directly above), so return false.
		canInsert = false;
		return pLowerBound;
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	typename rbtree<K, V, C, A, E, bM, bU>::rbtree_soft0_ptr
	rbtree<K, V, C, A, E, bM, bU>::DoGetKeyInsertionPositionNonuniqueKeys(const key_type& key)
	{
		// This is the pathway for insertion of non-unique keys (multimap and multiset, but not map and set).
		rbtree_soft0_ptr pCurrent  = get_root_node(); // Start with the root node.
		rbtree_soft0_ptr pRangeEnd = get_end_node();             // Set it to the container end for now.
		extract_key extractKey;

		while(pCurrent)
		{
			pRangeEnd = pCurrent;

			if(compare(key, extractKey(pCurrent->mValue)))
			{
				EASTL_VALIDATE_COMPARE(!compare(extractKey(pCurrent->mValue), key)); // Validate that the compare function is sane.
				pCurrent = pCurrent->get_node_left();
			}
			else
				pCurrent = pCurrent->get_node_right();
		}

		return pRangeEnd;
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	std::pair<typename rbtree<K, V, C, A, E, bM, bU>::iterator, bool> 
	rbtree<K, V, C, A, E, bM, bU>::DoInsertValue(std::true_type, value_type&& value)
	{
		extract_key extractKey;
		key_type    key(extractKey(value));
		bool        canInsert;
		rbtree_soft0_ptr  pPosition = DoGetKeyInsertionPositionUniqueKeys(canInsert, key);

		if(canInsert)
		{
			const iterator itResult(DoInsertValueImpl(pPosition, false, key, std::move(value)));
			return std::pair<iterator, bool>(itResult, true);
		}

		return std::pair<iterator, bool>(iterator(pPosition, get_end_node()), false);
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	typename rbtree<K, V, C, A, E, bM, bU>::iterator 
	rbtree<K, V, C, A, E, bM, bU>::DoInsertValue(std::false_type, value_type&& value)
	{
		extract_key extractKey;
		key_type    key(extractKey(value));
		rbtree_soft0_ptr  pPosition = DoGetKeyInsertionPositionNonuniqueKeys(key);

		return DoInsertValueImpl(pPosition, false, key, std::move(value));
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	template <class... Args>
	std::pair<typename rbtree<K, V, C, A, E, bM, bU>::iterator, bool>
	rbtree<K, V, C, A, E, bM, bU>::DoInsertValue(std::true_type, Args&&... args) // true_type means keys are unique.
	{
		// This is the pathway for insertion of unique keys (map and set, but not multimap and multiset).
		// Note that we return a pair and not an iterator. This is because the C++ standard for map
		// and set is to return a pair and not just an iterator.

		rbtree_owning_ptr pNodeNew = DoCreateNode(std::forward<Args>(args)...); // Note that pNodeNew->mpLeft, mpRight, mpParent, will be uninitialized.
		const key_type& key = extract_key{}(pNodeNew->mValue);

		bool        canInsert;
		rbtree_soft0_ptr  pPosition = DoGetKeyInsertionPositionUniqueKeys(canInsert, key);

		if(canInsert)
		{
			iterator itResult(DoInsertValueImpl(pPosition, false, key, std::move(pNodeNew)));
			return std::pair<iterator, bool>(itResult, true);
		}

		DoFreeNode(std::move(pNodeNew));
		return std::pair<iterator, bool>(iterator(pPosition, get_end_node()), false);
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	template <class... Args>
	typename rbtree<K, V, C, A, E, bM, bU>::iterator
	rbtree<K, V, C, A, E, bM, bU>::DoInsertValue(std::false_type, Args&&... args) // false_type means keys are not unique.
	{
		// We have a problem here if sizeof(value_type) is too big for the stack. We may want to consider having a specialization for large value_types.
		// To do: Change this so that we call DoCreateNode(eastl::forward<Args>(args)...) here and use the value from the resulting pNode to get the 
		// key, and make DoInsertValueImpl take that node as an argument. That way there is no value created on the stack.

		rbtree_owning_ptr pNodeNew = DoCreateNode(std::forward<Args>(args)...); // Note that pNodeNew->mpLeft, mpRight, mpParent, will be uninitialized.
		const key_type& key = extract_key{}(pNodeNew->mValue);

		rbtree_soft0_ptr pPosition = DoGetKeyInsertionPositionNonuniqueKeys(key);

		return DoInsertValueImpl(pPosition, false, key, std::move(pNodeNew));
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	template <class... Args>
	typename rbtree<K, V, C, A, E, bM, bU>::iterator
	rbtree<K, V, C, A, E, bM, bU>::DoInsertValueImpl(rbtree_soft0_ptr pNodeParent, bool bForceToLeft, const key_type& key, Args&&... args)
	{
		rbtree_owning_ptr pNodeNew = DoCreateNode(std::forward<Args>(args)...); // Note that pNodeNew->mpLeft, mpRight, mpParent, will be uninitialized.

		return DoInsertValueImpl(pNodeParent, bForceToLeft, key, std::move(pNodeNew));
	}

	
	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	typename rbtree<K, V, C, A, E, bM, bU>::iterator
	rbtree<K, V, C, A, E, bM, bU>::DoInsertValueImpl(rbtree_soft0_ptr pNodeParent, bool bForceToLeft, const key_type& key, rbtree_owning_ptr pNodeNew)
	{
		EASTL_ASSERT_MSG(pNodeNew != nullptr, "node to insert to the rbtree must not be null");

		RBTreeSide  side;
		extract_key extractKey;

		// The reason we may want to have bForceToLeft == true is that pNodeParent->mValue and value may be equal.
		// In that case it doesn't matter what side we insert on, except that the C++ LWG #233 improvement report
		// suggests that we should use the insert hint position to force an ordering. So that's what we do.
		if(bForceToLeft || (pNodeParent == get_end_node()) || compare(key, extractKey(pNodeParent->mValue)))
			side = kRBTreeSideLeft;
		else
			side = kRBTreeSideRight;

		RBTreeInsert<rbtree_soft0_ptr, rbtree_owning_ptr>(std::move(pNodeNew), pNodeParent, mpNodeEnd, mMinMaxNodes, side);
		mnSize++;

		return iterator(pNodeNew, get_end_node());
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	std::pair<typename rbtree<K, V, C, A, E, bM, bU>::iterator, bool>
	rbtree<K, V, C, A, E, bM, bU>::DoInsertKey(std::true_type, const key_type& key) // true_type means keys are unique.
	{
		// This is the pathway for insertion of unique keys (map and set, but not multimap and multiset).
		// Note that we return a pair and not an iterator. This is because the C++ standard for map
		// and set is to return a pair and not just an iterator.
		bool       canInsert;
		rbtree_soft0_ptr pPosition = DoGetKeyInsertionPositionUniqueKeys(canInsert, key);

		if(canInsert)
		{
			const iterator itResult(DoInsertKeyImpl(pPosition, false, key));
			return std::pair<iterator, bool>(itResult, true);
		}

		return std::pair<iterator, bool>(iterator(pPosition, get_end_node()), false);
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	typename rbtree<K, V, C, A, E, bM, bU>::iterator
	rbtree<K, V, C, A, E, bM, bU>::DoInsertKey(std::false_type, const key_type& key) // false_type means keys are not unique.
	{
		rbtree_soft0_ptr pPosition = DoGetKeyInsertionPositionNonuniqueKeys(key);

		return DoInsertKeyImpl(pPosition, false, key);
	}



	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	typename rbtree<K, V, C, A, E, bM, bU>::rbtree_soft0_ptr
	rbtree<K, V, C, A, E, bM, bU>::DoGetKeyInsertionPositionUniqueKeysHint(const_iterator position, bool& bForceToLeft, const key_type& key)
	{
		extract_key extractKey;

		if((position.get_node() != mMinMaxNodes.mpMaxChild) && (position.get_node() != get_end_node())) // If the user specified a specific insertion position...
		{
			// iterator itNext(position.get_node(), position.get_end_node());
			// ++itNext;

			//mb: TODO check safety of this change
			rbtree_soft0_ptr mpNext = RBTreeIncrement<rbtree_soft0_ptr, rbtree_soft_ptr>(position.get_node());

			// To consider: Change this so that 'position' specifies the position after 
			// where the insertion goes and not the position before where the insertion goes.
			// Doing so would make this more in line with user expectations and with LWG #233.
			const bool bPositionLessThanValue = compare(extractKey(position.mpNode->mValue), key);

			if(bPositionLessThanValue) // If (value > *position)...
			{
				EASTL_VALIDATE_COMPARE(!compare(key, extractKey(position.mpNode->mValue))); // Validate that the compare function is sane.

				const bool bValueLessThanNext = compare(key, extractKey(mpNext->mValue));

				if(bValueLessThanNext) // If value < *itNext...
				{
					EASTL_VALIDATE_COMPARE(!compare(extractKey(mpNext->mValue), key)); // Validate that the compare function is sane.

					if(position.mpNode->mpNodeRight)
					{
						bForceToLeft = true; // Specifically insert in front of (to the left of) itNext (and thus after 'position').
						return mpNext;
					}

					bForceToLeft = false;
					return mpNext;
				}
			}

			bForceToLeft = false;
			return NULL;  // The above specified hint was not useful, then we do a regular insertion.
		}

		if(mnSize && compare(extractKey(get_last_node()->mValue), key))
		{
			EASTL_VALIDATE_COMPARE(!compare(key, extractKey(get_last_node()->mValue))); // Validate that the compare function is sane.
			bForceToLeft = false;
			return get_last_node();
		}

		bForceToLeft = false;
		return NULL; // The caller can do a default insert.
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	typename rbtree<K, V, C, A, E, bM, bU>::rbtree_soft0_ptr
	rbtree<K, V, C, A, E, bM, bU>::DoGetKeyInsertionPositionNonuniqueKeysHint(const_iterator position, bool& bForceToLeft, const key_type& key)
	{
		extract_key extractKey;

		if((position.get_node() != mMinMaxNodes.mpMaxChild) && (position.get_node() != get_end_node())) // If the user specified a specific insertion position...
		{
			// iterator itNext(position.get_node(),);
			// ++itNext;

			//mb: TODO check safety of this change
			rbtree_soft0_ptr mpNext = RBTreeIncrement<rbtree_soft0_ptr, rbtree_soft_ptr>(position.get_node());

			// To consider: Change this so that 'position' specifies the position after 
			// where the insertion goes and not the position before where the insertion goes.
			// Doing so would make this more in line with user expectations and with LWG #233.
			if(!compare(key, extractKey(position.mpNode->mValue)) && // If value >= *position && 
			   !compare(extractKey(mpNext->mValue), key))     // if value <= *itNext...
			{
				if(position.mpNode->mpNodeRight) // If there are any nodes to the right... [this expression will always be true as long as we aren't at the end()]
				{
					bForceToLeft = true; // Specifically insert in front of (to the left of) itNext (and thus after 'position').
					return mpNext;
				}

				bForceToLeft = false;
				return mpNext;
			}

			bForceToLeft = false;
			return NULL; // The above specified hint was not useful, then we do a regular insertion.
		}

		// This pathway shouldn't be commonly executed, as the user shouldn't be calling 
		// this hinted version of insert if the user isn't providing a useful hint.
		if(mnSize && !compare(key, extractKey(get_last_node()->mValue))) // If we are non-empty and the value is >= the last node...
		{
			bForceToLeft =false;
			return get_last_node();
		}

		bForceToLeft = false;
		return NULL;
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	typename rbtree<K, V, C, A, E, bM, bU>::iterator
	rbtree<K, V, C, A, E, bM, bU>::DoInsertValueHint(std::true_type, const_iterator position, const value_type& value) // true_type means keys are unique.
	{
		// This is the pathway for insertion of unique keys (map and set, but not multimap and multiset).
		//
		// We follow the same approach as SGI STL/STLPort and use the position as
		// a forced insertion position for the value when possible.
		check_iterator(position);

		extract_key extractKey;
		key_type    key(extractKey(value));
		bool        bForceToLeft;
		rbtree_soft0_ptr  pPosition = DoGetKeyInsertionPositionUniqueKeysHint(position, bForceToLeft, key);

		if(pPosition)
			return DoInsertValueImpl(pPosition, bForceToLeft, key, value);
		else
			return DoInsertValue(has_unique_keys_type(), value).first;
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	typename rbtree<K, V, C, A, E, bM, bU>::iterator
	rbtree<K, V, C, A, E, bM, bU>::DoInsertValueHint(std::false_type, const_iterator position, const value_type& value) // false_type means keys are not unique.
	{
		// This is the pathway for insertion of non-unique keys (multimap and multiset, but not map and set).
		//
		// We follow the same approach as SGI STL/STLPort and use the position as
		// a forced insertion position for the value when possible.
		check_iterator(position);

		extract_key extractKey;
		key_type    key(extractKey(value));
		bool        bForceToLeft;
		rbtree_soft0_ptr  pPosition = DoGetKeyInsertionPositionNonuniqueKeysHint(position, bForceToLeft, key);

		if(pPosition)
			return DoInsertValueImpl(pPosition, bForceToLeft, key, value);
		else
			return DoInsertValue(has_unique_keys_type(), value);
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	typename rbtree<K, V, C, A, E, bM, bU>::iterator
	rbtree<K, V, C, A, E, bM, bU>::DoInsertKey(std::true_type, const_iterator position, const key_type& key) // true_type means keys are unique.
	{
		check_iterator(position);

		bool       bForceToLeft;
		rbtree_soft0_ptr pPosition = DoGetKeyInsertionPositionUniqueKeysHint(position, bForceToLeft, key);

		if(pPosition)
			return DoInsertKeyImpl(pPosition, bForceToLeft, key);
		else
			return DoInsertKey(has_unique_keys_type(), key).first;
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	typename rbtree<K, V, C, A, E, bM, bU>::iterator
	rbtree<K, V, C, A, E, bM, bU>::DoInsertKey(std::false_type, const_iterator position, const key_type& key) // false_type means keys are not unique.
	{
		// This is the pathway for insertion of non-unique keys (multimap and multiset, but not map and set).
		//
		// We follow the same approach as SGI STL/STLPort and use the position as
		// a forced insertion position for the value when possible.
		check_iterator(position);

		bool       bForceToLeft;
		rbtree_soft0_ptr pPosition = DoGetKeyInsertionPositionNonuniqueKeysHint(position, bForceToLeft, key);

		if(pPosition)
			return DoInsertKeyImpl(pPosition, bForceToLeft, key);
		else
			return DoInsertKey(has_unique_keys_type(), key); // We are empty or we are inserting at the end.
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	typename rbtree<K, V, C, A, E, bM, bU>::iterator
	rbtree<K, V, C, A, E, bM, bU>::DoInsertKeyImpl(rbtree_soft0_ptr pNodeParent, bool bForceToLeft, const key_type& key)
	{
		RBTreeSide  side;
		extract_key extractKey;

		// The reason we may want to have bForceToLeft == true is that pNodeParent->mValue and value may be equal.
		// In that case it doesn't matter what side we insert on, except that the C++ LWG #233 improvement report
		// suggests that we should use the insert hint position to force an ordering. So that's what we do.
		if(bForceToLeft || (pNodeParent == get_end_node()) || compare(key, extractKey(pNodeParent->mValue)))
			side = kRBTreeSideLeft;
		else
			side = kRBTreeSideRight;

		rbtree_owning_ptr pNodeNew = DoCreateNodeFromKey(key);
		rbtree_soft0_ptr pNodeNew2 = pNodeNew;
		RBTreeInsert<rbtree_soft0_ptr, rbtree_owning_ptr>(std::move(pNodeNew), pNodeParent, mpNodeEnd, mMinMaxNodes, side);
		mnSize++;

		return iterator(pNodeNew2, get_end_node());
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	void rbtree<K, V, C, A, E, bM, bU>::insert(std::initializer_list<value_type> ilist)
	{
		for(typename std::initializer_list<value_type>::iterator it = ilist.begin(), itEnd = ilist.end(); it != itEnd; ++it)
			DoInsertValue(has_unique_keys_type(), std::move(*it));
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	template <typename InputIterator>
	void rbtree<K, V, C, A, E, bM, bU>::insert(InputIterator first, InputIterator last)
	{
		for( ; first != last; ++first)
			DoInsertValue(has_unique_keys_type(), *first); // Or maybe we should call 'insert(end(), *first)' instead. If the first-last range was sorted then this might make some sense.
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline void rbtree<K, V, C, A, E, bM, bU>::clear()
	{
		// Erase the entire tree. DoNukeSubtree is not a 
		// conventional erase function, as it does no rebalancing.
		DoNukeSubtree(take_root_node());

		// reset_lose_memory();
		mMinMaxNodes.mpMinChild = get_end_node();
		mMinMaxNodes.mpMaxChild = get_end_node();	
		mnSize = 0;
	}


	// template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	// inline void rbtree<K, V, C, A, E, bM, bU>::reset_lose_memory()
	// {
	// 	// The reset_lose_memory function is a special extension function which unilaterally 
	// 	// resets the container to an empty state without freeing the memory of 
	// 	// the contained objects. This is useful for very quickly tearing down a 
	// 	// container built into scratch memory.
	// 	mpAnchor->mpNodeRight  = mpAnchor;
	// 	mpAnchor->mpNodeLeft   = mpAnchor;
	// 	mpAnchor->mpNodeParent = NULL;
	// 	mpAnchor->mColor       = kRBTreeColorRed;
	// 	mnSize               = 0;
	// }


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline typename rbtree<K, V, C, A, E, bM, bU>::iterator
	rbtree<K, V, C, A, E, bM, bU>::erase(const_iterator position)
	{
		check_iterator(position);

		const iterator iErase(position.get_node(), position.get_end_node());
		--mnSize; // Interleave this between the two references to itNext. We expect no exceptions to occur during the code below.
		++position;
		rbtree_owning_ptr pNode = RBTreeErase<rbtree_soft0_ptr, rbtree_soft_ptr, rbtree_owning_ptr>(iErase.get_node(), mpNodeEnd, mMinMaxNodes);
		DoFreeNode(std::move(pNode));
		return iterator(position.get_node(), get_end_node());
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	typename rbtree<K, V, C, A, E, bM, bU>::iterator
	rbtree<K, V, C, A, E, bM, bU>::erase(const_iterator first, const_iterator last)
	{
		check_iterator(first);
		check_iterator(last);

		// We expect that if the user means to clear the container, they will call clear.
		if(EASTL_LIKELY((first.get_node() != mMinMaxNodes.mpMinChild) || (last.get_node() != get_end_node()))) // If (first != begin or last != end) ...
		{
			// Basic implementation:
			while(first != last)
				first = erase(first);
			return iterator(first.get_node(), get_end_node());

			// Inlined implementation:
			//size_type n = 0;
			//while(first != last)
			//{
			//    const iterator itErase(first);
			//    ++n;
			//    ++first;
			//    RBTreeErase(itErase.mpNode, &mAnchor);
			//    DoFreeNode(itErase.mpNode);
			//}
			//mnSize -= n;
			//return first;
		}

		clear();
		return iterator(get_end_node()), get_end_node(); // Same as: return end();
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline typename rbtree<K, V, C, A, E, bM, bU>::reverse_iterator
	rbtree<K, V, C, A, E, bM, bU>::erase(const_reverse_iterator position)
	{
		return reverse_iterator(erase((++position).base()));
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	typename rbtree<K, V, C, A, E, bM, bU>::reverse_iterator
	rbtree<K, V, C, A, E, bM, bU>::erase(const_reverse_iterator first, const_reverse_iterator last)
	{
		// Version which erases in order from first to last.
		// difference_type i(first.base() - last.base());
		// while(i--)
		//     first = erase(first);
		// return first;

		// Version which erases in order from last to first, but is slightly more efficient:
		return reverse_iterator(erase((++last).base(), (++first).base()));
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline void rbtree<K, V, C, A, E, bM, bU>::erase(const key_type* first, const key_type* last)
	{
		// We have no choice but to run a loop like this, as the first/last range could
		// have values that are discontiguously located in the tree. And some may not 
		// even be in the tree.
		while(first != last)
			erase(*first++);
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	typename rbtree<K, V, C, A, E, bM, bU>::iterator
	rbtree<K, V, C, A, E, bM, bU>::find(const key_type& key)
	{
		// To consider: Implement this instead via calling lower_bound and 
		// inspecting the result. The following is an implementation of this:
		//    const iterator it(lower_bound(key));
		//    return ((it.mpNode == &mAnchor) || compare(key, extractKey(it.mpNode->mValue))) ? iterator(&mAnchor) : it;
		// We don't currently implement the above because in practice people tend to call 
		// find a lot with trees, but very uncommonly call lower_bound.
		extract_key extractKey;

		rbtree_soft0_ptr pCurrent  = get_root_node(); // Start with the root node.
		rbtree_soft0_ptr pRangeEnd = get_end_node();             // Set it to the container end for now.

		while(EASTL_LIKELY(pCurrent)) // Do a walk down the tree.
		{
			if(EASTL_LIKELY(!compare(extractKey(pCurrent->mValue), key))) // If pCurrent is >= key...
			{
				pRangeEnd = pCurrent;
				pCurrent  = pCurrent->get_node_left();
			}
			else
			{
				EASTL_VALIDATE_COMPARE(!compare(key, extractKey(pCurrent->mValue))); // Validate that the compare function is sane.
				pCurrent  = pCurrent->get_node_right();
			}
		}

		if(EASTL_LIKELY((pRangeEnd != get_end_node()) && !compare(key, extractKey(pRangeEnd->mValue))))
			return iterator(pRangeEnd, get_end_node());
		return iterator(get_end_node(), get_end_node());
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline typename rbtree<K, V, C, A, E, bM, bU>::const_iterator
	rbtree<K, V, C, A, E, bM, bU>::find(const key_type& key) const
	{
		typedef rbtree<K, V, C, A, E, bM, bU> rbtree_type;
		return const_iterator(const_cast<rbtree_type*>(this)->find(key), get_end_node());
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	template <typename U, typename Compare2>
	typename rbtree<K, V, C, A, E, bM, bU>::iterator
	rbtree<K, V, C, A, E, bM, bU>::find_as(const U& u, Compare2 compare2)
	{
		extract_key extractKey;

		rbtree_soft0_ptr pCurrent  = get_root_node(); // Start with the root node.
		rbtree_soft0_ptr pRangeEnd = get_end_node();             // Set it to the container end for now.

		while(EASTL_LIKELY(pCurrent)) // Do a walk down the tree.
		{
			if(EASTL_LIKELY(!compare2(extractKey(pCurrent->mValue), u))) // If pCurrent is >= u...
			{
				pRangeEnd = pCurrent;
				pCurrent  = pCurrent->get_node_left();
			}
			else
			{
				EASTL_VALIDATE_COMPARE(!compare2(u, extractKey(pCurrent->mValue))); // Validate that the compare function is sane.
				pCurrent  = pCurrent->get_node_right();
			}
		}

		if(EASTL_LIKELY((pRangeEnd != get_end_node()) && !compare2(u, extractKey(pRangeEnd->mValue))))
			return iterator(pRangeEnd, get_end_node());
		return iterator(get_end_node(), get_end_node());
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	template <typename U, typename Compare2>
	inline typename rbtree<K, V, C, A, E, bM, bU>::const_iterator
	rbtree<K, V, C, A, E, bM, bU>::find_as(const U& u, Compare2 compare2) const
	{
		typedef rbtree<K, V, C, A, E, bM, bU> rbtree_type;
		return const_iterator(const_cast<rbtree_type*>(this)->find_as(u, compare2), get_end_node());
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	typename rbtree<K, V, C, A, E, bM, bU>::iterator
	rbtree<K, V, C, A, E, bM, bU>::lower_bound(const key_type& key)
	{
		extract_key extractKey;

		rbtree_soft0_ptr pCurrent  = get_root_node(); // Start with the root node.
		rbtree_soft0_ptr pRangeEnd = get_end_node();             // Set it to the container end for now.

		while(EASTL_LIKELY(pCurrent)) // Do a walk down the tree.
		{
			if(EASTL_LIKELY(!compare(extractKey(pCurrent->mValue), key))) // If pCurrent is >= key...
			{
				pRangeEnd = pCurrent;
				pCurrent  = pCurrent->get_node_left();
			}
			else
			{
				EASTL_VALIDATE_COMPARE(!compare(key, extractKey(pCurrent->mValue))); // Validate that the compare function is sane.
				pCurrent  = pCurrent->get_node_right();
			}
		}

		return iterator(pRangeEnd, get_end_node());
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline typename rbtree<K, V, C, A, E, bM, bU>::const_iterator
	rbtree<K, V, C, A, E, bM, bU>::lower_bound(const key_type& key) const
	{
		typedef rbtree<K, V, C, A, E, bM, bU> rbtree_type;
		return const_iterator(const_cast<rbtree_type*>(this)->lower_bound(key), get_end_node());
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	typename rbtree<K, V, C, A, E, bM, bU>::iterator
	rbtree<K, V, C, A, E, bM, bU>::upper_bound(const key_type& key)
	{
		extract_key extractKey;

		rbtree_soft0_ptr pCurrent  = get_root_node(); // Start with the root node.
		rbtree_soft0_ptr pRangeEnd = get_end_node();             // Set it to the container end for now.

		while(EASTL_LIKELY(pCurrent)) // Do a walk down the tree.
		{
			if(EASTL_LIKELY(compare(key, extractKey(pCurrent->mValue)))) // If key is < pCurrent...
			{
				EASTL_VALIDATE_COMPARE(!compare(extractKey(pCurrent->mValue), key)); // Validate that the compare function is sane.
				pRangeEnd = pCurrent;
				pCurrent  = pCurrent->get_node_right();
			}
			else
				pCurrent  = pCurrent->get_node_right();
		}

		return iterator(pRangeEnd, get_end_node());
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline typename rbtree<K, V, C, A, E, bM, bU>::const_iterator
	rbtree<K, V, C, A, E, bM, bU>::upper_bound(const key_type& key) const
	{
		typedef rbtree<K, V, C, A, E, bM, bU> rbtree_type;
		return const_iterator(const_cast<rbtree_type*>(this)->upper_bound(key), get_end_node());
	}


	// To do: Move this validate function entirely to a template-less implementation.
	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	bool rbtree<K, V, C, A, E, bM, bU>::validate() const
	{
		// Red-black trees have the following canonical properties which we validate here:
		//   1 Every node is either red or black.
		//   2 Every leaf (NULL) is black by defintion. Any number of black nodes may appear in a sequence. 
		//   3 If a node is red, then both its children are black. Thus, on any path from 
		//     the root to a leaf, red nodes must not be adjacent.
		//   4 Every simple path from a node to a descendant leaf contains the same number of black nodes.
		//   5 The mnSize member of the tree must equal the number of nodes in the tree.
		//   6 The tree is sorted as per a conventional binary tree.
		//   7 The comparison function is sane; it obeys strict weak ordering. If compare(a,b) is true, then compare(b,a) must be false. Both cannot be true.

		extract_key extractKey;

		if(mnSize)
		{
			// Verify basic integrity.
			//if(!mAnchor.mpNodeParent || (mAnchor.mpNodeLeft == mAnchor.mpNodeRight))
			//    return false;             // Fix this for case of empty tree.

			if(mMinMaxNodes.mpMinChild != RBTreeGetMinChild<rbtree_soft0_ptr>(get_root_node()))
				return false;

			if(mMinMaxNodes.mpMaxChild != RBTreeGetMaxChild<rbtree_soft0_ptr>(get_root_node()))
				return false;

			const size_t nBlackCount   = RBTreeGetBlackCount<rbtree_soft0_ptr>(get_root_node(), mMinMaxNodes.mpMinChild);
			size_type    nIteratedSize = 0;

			for(const_iterator it = begin(); it != end(); ++it, ++nIteratedSize)
			{
				rbtree_soft_ptr pNodeTmp      = it.get_node();
				rbtree_soft0_ptr pNode;
				//mb: this is slow convertion from soft_ptr to soft0_ptr
				// but we don't care because this is a debug only function
				if(pNodeTmp == pNodeTmp->mpNodeParent->mpNodeLeft)
					pNode = pNodeTmp->mpNodeParent->get_node_left();
				else
					pNode = pNodeTmp->mpNodeParent->get_node_right();



				rbtree_soft0_ptr pNodeRight = pNode->get_node_right();
				rbtree_soft0_ptr pNodeLeft  = pNode->get_node_left();

				// Verify #7 above.
				if(pNodeRight && compare(extractKey(pNodeRight->mValue), extractKey(pNode->mValue)) && compare(extractKey(pNode->mValue), extractKey(pNodeRight->mValue))) // Validate that the compare function is sane.
					return false;

				// Verify #7 above.
				if(pNodeLeft && compare(extractKey(pNodeLeft->mValue), extractKey(pNode->mValue)) && compare(extractKey(pNode->mValue), extractKey(pNodeLeft->mValue))) // Validate that the compare function is sane.
					return false;

				// Verify item #1 above.
				if((pNode->mColor != kRBTreeColorRed) && (pNode->mColor != kRBTreeColorBlack))
					return false;

				// Verify item #3 above.
				if(pNode->mColor == kRBTreeColorRed)
				{
					if((pNodeRight && (pNodeRight->mColor == kRBTreeColorRed)) ||
					   (pNodeLeft  && (pNodeLeft->mColor  == kRBTreeColorRed)))
						return false;
				}

				// Verify item #6 above.
				if(pNodeRight && compare(extractKey(pNodeRight->mValue), extractKey(pNode->mValue)))
					return false;

				if(pNodeLeft && compare(extractKey(pNode->mValue), extractKey(pNodeLeft->mValue)))
					return false;

				if(!pNodeRight && !pNodeLeft) // If we are at a bottom node of the tree...
				{
					// Verify item #4 above.
					if(RBTreeGetBlackCount<rbtree_soft0_ptr>(get_root_node(), pNode) != nBlackCount)
						return false;
				}
			}

			// Verify item #5 above.
			if(nIteratedSize != mnSize)
				return false;

			return true;
		}
		else
		{
			if((mMinMaxNodes.mpMinChild != get_end_node()) || (mMinMaxNodes.mpMaxChild != get_end_node()))
				return false;
		}

		return true;
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline int rbtree<K, V, C, A, E, bM, bU>::validate_iterator(const_iterator i) const
	{
		// To do: Come up with a more efficient mechanism of doing this.

		for(const_iterator temp = begin(), tempEnd = end(); temp != tempEnd; ++temp)
		{
			if(temp == i)
				return (isf_valid | isf_current | isf_can_dereference);
		}

		if(i == end())
			return (isf_valid | isf_current); 

		return isf_none;
	}

	

// 	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
// 	inline typename rbtree<K, V, C, A, E, bM, bU>::node_owning_ptr
// 	rbtree<K, V, C, A, E, bM, bU>::DoAllocateNode()
// 	{
// //		auto* pNode = (node_type*)allocate_memory(mAllocator, sizeof(node_type), EASTL_ALIGN_OF(value_type), 0);
// 		node_owning_ptr pNode = safememory::allocate_with_control_block<node_type>();
// 		EASTL_ASSERT_MSG(pNode != nullptr, "the behaviour of eastl::allocators that return nullptr is not defined.");

// 		return pNode;
// 	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline void rbtree<K, V, C, A, E, bM, bU>::DoFreeNode(rbtree_owning_ptr pNode)
	{
		//mb: we put it in special state in case some iterator is still pointing to it
		pNode->set_as_deleted();

		safememory::lib_helpers::rbtree_delete_owning<node_type>(std::move(pNode));
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	typename rbtree<K, V, C, A, E, bM, bU>::rbtree_owning_ptr
	rbtree<K, V, C, A, E, bM, bU>::DoCreateNodeFromKey(const key_type& key)
	{
		// Note that this function intentionally leaves the node pointers uninitialized.
		// The caller would otherwise just turn right around and modify them, so there's
		// no point in us initializing them to anything (except in a debug build).
// 		node_owning_ptr pNode = DoAllocateNode();

// 		#if EASTL_EXCEPTIONS_ENABLED
// 			try
// 			{
// 		#endif
// //				::new (std::addressof(pNode->mValue)) value_type(pair_first_construct, key);
// 				safememory::construct_with_control_block<value_type>(pNode, std::addressof(pNode->mValue),
// 					std::piecewise_construct, std::forward_as_tuple(key), std::forward_as_tuple());
// 		#if EASTL_EXCEPTIONS_ENABLED
// 			}
// 			catch(...)
// 			{
// 				DoFreeNode(std::move(pNode));
// 				throw;
// 			}
// 		#endif

// 		#if EASTL_DEBUG
// 			pNode->mpNodeRight  = NULL;
// 			pNode->mpNodeLeft   = NULL;
// 			pNode->mpNodeParent = NULL;
// 			pNode->mColor       = kRBTreeColorBlack;
// 		#endif

// 		return pNode;

		return safememory::lib_helpers::rbtree_make_owning<node_type>(std::piecewise_construct, std::forward_as_tuple(key), std::forward_as_tuple());
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	typename rbtree<K, V, C, A, E, bM, bU>::rbtree_owning_ptr
	rbtree<K, V, C, A, E, bM, bU>::DoCreateNode(const value_type& value)
	{
		// Note that this function intentionally leaves the node pointers uninitialized.
		// The caller would otherwise just turn right around and modify them, so there's
		// no point in us initializing them to anything (except in a debug build).
		// node_owning_ptr pNode = DoAllocateNode();

		// #if EASTL_EXCEPTIONS_ENABLED
		// 	try
		// 	{
		// #endif
		// 		// ::new(std::addressof(pNode->mValue)) value_type(value);
		// 		safememory::construct_with_control_block<value_type>(pNode, std::addressof(pNode->mValue), value);
		// #if EASTL_EXCEPTIONS_ENABLED
		// 	}
		// 	catch(...)
		// 	{
		// 		DoFreeNode(std::move(pNode));
		// 		throw;
		// 	}
		// #endif

		// #if EASTL_DEBUG
		// 	pNode->mpNodeRight  = NULL;
		// 	pNode->mpNodeLeft   = NULL;
		// 	pNode->mpNodeParent = NULL;
		// 	pNode->mColor       = kRBTreeColorBlack;
		// #endif

		// return pNode;

		return safememory::lib_helpers::rbtree_make_owning<node_type>(value);
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	typename rbtree<K, V, C, A, E, bM, bU>::rbtree_owning_ptr
	rbtree<K, V, C, A, E, bM, bU>::DoCreateNode(value_type&& value)
	{
		// Note that this function intentionally leaves the node pointers uninitialized.
		// The caller would otherwise just turn right around and modify them, so there's
		// no point in us initializing them to anything (except in a debug build).
		// node_owning_ptr pNode = DoAllocateNode();

		// #if EASTL_EXCEPTIONS_ENABLED
		// 	try
		// 	{
		// #endif
		// 		// ::new(std::addressof(pNode->mValue)) value_type(std::move(value));
		// 		safememory::construct_with_control_block<value_type>(pNode, std::addressof(pNode->mValue), std::move(value));
		// #if EASTL_EXCEPTIONS_ENABLED
		// 	}
		// 	catch(...)
		// 	{
		// 		DoFreeNode(std::move(pNode));
		// 		throw;
		// 	}
		// #endif

		// #if EASTL_DEBUG
		// 	pNode->mpNodeRight  = NULL;
		// 	pNode->mpNodeLeft   = NULL;
		// 	pNode->mpNodeParent = NULL;
		// 	pNode->mColor       = kRBTreeColorBlack;
		// #endif

		// return pNode;

		return safememory::lib_helpers::rbtree_make_owning<node_type>(std::move(value));
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	template<class... Args>
	typename rbtree<K, V, C, A, E, bM, bU>::rbtree_owning_ptr
	rbtree<K, V, C, A, E, bM, bU>::DoCreateNode(Args&&... args)
	{
		// Note that this function intentionally leaves the node pointers uninitialized.
		// The caller would otherwise just turn right around and modify them, so there's
		// no point in us initializing them to anything (except in a debug build).
// 		node_owning_ptr pNode = DoAllocateNode();

// 		#if EASTL_EXCEPTIONS_ENABLED
// 			try
// 			{
// 		#endif
// //				::new(std::addressof(pNode->mValue)) value_type(std::forward<Args>(args)...);
// 				safememory::construct_with_control_block<value_type>(pNode, std::addressof(pNode->mValue), std::forward<Args>(args)...);
// 		#if EASTL_EXCEPTIONS_ENABLED
// 			}
// 			catch(...)
// 			{
// 				DoFreeNode(std::move(pNode));
// 				throw;
// 			}
// 		#endif

// 		#if EASTL_DEBUG
// 			pNode->mpNodeRight  = NULL;
// 			pNode->mpNodeLeft   = NULL;
// 			pNode->mpNodeParent = NULL;
// 			pNode->mColor       = kRBTreeColorBlack;
// 		#endif

// 		return pNode;

		return safememory::lib_helpers::rbtree_make_owning<node_type>(std::forward<Args>(args)...);
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	typename rbtree<K, V, C, A, E, bM, bU>::rbtree_owning_ptr
	rbtree<K, V, C, A, E, bM, bU>::DoCreateNode(const rbtree_soft0_ptr pNodeSource, rbtree_soft0_ptr pNodeParent)
	{
		// node_owning_ptr pNode = DoCreateNode(pNodeSource->mValue);

		// pNode->mpNodeRight  = NULL;
		// pNode->mpNodeLeft   = NULL;
		// pNode->mpNodeParent = pNodeParent;
		// pNode->mColor       = pNodeSource->mColor;

		// return pNode;

		return safememory::lib_helpers::rbtree_make_owning<node_type>(pNodeParent, pNodeSource->mColor, pNodeSource->mValue);

	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	typename rbtree<K, V, C, A, E, bM, bU>::rbtree_owning_ptr
	rbtree<K, V, C, A, E, bM, bU>::DoCopySubtree(const rbtree_soft0_ptr pNodeSource, rbtree_soft0_ptr pNodeDest)
	{
		rbtree_owning_ptr pNewNodeRoot = DoCreateNode(pNodeSource, pNodeDest);

		#if EASTL_EXCEPTIONS_ENABLED
			try
			{
		#endif
				// Copy the right side of the tree recursively.
				if(pNodeSource->mpNodeRight)
					pNewNodeRoot->mpNodeRight = DoCopySubtree(pNodeSource->get_node_right(), pNewNodeRoot);

//				node_type* pNewNodeLeft;

				for(pNodeSource = pNodeSource->get_node_left(), pNodeDest = pNewNodeRoot; 
					pNodeSource;
					pNodeSource = pNodeSource->get_node_left())
				{
					rbtree_owning_ptr pNewNodeLeft = DoCreateNode(pNodeSource, pNodeDest);

					pNodeDest->mpNodeLeft = std::move(pNewNodeLeft);
					pNodeDest = pNodeDest->mpNodeLeft

					// Copy the right side of the tree recursively.
					if(pNodeSource->mpNodeRight)
						pNodeDest->mpNodeRight = DoCopySubtree(pNodeSource->get_node_right(), pNodeDest);


					
				}
		#if EASTL_EXCEPTIONS_ENABLED
			}
			catch(...)
			{
				DoNukeSubtree(std::move(pNewNodeRoot));
				throw;
			}
		#endif

		return pNewNodeRoot;
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	void rbtree<K, V, C, A, E, bM, bU>::DoNukeSubtree(rbtree_owning_ptr pNode)
	{
		while(pNode) // Recursively traverse the tree and destroy items as we go.
		{
			rbtree_owning_ptr pNodeRight = pNode->take_node_right();
			DoNukeSubtree(std::move(pNodeRight));

			rbtree_owning_ptr pNodeLeft = pNode->take_node_left();
			DoFreeNode(std::move(pNode));
			pNode = std::move(pNodeLeft);
		}
	}

	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	typename rbtree<K, V, C, A, E, bM, bU>::rbtree_owning_ptr
	rbtree<K, V, C, A, E, bM, bU>::create_end_node()
	{
		//mb: here we are allocating on the heap the anchor, or 'end' node
		//	this is different from usual implementation where end node is embedded
		// as a member an not heap allocated.
		// we do this because we need to always be able to create a reliable soft_ptr
		// to the 'end' node, regardless of the map being created on the heap
		// or on the stack
		// This is also helpful as the 'end' node is used to match iterators to its source trees
		rbtree_owning_ptr pNode = safememory::lib_helpers::rbtree_make_owning<node_type>();
		pNode->mColor = kRBTreeColorZombie;

		mMinMaxNodes.mpMinChild = pNode;
		mMinMaxNodes.mpMaxChild = pNode;	

		return pNode;
	}

	// template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	// void rbtree<K, V, C, A, E, bM, bU>::reset_end_node(node_soft_ptr anchor, rbtree_min_max_nodes& all_nodes)
	// {
	// 	EASTL_ASSERT(!anchor->mpNodeRight);
	// 	EASTL_ASSERT(!anchor->mpNodeLeft);
	// 	EASTL_ASSERT(!anchor->mpNodeParent);
	// 	EASTL_ASSERT(anchor->mColor == kRBTreeColorRed);


	// 	all_nodes.mpMinChild = anchor;
	// 	all_nodes.mpMaxChild = anchor;	
	// }

	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	void rbtree<K, V, C, A, E, bM, bU>::check_iterator(const const_iterator& it) const
	{
		//first check if iterator belong to our tree
		if(it.get_end_node() != get_end_node())
			throw "TODO2"; //TODO


		// check if is not a deleted node
		it.assert_alive_or_end();
	}

	///////////////////////////////////////////////////////////////////////
	// global operators
	///////////////////////////////////////////////////////////////////////

	template <typename K, typename V, typename A, typename C, typename E, bool bM, bool bU>
	inline bool operator==(const rbtree<K, V, C, A, E, bM, bU>& a, const rbtree<K, V, C, A, E, bM, bU>& b)
	{
		return (a.size() == b.size()) && std::equal(a.begin(), a.end(), b.begin());
	}


	// Note that in operator< we do comparisons based on the tree value_type with operator<() of the
	// value_type instead of the tree's Compare function. For set/multiset, the value_type is T, while
	// for map/multimap the value_type is a pair<Key, T>. operator< for pair can be seen by looking
	// utility.h, but it basically is uses the operator< for pair.first and pair.second. The C++ standard
	// appears to require this behaviour, whether intentionally or not. If anything, a good reason to do
	// this is for consistency. A map and a vector that contain the same items should compare the same.
	template <typename K, typename V, typename A, typename C, typename E, bool bM, bool bU>
	inline bool operator<(const rbtree<K, V, C, A, E, bM, bU>& a, const rbtree<K, V, C, A, E, bM, bU>& b)
	{
		return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end());
	}


	template <typename K, typename V, typename A, typename C, typename E, bool bM, bool bU>
	inline bool operator!=(const rbtree<K, V, C, A, E, bM, bU>& a, const rbtree<K, V, C, A, E, bM, bU>& b)
	{
		return !(a == b);
	}


	template <typename K, typename V, typename A, typename C, typename E, bool bM, bool bU>
	inline bool operator>(const rbtree<K, V, C, A, E, bM, bU>& a, const rbtree<K, V, C, A, E, bM, bU>& b)
	{
		return b < a;
	}


	template <typename K, typename V, typename A, typename C, typename E, bool bM, bool bU>
	inline bool operator<=(const rbtree<K, V, C, A, E, bM, bU>& a, const rbtree<K, V, C, A, E, bM, bU>& b)
	{
		return !(b < a);
	}


	template <typename K, typename V, typename A, typename C, typename E, bool bM, bool bU>
	inline bool operator>=(const rbtree<K, V, C, A, E, bM, bU>& a, const rbtree<K, V, C, A, E, bM, bU>& b)
	{
		return !(a < b);
	}


	template <typename K, typename V, typename A, typename C, typename E, bool bM, bool bU>
	inline void swap(rbtree<K, V, C, A, E, bM, bU>& a, rbtree<K, V, C, A, E, bM, bU>& b)
	{
		a.swap(b);
	}


} // namespace nodecpp


#ifdef _MSC_VER
	#pragma warning(pop)
#endif


#endif // Header include guard














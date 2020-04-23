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

///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// This file implements a vector (array-like container), much like the C++ 
// std::vector class.
// The primary distinctions between this vector and std::vector are:
//    - vector has a couple extension functions that increase performance.
//    - vector can contain objects with alignment requirements. std::vector 
//      cannot do so without a bit of tedious non-portable effort.
//    - vector supports debug memory naming natively.
//    - vector is easier to read, debug, and visualize.
//    - vector is savvy to an environment that doesn't have exception handling,
//      as is sometimes the case with console or embedded environments.
//    - vector has less deeply nested function calls and allows the user to 
//      enable forced inlining in debug builds in order to reduce bloat.
//    - vector<bool> is a vector of boolean values and not a bit vector.
//    - vector guarantees that memory is contiguous and that vector::iterator
//      is nothing more than a pointer to T.
//    - vector has an explicit data() method for obtaining a pointer to storage 
//      which is safe to call even if the block is empty. This avoids the 
//      common &v[0], &v.front(), and &*v.begin() constructs that trigger false 
//      asserts in STL debugging modes.
//    - vector data is guaranteed to be contiguous.
//    - vector has a set_capacity() function which frees excess capacity. 
//      The only way to do this with std::vector is via the cryptic non-obvious 
//      trick of using: vector<SomeClass>(x).swap(x);
///////////////////////////////////////////////////////////////////////////////


#ifndef EASTL_VECTOR_H
#define EASTL_VECTOR_H

#include <safememory/EASTL/internal/__undef_macros.h>
#include <safememory/EASTL/internal/config.h>
#include <safememory/detail/safe_alloc.h>
//#include <EASTL/allocator.h>
#include <safememory/EASTL/type_traits.h>
#include <safememory/EASTL/iterator.h>
#include <iterator>
#include <algorithm>
#include <initializer_list>
#include <safememory/EASTL/memory.h>
#include <memory>
//#include <EASTL/bonus/compressed_pair.h>

//EA_DISABLE_ALL_VC_WARNINGS()
#include <new>
#include <stddef.h>
// #if EASTL_EXCEPTIONS_ENABLED
	#include <stdexcept> // std::out_of_range, std::length_error.
// #endif
//EA_RESTORE_ALL_VC_WARNINGS()


#ifdef _MSC_VER
	#pragma warning(push)
	#pragma warning(disable: 4530)  // C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc
	#pragma warning(disable: 4345)  // Behavior change: an object of POD type constructed with an initializer of the form () will be default-initialized
	#pragma warning(disable: 4244)  // Argument: conversion from 'int' to 'const eastl::vector<T>::value_type', possible loss of data
	#pragma warning(disable: 4127)  // Conditional expression is constant
	#pragma warning(disable: 4480)  // nonstandard extension used: specifying underlying type for enum
	#pragma warning(disable: 4571)  // catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught.
#endif

#if defined(EA_PRAGMA_ONCE_SUPPORTED)
	#pragma once // Some compilers (e.g. VC++) benefit significantly from using this. We've measured 3-4% build speed improvements in apps as a result.
#endif

// #if EASTL_NOMINMAX
// 	#ifdef min
// 		#undef min
// 	#endif
// 	#ifdef max
// 		#undef max
// 	#endif
// #endif

namespace safememory
{

	/// EASTL_VECTOR_DEFAULT_NAME
	///
	/// Defines a default container name in the absence of a user-provided name.
	///
	#ifndef EASTL_VECTOR_DEFAULT_NAME
		#define EASTL_VECTOR_DEFAULT_NAME EASTL_DEFAULT_NAME_PREFIX " vector" // Unless the user overrides something, this is "EASTL vector".
	#endif


	/// EASTL_VECTOR_DEFAULT_ALLOCATOR
	///
	// #ifndef EASTL_VECTOR_DEFAULT_ALLOCATOR
	// 	#define EASTL_VECTOR_DEFAULT_ALLOCATOR allocator_type(EASTL_VECTOR_DEFAULT_NAME)
	// #endif



	/// VectorBase
	///
	/// The reason we have a VectorBase class is that it makes exception handling
	/// simpler to implement because memory allocation is implemented entirely 
	/// in this class. If a user creates a vector which needs to allocate
	/// memory in the constructor, VectorBase handles it. If an exception is thrown
	/// by the allocator then the exception throw jumps back to the user code and 
	/// no try/catch code need be written in the vector or VectorBase constructor. 
	/// If an exception is thrown in the vector (not VectorBase) constructor, the 
	/// destructor for VectorBase will be called automatically (and free the allocated
	/// memory) before the execution jumps back to the user code.
	/// However, if the vector class were to handle both allocation and initialization
	/// then it would have no choice but to implement an explicit try/catch statement
	/// for all pathways that allocate memory. This increases code size and decreases
	/// performance and makes the code a little harder read and maintain.
	///
	/// The C++ standard (15.2 paragraph 2) states: 
	///    "An object that is partially constructed or partially destroyed will
	///     have destructors executed for all its fully constructed subobjects,
	///     that is, for subobjects for which the constructor has been completed
	///     execution and the destructor has not yet begun execution."
	///
	/// The C++ standard (15.3 paragraph 11) states: 
	///    "The fully constructed base classes and members of an object shall 
	///     be destroyed before entering the handler of a function-try-block
	///     of a constructor or destructor for that block."
	///
	// template <typename T, typename Allocator>
	// struct VectorBase
	// {
	// 	// typedef Allocator    allocator_type;
	// 	// typedef std::size_t       size_type;
	// 	// typedef std::ptrdiff_t    difference_type;


	// 	#if defined(_MSC_VER) && (_MSC_VER >= 1400) && (_MSC_VER <= 1600) && !EASTL_STD_CPP_ONLY  // _MSC_VER of 1400 means VS2005, 1600 means VS2010. VS2012 generates errors with usage of enum:size_type.
	// 		enum : size_type {                      // Use Microsoft enum language extension, allowing for smaller debug symbols than using a static const. Users have been affected by this.
	// 			npos     = (size_type)-1,
	// 			kMaxSize = (size_type)-2
	// 		};
	// 	#else
	// 		static const size_type npos     = (size_type)-1;      /// 'npos' means non-valid position or simply non-position.
	// 		static const size_type kMaxSize = (size_type)-2;      /// -1 is reserved for 'npos'. It also happens to be slightly beneficial that kMaxSize is a value less than -1, as it helps us deal with potential integer wraparound issues.
	// 	#endif

	// protected:
	// 	owning_heap_type							mHeap;
	// 	T*                                          mpBegin;
	// 	T*                                          mpEnd;
		// eastl::compressed_pair<T*, allocator_type>  mCapacityAllocator;
		// T*	                                        mCapacity;

		// T*& internalCapacityPtr() EA_NOEXCEPT { return mCapacity; }
		// T* const& internalCapacityPtr() const EA_NOEXCEPT { return mCapacity; }
		// allocator_type&  internalAllocator() EA_NOEXCEPT { return mCapacityAllocator.second(); }
		// const allocator_type&  internalAllocator() const EA_NOEXCEPT { return mCapacityAllocator.second(); }

	// public:
	// 	VectorBase();
	// 	// VectorBase(const allocator_type& allocator);
	// 	VectorBase(size_type n/*, const allocator_type& allocator*/);

	//    ~VectorBase();

		// const allocator_type& get_allocator() const EA_NOEXCEPT;
		// allocator_type&       get_allocator() EA_NOEXCEPT;
		// void                  set_allocator(const allocator_type& allocator);

	// protected:
		// owning_heap_type  DoAllocate(size_type n);
		// void      DoFree(T* p, size_type n);
		// size_type GetNewCapacity(size_type currentCapacity);

	// }; // VectorBase




	/// vector
	///
	/// Implements a dynamic array.
	///
	template <typename T, typename Allocator = std::allocator<T> >
	class vector
	{
		static_assert(std::is_nothrow_move_constructible<T>::value ||
			std::is_copy_constructible<T>::value, "T must be copiable or nothrow movable");


		// typedef VectorBase<T, Allocator>                      base_type;
		typedef vector<T, Allocator>                          this_type;

	public:
		typedef T                                             value_type;
		typedef T*                                            pointer;
		typedef const T*                                      const_pointer;
		typedef T&                                            reference;
		typedef const T&                                      const_reference;
		typedef std::reverse_iterator<pointer>        		  reverse_iterator_unsafe;
		typedef std::reverse_iterator<const_pointer>          const_reverse_iterator_unsafe;    
		typedef std::size_t								      size_type;
		typedef std::ptrdiff_t    							  difference_type;
		// typedef typename base_type::allocator_type            allocator_type;

		typedef owning_ptr<detail::array_of2<T>> 					owning_heap_type;
		typedef soft_ptr<detail::array_of2<T>> 						soft_heap_type;

		typedef detail::safe_iterator<T>							iterator_safe;
		typedef detail::safe_iterator<const T>						const_iterator_safe;
		typedef std::reverse_iterator<iterator_safe>                reverse_iterator_safe;
		typedef std::reverse_iterator<const_iterator_safe>          const_reverse_iterator_safe;
		typedef const const_iterator_safe&							csafe_it_arg;
		typedef const const_reverse_iterator_safe&					crsafe_it_arg;
		
		typedef std::pair<const_pointer, const_pointer>				const_pointer_pair;

		typedef iterator_safe                                       iterator;
		typedef const_iterator_safe                                 const_iterator;
		typedef reverse_iterator_safe             					reverse_iterator;
		typedef const_reverse_iterator_safe							const_reverse_iterator;    

		// using base_type::mpBegin;
		// using base_type::mpEnd;
		// using base_type::mCapacity;
		// using base_type::npos;
		// using base_type::GetNewCapacity;
		// using base_type::DoAllocate;
		// using base_type::DoFree;
		// using base_type::internalCapacityPtr;
		// using base_type::internalAllocator;

		static const size_type npos     = (size_type)-1;      /// 'npos' means non-valid position or simply non-position.
		static const size_type kMaxSize = (size_type)-2;      /// -1 is reserved for 'npos'. It also happens to be slightly beneficial that kMaxSize is a value less than -1, as it helps us deal with potential integer wraparound issues.

	private:
		owning_heap_type							mHeap;
		T*                                          mpBegin;
		T*                                          mpEnd;
		// eastl::compressed_pair<T*, allocator_type>  mCapacityAllocator;
		T*	                                        mCapacity;

	public:
		vector() /*EA_NOEXCEPT_IF(EA_NOEXCEPT_EXPR(EASTL_VECTOR_DEFAULT_ALLOCATOR))*/;
		// explicit vector(const allocator_type& allocator) EA_NOEXCEPT;
		explicit vector(size_type n/*, const allocator_type& allocator = EASTL_VECTOR_DEFAULT_ALLOCATOR*/);
		vector(size_type n, const value_type& value/*, const allocator_type& allocator = EASTL_VECTOR_DEFAULT_ALLOCATOR*/);
		vector(const this_type& x);
		// vector(const this_type& x, const allocator_type& allocator);
		vector(this_type&& x) EA_NOEXCEPT;
		// vector(this_type&& x, const allocator_type& allocator);
		vector(std::initializer_list<value_type> ilist/*, const allocator_type& allocator = EASTL_VECTOR_DEFAULT_ALLOCATOR*/);

		// template <typename InputIterator>
		// vector(InputIterator first, InputIterator last/*, const allocator_type& allocator = EASTL_VECTOR_DEFAULT_ALLOCATOR*/);
		vector(csafe_it_arg first, csafe_it_arg last);

	   ~vector();

		this_type& operator=(const this_type& x);
		this_type& operator=(std::initializer_list<value_type> ilist);
		this_type& operator=(this_type&& x); // TODO(c++17): noexcept(allocator_traits<Allocator>::propagate_on_container_move_assignment::value || allocator_traits<Allocator>::is_always_equal::value)

		void swap(this_type& x); // TODO(c++17): noexcept(allocator_traits<Allocator>::propagate_on_container_move_assignment::value || allocator_traits<Allocator>::is_always_equal::value)

		void assign(size_type n, const value_type& value);

		template <typename InputIterator>
		void assign_unsafe(InputIterator first, InputIterator last);
		
		void assign(csafe_it_arg first, csafe_it_arg last);

		void assign(std::initializer_list<value_type> ilist);

		pointer       begin_unsafe() EA_NOEXCEPT;
		const_pointer begin_unsafe() const EA_NOEXCEPT;
		const_pointer cbegin_unsafe() const EA_NOEXCEPT;

		pointer       end_unsafe() EA_NOEXCEPT;
		const_pointer end_unsafe() const EA_NOEXCEPT;
		const_pointer cend_unsafe() const EA_NOEXCEPT;

		reverse_iterator_unsafe       rbegin_unsafe() EA_NOEXCEPT;
		const_reverse_iterator_unsafe rbegin_unsafe() const EA_NOEXCEPT;
		const_reverse_iterator_unsafe crbegin_unsafe() const EA_NOEXCEPT;

		reverse_iterator_unsafe       rend_unsafe() EA_NOEXCEPT;
		const_reverse_iterator_unsafe rend_unsafe() const EA_NOEXCEPT;
		const_reverse_iterator_unsafe crend_unsafe() const EA_NOEXCEPT;

		iterator_safe       begin() EA_NOEXCEPT;
		const_iterator_safe begin() const EA_NOEXCEPT;
		const_iterator_safe cbegin() const EA_NOEXCEPT;

		iterator_safe       end() EA_NOEXCEPT;
		const_iterator_safe end() const EA_NOEXCEPT;
		const_iterator_safe cend() const EA_NOEXCEPT;

		reverse_iterator_safe       rbegin() EA_NOEXCEPT;
		const_reverse_iterator_safe rbegin() const EA_NOEXCEPT;
		const_reverse_iterator_safe crbegin() const EA_NOEXCEPT;

		reverse_iterator_safe       rend() EA_NOEXCEPT;
		const_reverse_iterator_safe rend() const EA_NOEXCEPT;
		const_reverse_iterator_safe crend() const EA_NOEXCEPT;

		bool      empty() const EA_NOEXCEPT;
		size_type size() const EA_NOEXCEPT;
		size_type capacity() const EA_NOEXCEPT;
		size_type max_size() const EA_NOEXCEPT;

		void resize(size_type n, const value_type& value);
		void resize(size_type n);
		void reserve(size_type n);
		// void set_capacity(size_type n = base_type::npos);   // Revises the capacity to the user-specified value. Resizes the container to match the capacity if the requested capacity n is less than the current size. If n == npos then the capacity is reallocated (if necessary) such that capacity == size.
		void shrink_to_fit();                               // C++11 function which is the same as set_capacity().

		pointer       data() EA_NOEXCEPT;
		const_pointer data() const EA_NOEXCEPT;

		reference       operator[](size_type n);
		const_reference operator[](size_type n) const;

		reference       at(size_type n);
		const_reference at(size_type n) const;

		reference       front();
		const_reference front() const;

		reference       back();
		const_reference back() const;

		void      push_back(const value_type& value);
		reference push_back();
		void*     push_back_uninitialized();
		void      push_back(value_type&& value);
		void      pop_back();

		template<class... Args>
		pointer emplace_unsafe(const_pointer position, Args&&... args);
		template<class... Args>
		iterator_safe emplace(csafe_it_arg position, Args&&... args);

		template<class... Args>
		reference emplace_back(Args&&... args);

		pointer insert_unsafe(const_pointer position, const value_type& value);
		pointer insert_unsafe(const_pointer position, size_type n, const value_type& value);
		pointer insert_unsafe(const_pointer position, value_type&& value);
		pointer insert_unsafe(const_pointer position, std::initializer_list<value_type> ilist);

		template <typename InputIterator>
		pointer insert_unsafe(const_pointer position, InputIterator first, InputIterator last);

		iterator_safe insert(csafe_it_arg position, const value_type& value);
		iterator_safe insert(csafe_it_arg position, size_type n, const value_type& value);
		iterator_safe insert(csafe_it_arg position, value_type&& value);
		iterator_safe insert(csafe_it_arg position, std::initializer_list<value_type> ilist);

		// template <typename InputIterator>
		iterator_safe insert(csafe_it_arg position, csafe_it_arg first, csafe_it_arg last);

//		template <typename = std::enable_if<std::has_equality_v<T>>>
		// iterator erase_first(const T& value);
//		template <typename = std::enable_if<std::has_equality_v<T>>>
		// iterator erase_first_unsorted(const T& value); // Same as erase, except it doesn't preserve order, but is faster because it simply copies the last item in the vector over the erased position.
//		template <typename = std::enable_if<std::has_equality_v<T>>>
		// reverse_iterator erase_last(const T& value);
//		template <typename = std::enable_if<std::has_equality_v<T>>>
		// reverse_iterator erase_last_unsorted(const T& value); // Same as erase, except it doesn't preserve order, but is faster because it simply copies the last item in the vector over the erased position.

		pointer erase_unsafe(const_pointer position);
		pointer erase_unsafe(const_pointer first, const_pointer last);
		iterator_safe erase(csafe_it_arg position);
		iterator_safe erase(csafe_it_arg first, csafe_it_arg last);
		// iterator erase_unsorted(const_iterator position);         // Same as erase, except it doesn't preserve order, but is faster because it simply copies the last item in the vector over the erased position.

		// reverse_iterator erase(const_reverse_iterator position);
		// reverse_iterator erase(const_reverse_iterator first, const_reverse_iterator last);
		// reverse_iterator erase_unsorted(const_reverse_iterator position);

		void clear() EA_NOEXCEPT;
		// void reset_lose_memory() EA_NOEXCEPT;                       // This is a unilateral reset to an initially empty state. No destructors are called, no deallocation occurs.

		bool validate() const EA_NOEXCEPT;
		int  validate_iterator(const_pointer i) const EA_NOEXCEPT;
		int  validate_iterator(csafe_it_arg i) const EA_NOEXCEPT;

	protected:
		// These functions do the real work of maintaining the vector. You will notice
		// that many of them have the same name but are specialized on iterator_tag
		// (iterator categories). This is because in these cases there is an optimized
		// implementation that can be had for some cases relative to others. Functions
		// which aren't referenced are neither compiled nor linked into the application.

		/// Allocates a new heap of at least \c n elements in size
		/// The actual allocated size depends on grow policy, and also
		/// depend on iibmalloc discrete posible sizes of allocation.
		owning_heap_type  DoAllocate(size_type n);

		/// Set a newly allocated heap, we currently don't support
		/// not having an allocated heap
		inline void SetNewHeap(owning_heap_type&& new_heap) {
			mHeap = std::move(new_heap);
			mpBegin = mHeap->begin();
			mCapacity = mHeap->begin() + mHeap->capacity();
		}

		/// Set a soft_ptr to the heap, mostly used by safe iterators
		inline soft_heap_type GetSoftHeapPtr() const EA_NOEXCEPT {
			return soft_heap_type(mHeap);
		}

		// struct should_copy_tag{}; struct should_move_tag : public should_copy_tag{};

		// template <typename ForwardIterator> // Allocates a pointer of array count n and copy-constructs it with [first,last).
		// void DoRealloc(size_type n, ForwardIterator first, ForwardIterator last, should_copy_tag);

		// template <typename ForwardIterator> // Allocates a pointer of array count n and copy-constructs it with [first,last).
		// void DoRealloc(size_type n, ForwardIterator first, ForwardIterator last, should_move_tag);

		// template <typename Integer>
		// void DoInit(Integer n, Integer value, std::true_type);

		// template <typename InputIterator>
		// void DoInit(InputIterator first, InputIterator last, std::false_type);

		// template <typename InputIterator>
		// void DoInitFromIterator(InputIterator first, InputIterator last, std::input_iterator_tag);

		// template <typename ForwardIterator>
		// void DoInitFromIterator(ForwardIterator first, ForwardIterator last, std::forward_iterator_tag);

		template <typename Integer, bool bMove>
		void DoAssign(Integer n, Integer value, std::true_type);

		template <typename InputIterator, bool bMove>
		void DoAssign(InputIterator first, InputIterator last, std::false_type);

		void DoAssignValues(size_type n, const value_type& value);

		template <typename InputIterator, bool bMove>
		void DoAssignFromIterator(InputIterator first, InputIterator last, std::input_iterator_tag);

		template <typename RandomAccessIterator, bool bMove>
		void DoAssignFromIterator(RandomAccessIterator first, RandomAccessIterator last, std::random_access_iterator_tag);

		template <typename Integer>
		void DoInsert(const_pointer position, Integer n, Integer value, std::true_type);

		template <typename InputIterator>
		void DoInsert(const_pointer position, InputIterator first, InputIterator last, std::false_type);

		template <typename InputIterator>
		void DoInsertFromIterator(const_pointer position, InputIterator first, InputIterator last, std::input_iterator_tag);

		template <typename BidirectionalIterator>
		void DoInsertFromIterator(const_pointer position, BidirectionalIterator first, BidirectionalIterator last, std::bidirectional_iterator_tag);

		void DoInsertValues(const_pointer position, size_type n, const value_type& value);

		void DoInsertValuesEnd(size_type n); // Default constructs n values
		void DoInsertValuesEnd(size_type n, const value_type& value);

		template<typename... Args>
		void DoInsertValue(const_pointer position, Args&&... args);

		template<typename... Args>
		void DoInsertValueEnd(Args&&... args);

		// void DoClearCapacity();

		void DoGrow(size_type n);

		void DoSwap(this_type& x);

		[[noreturn]] static void ThrowLengthException();
		[[noreturn]] static void ThrowRangeException();
		[[noreturn]] static void ThrowInvalidArgumentException();
		[[noreturn]] static void ThrowMaxSizeException();

		static
		const_pointer_pair CheckAndGet(csafe_it_arg itBegin, csafe_it_arg itEnd);
		
		const_pointer CheckMineAndGet(csafe_it_arg it) const;
		const_pointer_pair CheckMineAndGet(csafe_it_arg itBegin, csafe_it_arg itEnd) const;


	}; // class vector






	///////////////////////////////////////////////////////////////////////
	// VectorBase
	///////////////////////////////////////////////////////////////////////

// 	template <typename T, typename Allocator>
// 	inline VectorBase<T, Allocator>::VectorBase()
// // 		: mpBegin(NULL), 
// // 		  mpEnd(NULL),
// // //		  mCapacityAllocator(NULL, allocator_type(EASTL_VECTOR_DEFAULT_NAME))
// // 		  mCapacity(NULL)
// 	{
// 		SetNewHeap(DoAllocate(0));
// 		mpEnd      = mpBegin;
// 	}


// 	// template <typename T, typename Allocator>
// 	// inline VectorBase<T, Allocator>::VectorBase(const allocator_type& allocator)
// 	// 	: mpBegin(NULL), 
// 	// 	  mpEnd(NULL),
// 	// 	  mCapacityAllocator(NULL, allocator)
// 	// {
// 	// }


// 	template <typename T, typename Allocator>
// 	inline VectorBase<T, Allocator>::VectorBase(size_type n/*, const allocator_type& allocator*/)
// 		// : mCapacityAllocator(allocator)
// 	{
// 		SetNewHeap(DoAllocate(n));
// 		// mpBegin    = DoAllocate(n);
// 		// mCapacity = mpBegin + n;
// 		mpEnd      = mpBegin;
// 	}


// 	template <typename T, typename Allocator>
// 	inline VectorBase<T, Allocator>::~VectorBase()
// 	{


// 	}


	// template <typename T, typename Allocator>
	// inline const typename VectorBase<T, Allocator>::allocator_type&
	// VectorBase<T, Allocator>::get_allocator() const EA_NOEXCEPT
	// {
	// 	return internalAllocator();
	// }


	// template <typename T, typename Allocator>
	// inline typename VectorBase<T, Allocator>::allocator_type&
	// VectorBase<T, Allocator>::get_allocator() EA_NOEXCEPT
	// {
	// 	return internalAllocator();
	// }


	// template <typename T, typename Allocator>
	// inline void VectorBase<T, Allocator>::set_allocator(const allocator_type& allocator)
	// {
	// 	internalAllocator() = allocator;
	// }


	// template <typename T, typename Allocator>
	// inline typename VectorBase<T, Allocator>::owning_heap_type VectorBase<T, Allocator>::DoAllocate(size_type n)
	// {
	// 	#if EASTL_ASSERT_ENABLED
	// 		if(EASTL_UNLIKELY(n >= 0x80000000))
	// 			EASTL_FAIL_MSG("vector::DoAllocate -- improbably large request.");
	// 	#endif

	// 	// If n is zero, then we allocate no memory and just return nullptr. 
	// 	// This is fine, as our default ctor initializes with NULL pointers. 
	// 	if(EASTL_LIKELY(n))
	// 	{
	// 		// auto* p = (T*)safememory::lib_helpers::allocate_memory(n * sizeof(T), alignof(T), 0);
	// 		// EASTL_ASSERT_MSG(p != nullptr, "the behaviour of eastl::allocators that return nullptr is not defined.");
	// 		// return p;

	// 		//TODO
	// 		// if(EASTL_UNLIKELY(n > max_size()))
	// 		// 	ThrowMaxSizeException();

	// 		return detail::make_owning_array_of<T>(n);
	// 	}
	// 	else
	// 	{
	// 		return owning_heap_type();
	// 	}
	// }


	// template <typename T, typename Allocator>
	// inline void VectorBase<T, Allocator>::DoFree(T* p, size_type n)
	// {
	// 	if(p)
	// 		safememory::lib_helpers::EASTLFree(p, n * sizeof(T)); 
	// }


	// template <typename T, typename Allocator>
	// inline typename VectorBase<T, Allocator>::size_type
	// VectorBase<T, Allocator>::GetNewCapacity(size_type currentCapacity)
	// {
	// 	// TODO: mb update this to better take use of iibmalloc discrete allocation
	// 	// sizes

	// 	// This needs to return a value of at least currentCapacity and at least 1.
	// 	return (currentCapacity > 0) ? (2 * currentCapacity) : 1;
	// }




	///////////////////////////////////////////////////////////////////////
	// vector
	///////////////////////////////////////////////////////////////////////

	template <typename T, typename Allocator>
	inline vector<T, Allocator>::vector() /*EA_NOEXCEPT_IF(EA_NOEXCEPT_EXPR(EASTL_VECTOR_DEFAULT_ALLOCATOR))*/
		/*: base_type()*/
	{
		SetNewHeap(DoAllocate(0));
		mpEnd      = mpBegin;
	}


	// template <typename T, typename Allocator>
	// inline vector<T, Allocator>::vector(const allocator_type& allocator) EA_NOEXCEPT
	// 	: base_type(allocator)
	// {
	// 	// Empty
	// }


	template <typename T, typename Allocator>
	inline vector<T, Allocator>::vector(size_type n/*, const allocator_type& allocator*/)
		// : base_type(n/*, allocator*/)
	{
		SetNewHeap(DoAllocate(n));
		// eastl::uninitialized_default_fill_n(mpBegin, n);
		std::uninitialized_value_construct_n(mpBegin, n);
		mpEnd = mpBegin + n;
	}


	template <typename T, typename Allocator>
	inline vector<T, Allocator>::vector(size_type n, const value_type& value/*, const allocator_type& allocator*/)
		// : base_type(n/*, allocator*/)
	{
		SetNewHeap(DoAllocate(n));
//		eastl::uninitialized_fill_n_ptr(mpBegin, n, value);
		std::uninitialized_fill_n(mpBegin, n, value);
		mpEnd = mpBegin + n;
	}


	template <typename T, typename Allocator>
	inline vector<T, Allocator>::vector(const this_type& x)
//		: base_type(x.size()/*, x.internalAllocator()*/)
	{
		SetNewHeap(DoAllocate(x.size()));
//		mpEnd = eastl::uninitialized_copy_ptr(x.mpBegin, x.mpEnd, mpBegin);
		mpEnd = std::uninitialized_copy(x.mpBegin, x.mpEnd, mpBegin);
	}


	// template <typename T, typename Allocator>
	// inline vector<T, Allocator>::vector(const this_type& x, const allocator_type& allocator)
	// 	: base_type(x.size(), allocator)
	// {
	// 	mpEnd = eastl::uninitialized_copy_ptr(x.mpBegin, x.mpEnd, mpBegin);
	// }


	template <typename T, typename Allocator>
	inline vector<T, Allocator>::vector(this_type&& x) EA_NOEXCEPT
		// : base_type(/*std::move(x.internalAllocator())*/)  // vector requires move-construction of allocator in this case.
	{
		SetNewHeap(DoAllocate(0));
		mpEnd      = mpBegin;
		DoSwap(x);
	}


	// template <typename T, typename Allocator>
	// inline vector<T, Allocator>::vector(this_type&& x, const allocator_type& allocator)
	// 	: base_type(allocator)
	// {
	// 	if (internalAllocator() == x.internalAllocator()) // If allocators are equivalent...
	// 		DoSwap(x);
	// 	else 
	// 	{
	// 		this_type temp(std::move(*this)); // move construct so we don't require the use of copy-ctors that prevent the use of move-only types.
	// 		temp.swap(x);
	// 	}
	// }


	template <typename T, typename Allocator>
	inline vector<T, Allocator>::vector(std::initializer_list<value_type> ilist/*, const allocator_type& allocator*/)
//		: base_type(/*allocator*/)
	{
		SetNewHeap(DoAllocate(ilist.size()));
		mpEnd = std::uninitialized_copy(ilist.begin(), ilist.end(), mpBegin);
	}


	// template <typename T, typename Allocator>
	// template <typename InputIterator>
	// inline vector<T, Allocator>::vector(InputIterator first, InputIterator last/*, const allocator_type& allocator*/)
	// 	: base_type(/*allocator*/)
	// {
	// 	DoInit(first, last, std::is_integral<InputIterator>());
	// }

	template <typename T, typename Allocator>
	inline vector<T, Allocator>::vector(csafe_it_arg first, csafe_it_arg last)
//		: base_type()
	{
		const_pointer_pair p = CheckAndGet(first, last);
		size_type sz = static_cast<size_type>(p.second - p.first);

		SetNewHeap(DoAllocate(sz));
		mpEnd = std::uninitialized_copy(p.first, p.second, mpBegin);
	}



	template <typename T, typename Allocator>
	inline vector<T, Allocator>::~vector()
	{
		// Call destructor for the values. Parent class will free the memory.
		std::destroy(mpBegin, mpEnd);
	}


	template <typename T, typename Allocator>
	typename vector<T, Allocator>::this_type&
	vector<T, Allocator>::operator=(const this_type& x)
	{
		if(this != &x) // If not assigning to self...
		{
			// If (EASTL_ALLOCATOR_COPY_ENABLED == 1) and the current contents are allocated by an 
			// allocator that's unequal to x's allocator, we need to reallocate our elements with 
			// our current allocator and reallocate it with x's allocator. If the allocators are 
			// equal then we can use a more optimal algorithm that doesn't reallocate our elements
			// but instead can copy them in place.

			// #if EASTL_ALLOCATOR_COPY_ENABLED
			// 	bool bSlowerPathwayRequired = (internalAllocator() != x.internalAllocator());
			// #else
			// 	bool bSlowerPathwayRequired = false;
			// #endif

			// if(bSlowerPathwayRequired)
			// {
			// 	DoClearCapacity(); // Must clear the capacity instead of clear because set_capacity frees our memory, unlike clear.

			// 	#if EASTL_ALLOCATOR_COPY_ENABLED
			// 		internalAllocator() = x.internalAllocator();
			// 	#endif
			// }

			DoAssign<const_pointer, false>(x.begin_unsafe(), x.end_unsafe(), std::false_type());
		}

		return *this;
	}


	template <typename T, typename Allocator>
	typename vector<T, Allocator>::this_type&
	vector<T, Allocator>::operator=(std::initializer_list<value_type> ilist)
	{
		typedef typename std::initializer_list<value_type>::iterator InputIterator;
		typedef typename std::iterator_traits<InputIterator>::iterator_category IC;
		DoAssignFromIterator<InputIterator, false>(ilist.begin(), ilist.end(), IC()); // initializer_list has const elements and so we can't move from them.
		return *this;
	}


	template <typename T, typename Allocator>
	typename vector<T, Allocator>::this_type&
	vector<T, Allocator>::operator=(this_type&& x)
	{
		if(this != &x)
		{
//			DoClearCapacity(); // To consider: Are we really required to clear here? x is going away soon and will clear itself in its dtor.
			DoSwap(x);          // member swap handles the case that x has a different allocator than our allocator by doing a copy.
		}
		return *this; 
	}


	template <typename T, typename Allocator>
	inline void vector<T, Allocator>::assign(size_type n, const value_type& value)
	{
		DoAssignValues(n, value);
	}


	template <typename T, typename Allocator>
	template <typename InputIterator>                              
	inline void vector<T, Allocator>::assign_unsafe(InputIterator first, InputIterator last)
	{
		// It turns out that the C++ std::vector<int, int> specifies a two argument
		// version of assign that takes (int size, int value). These are not iterators, 
		// so we need to do a template compiler trick to do the right thing.
		DoAssign<InputIterator, false>(first, last, std::is_integral<InputIterator>());
	}

	template <typename T, typename Allocator>
	inline void vector<T, Allocator>::assign(csafe_it_arg first, csafe_it_arg last)
	{
		const_pointer_pair p = CheckAndGet(first, last);
		assign_unsafe(p.first, p.second);
	}

	template <typename T, typename Allocator>
	inline void vector<T, Allocator>::assign(std::initializer_list<value_type> ilist)
	{
		typedef typename std::initializer_list<value_type>::iterator InputIterator;
		typedef typename std::iterator_traits<InputIterator>::iterator_category IC;
		DoAssignFromIterator<InputIterator, false>(ilist.begin(), ilist.end(), IC()); // initializer_list has const elements and so we can't move from them.
	}


	template <typename T, typename Allocator>
	inline typename vector<T, Allocator>::pointer
	vector<T, Allocator>::begin_unsafe() EA_NOEXCEPT
	{
		return mpBegin;
	}


	template <typename T, typename Allocator>
	inline typename vector<T, Allocator>::const_pointer
	vector<T, Allocator>::begin_unsafe() const EA_NOEXCEPT
	{
		return mpBegin;
	}


	template <typename T, typename Allocator>
	inline typename vector<T, Allocator>::const_pointer
	vector<T, Allocator>::cbegin_unsafe() const EA_NOEXCEPT
	{
		return mpBegin;
	}


	template <typename T, typename Allocator>
	inline typename vector<T, Allocator>::pointer
	vector<T, Allocator>::end_unsafe() EA_NOEXCEPT
	{
		return mpEnd;
	}


	template <typename T, typename Allocator>
	inline typename vector<T, Allocator>::const_pointer
	vector<T, Allocator>::end_unsafe() const EA_NOEXCEPT
	{
		return mpEnd;
	}


	template <typename T, typename Allocator>
	inline typename vector<T, Allocator>::const_pointer
	vector<T, Allocator>::cend_unsafe() const EA_NOEXCEPT
	{
		return mpEnd;
	}


	template <typename T, typename Allocator>
	inline typename vector<T, Allocator>::reverse_iterator_unsafe
	vector<T, Allocator>::rbegin_unsafe() EA_NOEXCEPT
	{
		return reverse_iterator_unsafe(mpEnd);
	}


	template <typename T, typename Allocator>
	inline typename vector<T, Allocator>::const_reverse_iterator_unsafe
	vector<T, Allocator>::rbegin_unsafe() const EA_NOEXCEPT
	{
		return const_reverse_iterator_unsafe(mpEnd);
	}


	template <typename T, typename Allocator>
	inline typename vector<T, Allocator>::const_reverse_iterator_unsafe
	vector<T, Allocator>::crbegin_unsafe() const EA_NOEXCEPT
	{
		return const_reverse_iterator_unsafe(mpEnd);
	}


	template <typename T, typename Allocator>
	inline typename vector<T, Allocator>::reverse_iterator_unsafe
	vector<T, Allocator>::rend_unsafe() EA_NOEXCEPT
	{
		return reverse_iterator_unsafe(mpBegin);
	}


	template <typename T, typename Allocator>
	inline typename vector<T, Allocator>::const_reverse_iterator_unsafe
	vector<T, Allocator>::rend_unsafe() const EA_NOEXCEPT
	{
		return const_reverse_iterator_unsafe(mpBegin);
	}


	template <typename T, typename Allocator>
	inline typename vector<T, Allocator>::const_reverse_iterator_unsafe
	vector<T, Allocator>::crend_unsafe() const EA_NOEXCEPT
	{
		return const_reverse_iterator_unsafe(mpBegin);
	}



	template <typename T, typename Allocator>
	inline typename vector<T, Allocator>::iterator_safe
	vector<T, Allocator>::begin() EA_NOEXCEPT
	{
		return iterator_safe(GetSoftHeapPtr());
	}


	template <typename T, typename Allocator>
	inline typename vector<T, Allocator>::const_iterator_safe
	vector<T, Allocator>::begin() const EA_NOEXCEPT
	{
		return const_iterator_safe(GetSoftHeapPtr());
	}


	template <typename T, typename Allocator>
	inline typename vector<T, Allocator>::const_iterator_safe
	vector<T, Allocator>::cbegin() const EA_NOEXCEPT
	{
		return const_iterator_safe(GetSoftHeapPtr());
	}


	template <typename T, typename Allocator>
	inline typename vector<T, Allocator>::iterator_safe
	vector<T, Allocator>::end() EA_NOEXCEPT
	{
		return iterator_safe(GetSoftHeapPtr(), size());
	}


	template <typename T, typename Allocator>
	inline typename vector<T, Allocator>::const_iterator_safe
	vector<T, Allocator>::end() const EA_NOEXCEPT
	{
		return const_iterator_safe(GetSoftHeapPtr(), size());
	}


	template <typename T, typename Allocator>
	inline typename vector<T, Allocator>::const_iterator_safe
	vector<T, Allocator>::cend() const EA_NOEXCEPT
	{
		return const_iterator_safe(GetSoftHeapPtr(), size());
	}


	template <typename T, typename Allocator>
	inline typename vector<T, Allocator>::reverse_iterator_safe
	vector<T, Allocator>::rbegin() EA_NOEXCEPT
	{
		return reverse_iterator_safe(end());
	}


	template <typename T, typename Allocator>
	inline typename vector<T, Allocator>::const_reverse_iterator_safe
	vector<T, Allocator>::rbegin() const EA_NOEXCEPT
	{
		return const_reverse_iterator_safe(end());
	}


	template <typename T, typename Allocator>
	inline typename vector<T, Allocator>::const_reverse_iterator_safe
	vector<T, Allocator>::crbegin() const EA_NOEXCEPT
	{
		return const_reverse_iterator_safe(end());
	}


	template <typename T, typename Allocator>
	inline typename vector<T, Allocator>::reverse_iterator_safe
	vector<T, Allocator>::rend() EA_NOEXCEPT
	{
		return reverse_iterator_safe(begin());
	}


	template <typename T, typename Allocator>
	inline typename vector<T, Allocator>::const_reverse_iterator_safe
	vector<T, Allocator>::rend() const EA_NOEXCEPT
	{
		return const_reverse_iterator_safe(begin());
	}


	template <typename T, typename Allocator>
	inline typename vector<T, Allocator>::const_reverse_iterator_safe
	vector<T, Allocator>::crend() const EA_NOEXCEPT
	{
		return const_reverse_iterator_safe(begin());
	}

	template <typename T, typename Allocator>
	bool vector<T, Allocator>::empty() const EA_NOEXCEPT
	{
		return (mpBegin == mpEnd);
	}


	template <typename T, typename Allocator>
	inline typename vector<T, Allocator>::size_type
	vector<T, Allocator>::size() const EA_NOEXCEPT
	{
		return (size_type)(mpEnd - mpBegin);
	}


	template <typename T, typename Allocator>
	inline typename vector<T, Allocator>::size_type
	vector<T, Allocator>::capacity() const EA_NOEXCEPT
	{
		return (size_type)(mCapacity - mpBegin);
	}

	template <typename T, typename Allocator>
	inline typename vector<T, Allocator>::size_type
	vector<T, Allocator>::max_size() const EA_NOEXCEPT
	{
		return kMaxSize;
	}

	template <typename T, typename Allocator>
	inline void vector<T, Allocator>::resize(size_type n, const value_type& value)
	{
		if(n > (size_type)(mpEnd - mpBegin))  // We expect that more often than not, resizes will be upsizes.
			DoInsertValuesEnd(n - ((size_type)(mpEnd - mpBegin)), value);
		else
		{
			std::destroy(mpBegin + n, mpEnd);
			mpEnd = mpBegin + n;
		}
	}


	template <typename T, typename Allocator>
	inline void vector<T, Allocator>::resize(size_type n)
	{
		// Alternative implementation:
		// resize(n, value_type());

		if(n > (size_type)(mpEnd - mpBegin))  // We expect that more often than not, resizes will be upsizes.
			DoInsertValuesEnd(n - ((size_type)(mpEnd - mpBegin)));
		else
		{
			std::destroy(mpBegin + n, mpEnd);
			mpEnd = mpBegin + n;
		}
	}


	template <typename T, typename Allocator>
	void vector<T, Allocator>::reserve(size_type n)
	{
		// If the user wants to reduce the reserved memory, there is the set_capacity function.
		if(n > capacity()) // If n > capacity ...
			DoGrow(n);
	}


	// template <typename T, typename Allocator>
	// void vector<T, Allocator>::set_capacity(size_type n)
	// {
	// 	if((n == npos) || (n <= (size_type)(mpEnd - mpBegin))) // If new capacity <= size...
	// 	{
	// 		if(n == 0)  // Very often n will be 0, and clear will be faster than resize and use less stack space.
	// 			clear();
	// 		else if(n < (size_type)(mpEnd - mpBegin))
	// 			resize(n);

	// 		shrink_to_fit();
	// 	}
	// 	else // Else new capacity > size.
	// 	{
	// 		DoRealloc(n, mpBegin, mpEnd, should_move_tag());
	// 		// pointer const pNewData = DoRealloc(n, mpBegin, mpEnd, should_move_tag());
	// 		// std::destroy(mpBegin, mpEnd);
	// 		// DoFree(mpBegin, (size_type)(mCapacity - mpBegin));

	// 		// const std::ptrdiff_t nPrevSize = mpEnd - mpBegin;
	// 		// mpBegin    = pNewData;
	// 		// mpEnd      = pNewData + nPrevSize;
	// 		// mCapacity = mpBegin + n;
	// 	}
	// }

	template <typename T, typename Allocator>
	inline void vector<T, Allocator>::shrink_to_fit()
	{
		// This is the simplest way to accomplish this, and it is as efficient as any other.
		this_type temp;
		temp.assign_unsafe(std::move_iterator<pointer>(begin_unsafe()), std::move_iterator<pointer>(end_unsafe())/*, internalAllocator()*/);

		// Call DoSwap() rather than swap() as we know our allocators match and we don't want to invoke the code path
		// handling non matching allocators as it imposes additional restrictions on the type of T to be copyable
		DoSwap(temp);
	}

	template <typename T, typename Allocator>
	inline typename vector<T, Allocator>::pointer
	vector<T, Allocator>::data() EA_NOEXCEPT
	{
		return mpBegin;
	}


	template <typename T, typename Allocator>
	inline typename vector<T, Allocator>::const_pointer
	vector<T, Allocator>::data() const EA_NOEXCEPT
	{
		return mpBegin;
	}


	template <typename T, typename Allocator>
	inline typename vector<T, Allocator>::reference
	vector<T, Allocator>::operator[](size_type n)
	{
		#if EASTL_EMPTY_REFERENCE_ASSERT_ENABLED    // We allow the user to use a reference to v[0] of an empty container. But this was merely grandfathered in and ideally we shouldn't allow such access to [0].
			if(EASTL_UNLIKELY((n != 0) && (n >= (static_cast<size_type>(mpEnd - mpBegin)))))
				EASTL_FAIL_MSG("vector::operator[] -- out of range");
		#elif EASTL_ASSERT_ENABLED
			if(EASTL_UNLIKELY(n >= (static_cast<size_type>(mpEnd - mpBegin))))
				EASTL_FAIL_MSG("vector::operator[] -- out of range");
		#endif

		return *(mpBegin + n);
	}


	template <typename T, typename Allocator>
	inline typename vector<T, Allocator>::const_reference
	vector<T, Allocator>::operator[](size_type n) const
	{
		#if EASTL_EMPTY_REFERENCE_ASSERT_ENABLED    // We allow the user to use a reference to v[0] of an empty container. But this was merely grandfathered in and ideally we shouldn't allow such access to [0].
			if(EASTL_UNLIKELY((n != 0) && (n >= (static_cast<size_type>(mpEnd - mpBegin)))))
				EASTL_FAIL_MSG("vector::operator[] -- out of range");
		#elif EASTL_ASSERT_ENABLED
			if(EASTL_UNLIKELY(n >= (static_cast<size_type>(mpEnd - mpBegin))))
				EASTL_FAIL_MSG("vector::operator[] -- out of range");
		#endif

		return *(mpBegin + n);
	}


	template <typename T, typename Allocator>
	inline typename vector<T, Allocator>::reference
	vector<T, Allocator>::at(size_type n)
	{
		// The difference between at() and operator[] is it signals 
		// the requested position is out of range by throwing an 
		// out_of_range exception.

		// #if EASTL_EXCEPTIONS_ENABLED
			if(EASTL_UNLIKELY(n >= (static_cast<size_type>(mpEnd - mpBegin))))
				throw std::out_of_range("vector::at -- out of range");
		// #elif EASTL_ASSERT_ENABLED
		// 	if(EASTL_UNLIKELY(n >= (static_cast<size_type>(mpEnd - mpBegin))))
		// 		EASTL_FAIL_MSG("vector::at -- out of range");
		// #endif

		return *(mpBegin + n);
	}


	template <typename T, typename Allocator>
	inline typename vector<T, Allocator>::const_reference
	vector<T, Allocator>::at(size_type n) const
	{
		// #if EASTL_EXCEPTIONS_ENABLED
			if(EASTL_UNLIKELY(n >= (static_cast<size_type>(mpEnd - mpBegin))))
				throw std::out_of_range("vector::at -- out of range");
		// #elif EASTL_ASSERT_ENABLED
		// 	if(EASTL_UNLIKELY(n >= (static_cast<size_type>(mpEnd - mpBegin))))
		// 		EASTL_FAIL_MSG("vector::at -- out of range");
		// #endif

		return *(mpBegin + n);
	}


	template <typename T, typename Allocator>
	inline typename vector<T, Allocator>::reference
	vector<T, Allocator>::front()
	{
		#if EASTL_EMPTY_REFERENCE_ASSERT_ENABLED
			// We allow the user to reference an empty container.
		#elif EASTL_ASSERT_ENABLED
			if(EASTL_UNLIKELY(mpEnd <= mpBegin)) // We don't allow the user to reference an empty container.
				EASTL_FAIL_MSG("vector::front -- empty vector");
		#endif

		return *mpBegin;
	}


	template <typename T, typename Allocator>
	inline typename vector<T, Allocator>::const_reference
	vector<T, Allocator>::front() const
	{
		#if EASTL_EMPTY_REFERENCE_ASSERT_ENABLED
			// We allow the user to reference an empty container.
		#elif EASTL_ASSERT_ENABLED
			if(EASTL_UNLIKELY(mpEnd <= mpBegin)) // We don't allow the user to reference an empty container.
				EASTL_FAIL_MSG("vector::front -- empty vector");
		#endif

		return *mpBegin;
	}


	template <typename T, typename Allocator>
	inline typename vector<T, Allocator>::reference
	vector<T, Allocator>::back()
	{
		#if EASTL_EMPTY_REFERENCE_ASSERT_ENABLED
			// We allow the user to reference an empty container.
		#elif EASTL_ASSERT_ENABLED
			if(EASTL_UNLIKELY(mpEnd <= mpBegin)) // We don't allow the user to reference an empty container.
				EASTL_FAIL_MSG("vector::back -- empty vector");
		#endif

		return *(mpEnd - 1);
	}


	template <typename T, typename Allocator>
	inline typename vector<T, Allocator>::const_reference
	vector<T, Allocator>::back() const
	{
		#if EASTL_EMPTY_REFERENCE_ASSERT_ENABLED
			// We allow the user to reference an empty container.
		#elif EASTL_ASSERT_ENABLED
			if(EASTL_UNLIKELY(mpEnd <= mpBegin)) // We don't allow the user to reference an empty container.
				EASTL_FAIL_MSG("vector::back -- empty vector");
		#endif

		return *(mpEnd - 1);
	}


	template <typename T, typename Allocator>
	inline void vector<T, Allocator>::push_back(const value_type& value)
	{
		if(mpEnd < mCapacity)
			::new((void*)mpEnd++) value_type(value);
		else
			DoInsertValueEnd(value);
	}


	template <typename T, typename Allocator>
	inline void vector<T, Allocator>::push_back(value_type&& value)
	{
		if (mpEnd < mCapacity)
			::new((void*)mpEnd++) value_type(std::move(value));
		else
			DoInsertValueEnd(std::move(value));
	}


	template <typename T, typename Allocator>
	inline typename vector<T, Allocator>::reference
	vector<T, Allocator>::push_back()
	{
		if(mpEnd < mCapacity)
			::new((void*)mpEnd++) value_type();
		else // Note that in this case we create a temporary, which is less desirable.
			DoInsertValueEnd(value_type());

		return *(mpEnd - 1); // Same as return back();
	}


	template <typename T, typename Allocator>
	inline void* vector<T, Allocator>::push_back_uninitialized()
	{
		if(mpEnd == mCapacity)
		{
			const size_type newSize = (size_type)(mpEnd - mpBegin) + 1;
			reserve(newSize);
		}
 
		return mpEnd++;
	}


	template <typename T, typename Allocator>
	inline void vector<T, Allocator>::pop_back()
	{
		#if EASTL_ASSERT_ENABLED
			if(EASTL_UNLIKELY(mpEnd <= mpBegin))
				EASTL_FAIL_MSG("vector::pop_back -- empty vector");
		#endif

		--mpEnd;
		mpEnd->~value_type();
	}


	template <typename T, typename Allocator>
	template<class... Args>
	inline typename vector<T, Allocator>::pointer 
	vector<T, Allocator>::emplace_unsafe(const_pointer position, Args&&... args)
	{
		const std::ptrdiff_t n = position - mpBegin; // Save this because we might reallocate.

		if((mpEnd == mCapacity) || (position != mpEnd))
			DoInsertValue(position, std::forward<Args>(args)...);
		else
		{
			::new((void*)mpEnd) value_type(std::forward<Args>(args)...);
			++mpEnd; // Increment this after the construction above in case the construction throws an exception.
		}

		return mpBegin + n;
	}

	template <typename T, typename Allocator>
	template<class... Args>
	inline typename vector<T, Allocator>::iterator_safe 
	vector<T, Allocator>::emplace(csafe_it_arg position, Args&&... args)
	{
		const_pointer p = CheckMineAndGet(position);
		pointer r = emplace_unsafe(p, std::forward<Args>(args)...);
		return iterator_safe(GetSoftHeapPtr(), r);
	}

	template <typename T, typename Allocator>
	template<class... Args>
	inline typename vector<T, Allocator>::reference
	vector<T, Allocator>::emplace_back(Args&&... args)
	{
		if(mpEnd < mCapacity)
		{
			::new((void*)mpEnd) value_type(std::forward<Args>(args)...);  // If value_type has a move constructor, it will use it and this operation may be faster than otherwise.
			++mpEnd; // Increment this after the construction above in case the construction throws an exception.
		}
		else
			DoInsertValueEnd(std::forward<Args>(args)...);

		return back();
	}

	template <typename T, typename Allocator>
	inline typename vector<T, Allocator>::pointer
	vector<T, Allocator>::insert_unsafe(const_pointer position, const value_type& value)
	{
		#if EASTL_ASSERT_ENABLED
			if(EASTL_UNLIKELY((position < mpBegin) || (position > mpEnd)))
				EASTL_FAIL_MSG("vector::insert -- invalid position");
		#endif

		// We implment a quick pathway for the case that the insertion position is at the end and we have free capacity for it.
		const std::ptrdiff_t n = position - mpBegin; // Save this because we might reallocate.

		if((mpEnd == mCapacity) || (position != mpEnd))
			DoInsertValue(position, value);
		else
		{
			::new((void*)mpEnd) value_type(value);
			++mpEnd; // Increment this after the construction above in case the construction throws an exception.
		}

		return mpBegin + n;
	}


	template <typename T, typename Allocator>       
	inline typename vector<T, Allocator>::pointer
	vector<T, Allocator>::insert_unsafe(const_pointer position, value_type&& value)
	{
		return emplace_unsafe(position, std::move(value));
	}


	template <typename T, typename Allocator>
	inline typename vector<T, Allocator>::pointer
	vector<T, Allocator>::insert_unsafe(const_pointer position, size_type n, const value_type& value)
	{
		const std::ptrdiff_t p = position - mpBegin; // Save this because we might reallocate.
		DoInsertValues(position, n, value);
		return mpBegin + p;
	}


	template <typename T, typename Allocator>
	template <typename InputIterator>
	inline typename vector<T, Allocator>::pointer
	vector<T, Allocator>::insert_unsafe(const_pointer position, InputIterator first, InputIterator last)
	{
		const std::ptrdiff_t n = position - mpBegin; // Save this because we might reallocate.
		DoInsert(position, first, last, std::is_integral<InputIterator>());
		return mpBegin + n;
	}


	template <typename T, typename Allocator>       
	inline typename vector<T, Allocator>::pointer
	vector<T, Allocator>::insert_unsafe(const_pointer position, std::initializer_list<value_type> ilist)
	{
		const std::ptrdiff_t n = position - mpBegin; // Save this because we might reallocate.
		DoInsert(position, ilist.begin(), ilist.end(), std::false_type());
		return mpBegin + n;
	}

	template <typename T, typename Allocator>
	inline typename vector<T, Allocator>::iterator_safe
	vector<T, Allocator>::insert(csafe_it_arg position, const value_type& value)
	{
		const_pointer p = CheckMineAndGet(position);
		pointer r = insert_unsafe(p, value);
		return iterator_safe(GetSoftHeapPtr(), r);
	}


	template <typename T, typename Allocator>       
	inline typename vector<T, Allocator>::iterator_safe
	vector<T, Allocator>::insert(csafe_it_arg position, value_type&& value)
	{
		const_pointer p = CheckMineAndGet(position);
		pointer r = insert_unsafe(p, std::move(value));
		return iterator_safe(GetSoftHeapPtr(), r);
	}


	template <typename T, typename Allocator>
	inline typename vector<T, Allocator>::iterator_safe
	vector<T, Allocator>::insert(csafe_it_arg position, size_type n, const value_type& value)
	{
		const_pointer p = CheckMineAndGet(position);
		pointer r = insert_unsafe(p, n, value);
		return iterator_safe(GetSoftHeapPtr(), r);
	}


	template <typename T, typename Allocator>
	inline typename vector<T, Allocator>::iterator_safe
	vector<T, Allocator>::insert(csafe_it_arg position, csafe_it_arg first, csafe_it_arg last)
	{
		const_pointer p = CheckMineAndGet(position);
		const_pointer_pair other = CheckAndGet(first, last);
		pointer r = insert_unsafe(p, other.first, other.second);
		return iterator_safe(GetSoftHeapPtr(), r);
	}


	template <typename T, typename Allocator>       
	inline typename vector<T, Allocator>::iterator_safe
	vector<T, Allocator>::insert(csafe_it_arg position, std::initializer_list<value_type> ilist)
	{
		const_pointer p = CheckMineAndGet(position);
		pointer r = insert_unsafe(p, ilist);
		return iterator_safe(GetSoftHeapPtr(), r);
	}


	template <typename T, typename Allocator>
	inline typename vector<T, Allocator>::pointer
	vector<T, Allocator>::erase_unsafe(const_pointer position)
	{
		#if EASTL_ASSERT_ENABLED
			if(EASTL_UNLIKELY((position < mpBegin) || (position >= mpEnd)))
				EASTL_FAIL_MSG("vector::erase -- invalid position");
		#endif

		// C++11 stipulates that position is const_iterator, but the return value is iterator.
		pointer destPosition = const_cast<value_type*>(position);        

		if((position + 1) < mpEnd)
			std::move(destPosition + 1, mpEnd, destPosition);
		--mpEnd;
		mpEnd->~value_type();
		return destPosition;
	}


	template <typename T, typename Allocator>
	inline typename vector<T, Allocator>::pointer
	vector<T, Allocator>::erase_unsafe(const_pointer first, const_pointer last)
	{
		#if EASTL_ASSERT_ENABLED
			if(EASTL_UNLIKELY((first < mpBegin) || (first > mpEnd) || (last < mpBegin) || (last > mpEnd) || (last < first)))
				EASTL_FAIL_MSG("vector::erase -- invalid position");
		#endif
 
		if (first != last)
		{
			pointer const position = const_cast<value_type*>(std::move(const_cast<value_type*>(last), const_cast<value_type*>(mpEnd), const_cast<value_type*>(first)));
			std::destroy(position, mpEnd);
			mpEnd -= (last - first);
		}
 
		return const_cast<value_type*>(first);
	}

	template <typename T, typename Allocator>
	inline typename vector<T, Allocator>::iterator_safe
	vector<T, Allocator>::erase(csafe_it_arg position)
	{
		const_pointer p = CheckMineAndGet(position);
		pointer r = erase_unsafe(p);
		return iterator_safe(GetSoftHeapPtr(), r);
	}


	template <typename T, typename Allocator>
	inline typename vector<T, Allocator>::iterator_safe
	vector<T, Allocator>::erase(csafe_it_arg first, csafe_it_arg last)
	{
		const_pointer_pair p = CheckMineAndGet(first, last);
		pointer r = erase_unsafe(p.first, p.second);
		return iterator_safe(GetSoftHeapPtr(), r);
	}

	// template <typename T, typename Allocator>
	// inline typename vector<T, Allocator>::iterator
	// vector<T, Allocator>::erase_unsorted(const_iterator position)
	// {
	// 	#if EASTL_ASSERT_ENABLED
	// 		if(EASTL_UNLIKELY((position < mpBegin) || (position >= mpEnd)))
	// 			EASTL_FAIL_MSG("vector::erase -- invalid position");
	// 	#endif

	// 	// C++11 stipulates that position is const_iterator, but the return value is iterator.
	// 	iterator destPosition = const_cast<value_type*>(position);
	// 	*destPosition = std::move(*(mpEnd - 1));

	// 	// pop_back();
	// 	--mpEnd;
	// 	mpEnd->~value_type();

	// 	return destPosition;
	// }

// 	template <typename T, typename Allocator>
// //	template <typename>
// 	inline typename vector<T, Allocator>::iterator vector<T, Allocator>::erase_first(const T& value)
// 	{
// 		iterator it = std::find(begin(), end(), value);

// 		if (it != end())
// 			return erase(it);
// 		else
// 			return it;
// 	}

// 	template <typename T, typename Allocator>
// //	template <typename>
// 	inline typename vector<T, Allocator>::iterator 
// 	vector<T, Allocator>::erase_first_unsorted(const T& value)
// 	{
// 		iterator it = std::find(begin(), end(), value);

// 		if (it != end())
// 			return erase_unsorted(it);
// 		else
// 			return it;
// 	}

// 	template <typename T, typename Allocator>
// //	template <typename>
// 	inline typename vector<T, Allocator>::reverse_iterator 
// 	vector<T, Allocator>::erase_last(const T& value)
// 	{
// 		reverse_iterator it = std::find(rbegin(), rend(), value);

// 		if (it != rend())
// 			return erase(it);
// 		else
// 			return it;
// 	}

// 	template <typename T, typename Allocator>
// //	template <typename>
// 	inline typename vector<T, Allocator>::reverse_iterator 
// 	vector<T, Allocator>::erase_last_unsorted(const T& value)
// 	{
// 		reverse_iterator it = std::find(rbegin(), rend(), value);

// 		if (it != rend())
// 			return erase_unsorted(it);
// 		else
// 			return it;
// 	}

	// template <typename T, typename Allocator>
	// inline typename vector<T, Allocator>::reverse_iterator
	// vector<T, Allocator>::erase(const_reverse_iterator position)
	// {
	// 	return reverse_iterator(erase((++position).base()));
	// }


	// template <typename T, typename Allocator>
	// inline typename vector<T, Allocator>::reverse_iterator
	// vector<T, Allocator>::erase(const_reverse_iterator first, const_reverse_iterator last)
	// {
	// 	// Version which erases in order from first to last.
	// 	// difference_type i(first.base() - last.base());
	// 	// while(i--)
	// 	//     first = erase(first);
	// 	// return first;

	// 	// Version which erases in order from last to first, but is slightly more efficient:
	// 	return reverse_iterator(erase(last.base(), first.base()));
	// }


	// template <typename T, typename Allocator>
	// inline typename vector<T, Allocator>::reverse_iterator
	// vector<T, Allocator>::erase_unsorted(const_reverse_iterator position)
	// {
	// 	return reverse_iterator(erase_unsorted((++position).base()));
	// }


	template <typename T, typename Allocator>
	inline void vector<T, Allocator>::clear() EA_NOEXCEPT
	{
		std::destroy(mpBegin, mpEnd);
		mpEnd = mpBegin;
	}


	// template <typename T, typename Allocator>
	// inline void vector<T, Allocator>::reset_lose_memory() EA_NOEXCEPT
	// {
	// 	// The reset function is a special extension function which unilaterally 
	// 	// resets the container to an empty state without freeing the memory of 
	// 	// the contained objects. This is useful for very quickly tearing down a 
	// 	// container built into scratch memory.
	// 	mpBegin = mpEnd = mCapacity = NULL;
	// }


	// swap exchanges the contents of two containers. With respect to the containers allocators,
	// the C11++ Standard (23.2.1/7) states that the behavior of a call to a container's swap function 
	// is undefined unless the objects being swapped have allocators that compare equal or 
	// allocator_traits<allocator_type>::propagate_on_container_swap::value is true (propagate_on_container_swap
	// is false by default). EASTL doesn't have allocator_traits and so this doesn't directly apply,
	// but EASTL has the effective behavior of propagate_on_container_swap = false for all allocators. 
	template <typename T, typename Allocator>
	inline void vector<T, Allocator>::swap(this_type& x)
	{
	// #if defined(EASTL_VECTOR_LEGACY_SWAP_BEHAVIOUR_REQUIRES_COPY_CTOR) && EASTL_VECTOR_LEGACY_SWAP_BEHAVIOUR_REQUIRES_COPY_CTOR
	// 	if(internalAllocator() == x.internalAllocator()) // If allocators are equivalent...
	// 		DoSwap(x);
	// 	else // else swap the contents.
	// 	{
	// 		const this_type temp(*this); // Can't call eastl::swap because that would
	// 		*this = x;                   // itself call this member swap function.
	// 		x     = temp;
	// 	}
	// #else
		// NOTE(rparolin): The previous implementation required T to be copy-constructible in the fall-back case where
		// allocators with unique instances copied elements.  This was an unnecessary restriction and prevented the common
		// usage of vector with non-copyable types (eg. eastl::vector<non_copyable> or eastl::vector<unique_ptr>). 
		// 
		// The previous implementation violated the following requirements of vector::swap so the fall-back code has
		// been removed.  EASTL implicitly defines 'propagate_on_container_swap = false' therefore the fall-back case is
		// undefined behaviour.  We simply swap the contents and the allocator as that is the common expectation of
		// users and does not put the container into an invalid state since it can not free its memory via its current
		// allocator instance.
		//
		// http://en.cppreference.com/w/cpp/container/vector/swap
		// "Exchanges the contents of the container with those of other. Does not invoke any move, copy, or swap
		// operations on individual elements."
		//
	    // http://en.cppreference.com/w/cpp/concept/AllocatorAwareContainer
	    // "Swapping two containers with unequal allocators if propagate_on_container_swap is false is undefined
	    // behavior."

		DoSwap(x);
	// #endif
	}

	template <typename T, typename Allocator>
	inline typename vector<T, Allocator>::owning_heap_type vector<T, Allocator>::DoAllocate(size_type n)
	{
		// TODO, allocated heap should be zeroed 

		if(EASTL_UNLIKELY(n > kMaxSize))
			ThrowMaxSizeException();

		// TODO remove this once we are correctly asking iibmalloc
		// about possible allocation sizes. Since iibmalloc usually
		// allocates on power of 2 sizes
		size_type currentCapacity = mHeap ? mHeap->capacity() : 0;
		n = std::max(n, (2 * currentCapacity));

		//mb: we currently don't support NULL alloc
		n = std::max(n, static_cast<size_type>(4));
		
		// mb: make_owning_array_of may return an array bigger
		// than requested because allocation has discrete possible
		// values under iibmalloc and we don't want to waste space
		return detail::make_owning_array_of<T>(n);
	}

	// template <typename T, typename Allocator>
	// template <typename ForwardIterator>
	// inline void
	// vector<T, Allocator>::DoRealloc(size_type n, ForwardIterator first, ForwardIterator last, should_copy_tag)
	// {
	// 	const std::ptrdiff_t nPrevSize = last - first;
	// 	auto p = DoAllocate(n); // p is of type T* but is not constructed. 
	// 	// eastl::uninitialized_copy_ptr(first, last, p); // copy-constructs p from [first,last).
	// 	std::uninitialized_copy(first, last, p->begin()); // copy-constructs p from [first,last).


	// 		// pointer const pNewData = DoRealloc(n, first, last, bMove ? should_move_tag() : should_copy_tag());

	// 	std::destroy(mpBegin, mpEnd);
	// 	SetNewHeap(std::move(p));
	// 		// DoFree(mpBegin, (size_type)(mCapacity - mpBegin));

	// 		// mpBegin    = pNewData;
	// 	mpEnd      = mpBegin + nPrevSize;
	// 		// mCapacity = mpEnd;
	// 	// return p;

	// }


	// template <typename T, typename Allocator>
	// template <typename ForwardIterator>
	// inline void
	// vector<T, Allocator>::DoRealloc(size_type n, ForwardIterator first, ForwardIterator last, should_move_tag)
	// {
	// 	// pointer const pNewData = DoRealloc(n, mpBegin, mpEnd, should_move_tag());

	// 	const std::ptrdiff_t nPrevSize = last - first;

	// 	auto p = DoAllocate(n); // p is of type T* but is not constructed. 
	// 	safememory::uninitialized_move_ptr_if_noexcept(first, last, p->begin()); // move-constructs p from [first,last).


		
	// 	std::destroy(mpBegin, mpEnd);
	// 	SetNewHeap(std::move(p));
	// 	// DoFree(mpBegin, (size_type)(mCapacity - mpBegin));

	// 	// mpBegin    = pNewData;
	// 	mpEnd      = mpBegin + nPrevSize;
	// 	// mCapacity = mpBegin + n;



	// 	// return p;
	// }


// 	template <typename T, typename Allocator>
// 	template <typename Integer>
// 	inline void vector<T, Allocator>::DoInit(Integer n, Integer value, std::true_type)
// 	{
// 		SetNewHeap(DoAllocate((size_type)n));
// 		// mpBegin    = DoAllocate((size_type)n);
// 		// mCapacity = mpBegin + n;
// //		mpEnd      = mCapacity;

// 		// typedef typename std::remove_const<T>::type non_const_value_type; // If T is a const type (e.g. const int) then we need to initialize it as if it were non-const.
// //		eastl::uninitialized_fill_n_ptr<value_type, Integer>((non_const_value_type*)mpBegin, n, value);
// 		mpEnd = std::uninitialized_fill_n(mpBegin, n, value);
// 	}


// 	template <typename T, typename Allocator>
// 	template <typename InputIterator>
// 	inline void vector<T, Allocator>::DoInit(InputIterator first, InputIterator last, std::false_type)
// 	{
// 		typedef typename std::iterator_traits<InputIterator>:: iterator_category IC;
// 		DoInitFromIterator(first, last, IC());
// 	}


// 	template <typename T, typename Allocator>
// 	template <typename InputIterator>
// 	inline void vector<T, Allocator>::DoInitFromIterator(InputIterator first, InputIterator last, std::input_iterator_tag)
// 	{
// 		// To do: Use emplace_back instead of push_back(). Our emplace_back will work below without any ifdefs.
// 		for(; first != last; ++first)  // InputIterators by definition actually only allow you to iterate through them once.
// 			push_back(*first);        // Thus the standard *requires* that we do this (inefficient) implementation.
// 	}                                 // Luckily, InputIterators are in practice almost never used, so this code will likely never get executed.


// 	template <typename T, typename Allocator>
// 	template <typename ForwardIterator>
// 	inline void vector<T, Allocator>::DoInitFromIterator(ForwardIterator first, ForwardIterator last, std::forward_iterator_tag)
// 	{
// 		const size_type n = (size_type)std::distance(first, last);
// 		SetNewHeap(DoAllocate(n));
// 		// mpBegin    = DoAllocate(n);
// 		// mCapacity = mpBegin + n;
// 		// mpEnd      = mCapacity;

// 		typedef typename std::remove_const<T>::type non_const_value_type; // If T is a const type (e.g. const int) then we need to initialize it as if it were non-const.
// //		eastl::uninitialized_copy_ptr(first, last, (non_const_value_type*)mpBegin);
// 		mpEnd = std::uninitialized_copy(first, last, (non_const_value_type*)mpBegin);
// 	}


	template <typename T, typename Allocator>
	template <typename Integer, bool bMove>
	inline void vector<T, Allocator>::DoAssign(Integer n, Integer value, std::true_type)
	{
		DoAssignValues(static_cast<size_type>(n), static_cast<value_type>(value));
	}


	template <typename T, typename Allocator>
	template <typename InputIterator, bool bMove>
	inline void vector<T, Allocator>::DoAssign(InputIterator first, InputIterator last, std::false_type)
	{
		typedef typename std::iterator_traits<InputIterator>::iterator_category IC;
		DoAssignFromIterator<InputIterator, bMove>(first, last, IC());
	}


	template <typename T, typename Allocator>
	void vector<T, Allocator>::DoAssignValues(size_type n, const value_type& value)
	{
		if(n > capacity()) // If n > capacity ...
		{
			this_type temp(n, value/*, internalAllocator()*/); // We have little choice but to reallocate with new memory.
			swap(temp);
		}
		else if(n > size_type(mpEnd - mpBegin)) // If n > size ...
		{
			std::fill(mpBegin, mpEnd, value);
//			eastl::uninitialized_fill_n_ptr(mpEnd, n - size_type(mpEnd - mpBegin), value);
			std::uninitialized_fill_n(mpEnd, n - size_type(mpEnd - mpBegin), value);
			mpEnd += n - size_type(mpEnd - mpBegin);
		}
		else // else 0 <= n <= size
		{
			std::fill_n(mpBegin, n, value);
			erase_unsafe(mpBegin + n, mpEnd);
		}
	}


	template <typename T, typename Allocator>
	template <typename InputIterator, bool bMove>
	void vector<T, Allocator>::DoAssignFromIterator(InputIterator first, InputIterator last, std::input_iterator_tag)
	{
		pointer position(mpBegin);

		while((position != mpEnd) && (first != last))
		{
			*position = *first;
			++first;
			++position;
		}
		if(first == last)
			erase_unsafe(position, mpEnd);
		else
			insert_unsafe(mpEnd, first, last);
	}


	template <typename T, typename Allocator>
	template <typename RandomAccessIterator, bool bMove>
	void vector<T, Allocator>::DoAssignFromIterator(RandomAccessIterator first, RandomAccessIterator last, std::random_access_iterator_tag)
	{
		const size_type n = (size_type)std::distance(first, last);

		if(n > capacity()) // If n > capacity ...
		{
			// DoRealloc(n, first, last, bMove ? should_move_tag() : should_copy_tag());
			// pointer const pNewData = DoRealloc(n, first, last, bMove ? should_move_tag() : should_copy_tag());
			// std::destroy(mpBegin, mpEnd);
			// DoFree(mpBegin, (size_type)(mCapacity - mpBegin));

			// mpBegin    = pNewData;
			// mpEnd      = mpBegin + n;
			// mCapacity = mpEnd;
			auto 			nNewHeap  = DoAllocate(n);
			pointer 		pNewData  = nNewHeap->begin();

			typedef typename std::remove_const<T>::type non_const_value_type; // If T is a const type (e.g. const int) then we need to initialize it as if it were non-const.
			pNewData =std::uninitialized_copy(first, last, (non_const_value_type*)pNewData);

			std::destroy(mpBegin, mpEnd);
			SetNewHeap(std::move(nNewHeap));
			// mpBegin    = DoAllocate(n);
			// mCapacity = mpBegin + n;
			mpEnd      = pNewData;

	//		eastl::uninitialized_copy_ptr(first, last, (non_const_value_type*)mpBegin);


		}
		else if(n <= size_type(mpEnd - mpBegin)) // If n <= size ...
		{
			pointer const pNewEnd = std::copy(first, last, mpBegin); // Since we are copying to mpBegin, we don't have to worry about needing copy_backward or a memmove-like copy (as opposed to memcpy-like copy).
			std::destroy(pNewEnd, mpEnd);
			mpEnd = pNewEnd;
		}
		else // else size < n <= capacity
		{
			RandomAccessIterator position = first + (mpEnd - mpBegin);
			std::copy(first, position, mpBegin); // Since we are copying to mpBegin, we don't have to worry about needing copy_backward or a memmove-like copy (as opposed to memcpy-like copy).
//			mpEnd = eastl::uninitialized_copy_ptr(position, last, mpEnd);
			mpEnd = std::uninitialized_copy(position, last, mpEnd);
		}
	}


	template <typename T, typename Allocator>
	template <typename Integer>
	inline void vector<T, Allocator>::DoInsert(const_pointer position, Integer n, Integer value, std::true_type)
	{
		DoInsertValues(position, static_cast<size_type>(n), static_cast<value_type>(value));
	}


	template <typename T, typename Allocator>
	template <typename InputIterator>
	inline void vector<T, Allocator>::DoInsert(const_pointer position, InputIterator first, InputIterator last, std::false_type)
	{
		typedef typename std::iterator_traits<InputIterator>::iterator_category IC;
		DoInsertFromIterator(position, first, last, IC());
	}


	template <typename T, typename Allocator>
	template <typename InputIterator>
	inline void vector<T, Allocator>::DoInsertFromIterator(const_pointer position, InputIterator first, InputIterator last, std::input_iterator_tag)
	{
		for(; first != last; ++first, ++position)
			position = insert_unsafe(position, *first);
	}


	template <typename T, typename Allocator>
	template <typename BidirectionalIterator>
	void vector<T, Allocator>::DoInsertFromIterator(const_pointer position, BidirectionalIterator first, BidirectionalIterator last, std::bidirectional_iterator_tag)
	{
		#if EASTL_ASSERT_ENABLED
			if(EASTL_UNLIKELY((position < mpBegin) || (position > mpEnd)))
				EASTL_FAIL_MSG("vector::insert -- invalid position");
		#endif

		// C++11 stipulates that position is const_iterator, but the return value is iterator.
		pointer destPosition = const_cast<value_type*>(position);

		if(first != last)
		{
			const size_type n = (size_type)std::distance(first, last);  // n is the number of elements we are inserting.

			if(n <= size_type(mCapacity - mpEnd)) // If n fits within the existing capacity...
			{
				const size_type nExtra = static_cast<size_type>(mpEnd - destPosition);

				if(n < nExtra) // If the inserted values are entirely within initialized memory (i.e. are before mpEnd)...
				{
//					eastl::uninitialized_move_ptr(mpEnd - n, mpEnd, mpEnd);
					std::uninitialized_move(mpEnd - n, mpEnd, mpEnd);
					std::move_backward(destPosition, mpEnd - n, mpEnd); // We need move_backward because of potential overlap issues.
					std::copy(first, last, destPosition);
				}
				else
				{
					BidirectionalIterator iTemp = first;
					std::advance(iTemp, nExtra);
//					eastl::uninitialized_copy_ptr(iTemp, last, mpEnd);
					std::uninitialized_copy(iTemp, last, mpEnd);
//					eastl::uninitialized_move_ptr(destPosition, mpEnd, mpEnd + n - nExtra);
					std::uninitialized_move(destPosition, mpEnd, mpEnd + n - nExtra);
					std::copy_backward(first, iTemp, destPosition + nExtra);
				}

				mpEnd += n;
			}
			else // else we need to expand our capacity.
			{
				const size_type nPrevSize = size_type(mpEnd - mpBegin);
				// const size_type nGrowSize = GetNewCapacity(nPrevSize);
				// const size_type nNewSize  = nGrowSize > (nPrevSize + n) ? nGrowSize : (nPrevSize + n);
				auto 			nNewHeap  = DoAllocate(nPrevSize + n);
				pointer const   pNewData  = nNewHeap->begin();

				// #if EASTL_EXCEPTIONS_ENABLED
					pointer pNewEnd = pNewData;
					try
					{
						pNewEnd = safememory::uninitialized_move_ptr_if_noexcept(mpBegin, destPosition, pNewData);
						// pNewEnd = eastl::uninitialized_copy_ptr(first, last, pNewEnd);
						pNewEnd = std::uninitialized_copy(first, last, pNewEnd);
						pNewEnd = safememory::uninitialized_move_ptr_if_noexcept(destPosition, mpEnd, pNewEnd);
					}
					catch(...)
					{
						std::destroy(pNewData, pNewEnd);
						// DoFree(pNewData, nNewSize);
						throw;
					}
				// #else
				// 	pointer pNewEnd = eastl::uninitialized_move_ptr_if_noexcept(mpBegin, destPosition, pNewData);
				// 	pNewEnd         = eastl::uninitialized_copy_ptr(first, last, pNewEnd);
				// 	pNewEnd         = eastl::uninitialized_move_ptr_if_noexcept(destPosition, mpEnd, pNewEnd);
				// #endif

				std::destroy(mpBegin, mpEnd);
				SetNewHeap(std::move(nNewHeap));
				// DoFree(mpBegin, (size_type)(mCapacity - mpBegin));

				// mpBegin    = pNewData;
				mpEnd      = pNewEnd;
				// mCapacity = pNewData + nNewSize;
			}
		}
	}


	template <typename T, typename Allocator>
	void vector<T, Allocator>::DoInsertValues(const_pointer position, size_type n, const value_type& value)
	{
		#if EASTL_ASSERT_ENABLED
			if(EASTL_UNLIKELY((position < mpBegin) || (position > mpEnd)))
				EASTL_FAIL_MSG("vector::insert -- invalid position");
		#endif

		// C++11 stipulates that position is const_iterator, but the return value is iterator.
		pointer destPosition = const_cast<value_type*>(position);

		if(n <= size_type(mCapacity - mpEnd)) // If n is <= capacity...
		{
			if(n > 0) // To do: See if there is a way we can eliminate this 'if' statement.
			{
				// To consider: Make this algorithm work more like DoInsertValue whereby a pointer to value is used.
				const value_type temp  = value;
				const size_type nExtra = static_cast<size_type>(mpEnd - destPosition);

				if(n < nExtra)
				{
//					eastl::uninitialized_move_ptr(mpEnd - n, mpEnd, mpEnd);
					std::uninitialized_move(mpEnd - n, mpEnd, mpEnd);
					std::move_backward(destPosition, mpEnd - n, mpEnd); // We need move_backward because of potential overlap issues.
					std::fill(destPosition, destPosition + n, temp);
				}
				else
				{
//					eastl::uninitialized_fill_n_ptr(mpEnd, n - nExtra, temp);
					std::uninitialized_fill_n(mpEnd, n - nExtra, temp);
//					eastl::uninitialized_move_ptr(destPosition, mpEnd, mpEnd + n - nExtra);
					std::uninitialized_move(destPosition, mpEnd, mpEnd + n - nExtra);
					std::fill(destPosition, mpEnd, temp);
				}

				mpEnd += n;
			}
		}
		else // else n > capacity
		{
			const size_type nPrevSize = size_type(mpEnd - mpBegin);
			// const size_type nGrowSize = GetNewCapacity(nPrevSize);
			// const size_type nNewSize  = nGrowSize > (nPrevSize + n) ? nGrowSize : (nPrevSize + n);
			auto			nNewHeap  = DoAllocate(nPrevSize + n);
			pointer const pNewData    = nNewHeap->begin();

			// #if EASTL_EXCEPTIONS_ENABLED
				pointer pNewEnd = pNewData;
				try
				{
					pNewEnd = safememory::uninitialized_move_ptr_if_noexcept(mpBegin, destPosition, pNewData);
					// eastl::uninitialized_fill_n_ptr(pNewEnd, n, value);
					std::uninitialized_fill_n(pNewEnd, n, value);
					pNewEnd = safememory::uninitialized_move_ptr_if_noexcept(destPosition, mpEnd, pNewEnd + n);
				}
				catch(...)
				{
					std::destroy(pNewData, pNewEnd);
					// DoFree(pNewData, nNewSize);
					throw;
				}
			// #else
			// 	pointer pNewEnd = eastl::uninitialized_move_ptr_if_noexcept(mpBegin, destPosition, pNewData);
			// 	eastl::uninitialized_fill_n_ptr(pNewEnd, n, value);
			// 	pNewEnd = eastl::uninitialized_move_ptr_if_noexcept(destPosition, mpEnd, pNewEnd + n);
			// #endif

			std::destroy(mpBegin, mpEnd);
			SetNewHeap(std::move(nNewHeap));
			// DoFree(mpBegin, (size_type)(mCapacity - mpBegin));

			// mpBegin    = pNewData;
			mpEnd      = pNewEnd;
			// mCapacity = pNewData + nNewSize;
		}
	}


	// template <typename T, typename Allocator>
	// void vector<T, Allocator>::DoClearCapacity() // This function exists because set_capacity() currently indirectly requires value_type to be default-constructible, 
	// {                                            // and some functions that need to clear our capacity (e.g. operator=) aren't supposed to require default-constructibility. 
	// 	clear();
	// 	this_type temp(std::move(*this));  // This is the simplest way to accomplish this, 
	// 	swap(temp);             // and it is as efficient as any other.
	// }


	template <typename T, typename Allocator>
	void vector<T, Allocator>::DoGrow(size_type n)
	{
		// TODO, review, if moved, sholdn't destroy...
		auto nNewHeap = DoAllocate(n);
		pointer const pNewData = nNewHeap->begin();

		pointer pNewEnd = safememory::uninitialized_move_ptr_if_noexcept(mpBegin, mpEnd, pNewData);

		std::destroy(mpBegin, mpEnd);
		SetNewHeap(std::move(nNewHeap));
		// DoFree(mpBegin, (size_type)(mCapacity - mpBegin));

		// mpBegin    = pNewData;
		mpEnd      = pNewEnd;
		// mCapacity = pNewData + n;
	}


	template <typename T, typename Allocator>
	inline void vector<T, Allocator>::DoSwap(this_type& x)
	{
		std::swap(mHeap,      x.mHeap);
		std::swap(mpBegin,    x.mpBegin);
		std::swap(mpEnd,      x.mpEnd);
		std::swap(mCapacity,  x.mCapacity); // We do this even if EASTL_ALLOCATOR_COPY_ENABLED is 0.
	}

	// The code duplication between this and the version that takes no value argument and default constructs the values
	// is unfortunate but not easily resolved without relying on C++11 perfect forwarding.
	template <typename T, typename Allocator>
	void vector<T, Allocator>::DoInsertValuesEnd(size_type n, const value_type& value)
	{
		if(n > size_type(mCapacity - mpEnd))
		{
			const size_type nPrevSize = size_type(mpEnd - mpBegin);
			// const size_type nGrowSize = GetNewCapacity(nPrevSize);
			// const size_type nNewSize = std::max(nGrowSize, nPrevSize + n);
			auto			nNewHeap = DoAllocate(nPrevSize + n);
			pointer const pNewData = nNewHeap->begin();

			// #if EASTL_EXCEPTIONS_ENABLED
				pointer pNewEnd = pNewData; // Assign pNewEnd a value here in case the copy throws.
				try
				{
					pNewEnd = safememory::uninitialized_move_ptr_if_noexcept(mpBegin, mpEnd, pNewData);
				}
				catch(...)
				{
					std::destroy(pNewData, pNewEnd);
					// DoFree(pNewData, nNewSize);
					throw;
				}
			// #else
			// 	pointer pNewEnd = eastl::uninitialized_move_ptr_if_noexcept(mpBegin, mpEnd, pNewData);
			// #endif

//			eastl::uninitialized_fill_n_ptr(pNewEnd, n, value);
			std::uninitialized_fill_n(pNewEnd, n, value);
			pNewEnd += n;

			std::destroy(mpBegin, mpEnd);
			SetNewHeap(std::move(nNewHeap));
			// DoFree(mpBegin, (size_type)(mCapacity - mpBegin));

			// mpBegin    = pNewData;
			mpEnd      = pNewEnd;
			// mCapacity = pNewData + nNewSize;
		}
		else
		{
			// eastl::uninitialized_fill_n_ptr(mpEnd, n, value);
			std::uninitialized_fill_n(mpEnd, n, value);
			mpEnd += n;
		}
	}

	template <typename T, typename Allocator>
	void vector<T, Allocator>::DoInsertValuesEnd(size_type n)
	{
		if (n > size_type(mCapacity - mpEnd))
		{
			const size_type nPrevSize = size_type(mpEnd - mpBegin);
			// const size_type nGrowSize = GetNewCapacity(nPrevSize);
			// const size_type nNewSize = std::max(nGrowSize, nPrevSize + n);
			auto			nNewHeap = DoAllocate(nPrevSize + n);
			pointer const pNewData = nNewHeap->begin();

			// #if EASTL_EXCEPTIONS_ENABLED
				pointer pNewEnd = pNewData;  // Assign pNewEnd a value here in case the copy throws.
				try 
				{
					pNewEnd = safememory::uninitialized_move_ptr_if_noexcept(mpBegin, mpEnd, pNewData);
				}
				catch (...)
				{
					std::destroy(pNewData, pNewEnd);
					// DoFree(pNewData, nNewSize);
					throw;
				}
			// #else
			// 	pointer pNewEnd = eastl::uninitialized_move_ptr_if_noexcept(mpBegin, mpEnd, pNewData);
			// #endif

//			eastl::uninitialized_default_fill_n(pNewEnd, n);
			std::uninitialized_value_construct_n(pNewEnd, n);
			pNewEnd += n;

			std::destroy(mpBegin, mpEnd);
			SetNewHeap(std::move(nNewHeap));
			// DoFree(mpBegin, (size_type)(mCapacity - mpBegin));

			// mpBegin = pNewData;
			mpEnd = pNewEnd;
			// mCapacity = pNewData + nNewSize;
		}
		else
		{
//			eastl::uninitialized_default_fill_n(mpEnd, n);
			std::uninitialized_value_construct_n(mpEnd, n);
			mpEnd += n;
		}
	}

	template <typename T, typename Allocator>
	template<typename... Args>
	void vector<T, Allocator>::DoInsertValue(const_pointer position, Args&&... args)
	{
		// To consider: It's feasible that the args is from a value_type comes from within the current sequence itself and 
		// so we need to be sure to handle that case. This is different from insert(position, const value_type&) because in 
		// this case value is potentially being modified.

		#if EASTL_ASSERT_ENABLED
			if(EASTL_UNLIKELY((position < mpBegin) || (position > mpEnd)))
				EASTL_FAIL_MSG("vector::insert/emplace -- invalid position");
		#endif

		// C++11 stipulates that position is const_iterator, but the return value is iterator.
		pointer destPosition = const_cast<value_type*>(position);

		if(mpEnd != mCapacity) // If size < capacity ...
		{
			// We need to take into account the possibility that args is a value_type that comes from within the vector itself.
			// creating a temporary value on the stack here is not an optimal way to solve this because sizeof(value_type) may be
			// too much for the given platform. An alternative solution may be to specialize this function for the case of the
			// argument being const value_type& or value_type&&.
			EASTL_ASSERT(position < mpEnd);                                 // While insert at end() is valid, our design is such that calling code should handle that case before getting here, as our streamlined logic directly doesn't handle this particular case due to resulting negative ranges.
			#if EASTL_USE_FORWARD_WORKAROUND
				auto value = value_type(std::forward<Args>(args)...);     // Workaround for compiler bug in VS2013 which results in a compiler internal crash while compiling this code.
			#else
				value_type  value(std::forward<Args>(args)...);           // Need to do this before the move_backward below because maybe args refers to something within the moving range.
			#endif
			::new(static_cast<void*>(mpEnd)) value_type(std::move(*(mpEnd - 1)));      // mpEnd is uninitialized memory, so we must construct into it instead of move into it like we do with the other elements below.
			std::move_backward(destPosition, mpEnd - 1, mpEnd);           // We need to go backward because of potential overlap issues.
			std::destroy_at(destPosition);
			::new(static_cast<void*>(destPosition)) value_type(std::move(value));                             // Move the value argument to the given position.
			++mpEnd;
		}
		else // else (size == capacity)
		{
			const size_type nPosSize  = size_type(destPosition - mpBegin); // Index of the insertion position.
			const size_type nPrevSize = size_type(mpEnd - mpBegin);
			// const size_type nNewSize  = GetNewCapacity(nPrevSize);
			auto			nNewHeap  = DoAllocate(nPrevSize + 1);
			pointer const   pNewData  = nNewHeap->begin();

			// #if EASTL_EXCEPTIONS_ENABLED
				pointer pNewEnd = pNewData;
				try
				{   // To do: We are not handling exceptions properly below.  In particular we don't want to 
					// call eastl::destruct on the entire range if only the first part of the range was costructed.
					::new((void*)(pNewData + nPosSize)) value_type(std::forward<Args>(args)...);              // Because the old data is potentially being moved rather than copied, we need to move.
					pNewEnd = NULL;                                                                             // Set to NULL so that in catch we can tell the exception occurred during the next call.
					pNewEnd = safememory::uninitialized_move_ptr_if_noexcept(mpBegin, destPosition, pNewData);       // the value first, because it might possibly be a reference to the old data being moved.
					pNewEnd = safememory::uninitialized_move_ptr_if_noexcept(destPosition, mpEnd, ++pNewEnd);
				}
				catch(...)
				{
					if(pNewEnd)
						std::destroy(pNewData, pNewEnd);                                         // Destroy what has been constructed so far.
					else
						std::destroy_at(pNewData + nPosSize);                                       // The exception occurred during the first unintialized move, so destroy only the value at nPosSize.
					// DoFree(pNewData, nNewSize);
					throw;
				}
			// #else
			// 	::new((void*)(pNewData + nPosSize)) value_type(std::forward<Args>(args)...);                  // Because the old data is potentially being moved rather than copied, we need to move 
			// 	pointer pNewEnd = eastl::uninitialized_move_ptr_if_noexcept(mpBegin, destPosition, pNewData);   // the value first, because it might possibly be a reference to the old data being moved.
			// 	pNewEnd = eastl::uninitialized_move_ptr_if_noexcept(destPosition, mpEnd, ++pNewEnd);            // Question: with exceptions disabled, do we asssume all operations are noexcept and thus there's no need for uninitialized_move_ptr_if_noexcept?
			// #endif

			std::destroy(mpBegin, mpEnd);
			SetNewHeap(std::move(nNewHeap));
			// DoFree(mpBegin, (size_type)(mCapacity - mpBegin));

			// mpBegin    = pNewData;
			mpEnd      = pNewEnd;
			// mCapacity = pNewData + nNewSize;
		}
	}


	template <typename T, typename Allocator>
	template<typename... Args>
	void vector<T, Allocator>::DoInsertValueEnd(Args&&... args)
	{
		const size_type nPrevSize = size_type(mpEnd - mpBegin);
		// const size_type nNewSize  = GetNewCapacity(nPrevSize);
		auto 			nNewHeap  = DoAllocate(nPrevSize + 1);
		pointer const   pNewData  = nNewHeap->begin();

		// #if EASTL_EXCEPTIONS_ENABLED
			pointer pNewEnd = pNewData; // Assign pNewEnd a value here in case the copy throws.
			try
			{
				pNewEnd = safememory::uninitialized_move_ptr_if_noexcept(mpBegin, mpEnd, pNewData);
				::new((void*)pNewEnd) value_type(std::forward<Args>(args)...);
				pNewEnd++;
			}
			catch(...)
			{
				std::destroy(pNewData, pNewEnd);
				// DoFree(pNewData, nNewSize);
				throw;
			}
		// #else
		// 	pointer pNewEnd = eastl::uninitialized_move_ptr_if_noexcept(mpBegin, mpEnd, pNewData);
		// 	::new((void*)pNewEnd) value_type(std::forward<Args>(args)...);
		// 	pNewEnd++;
		// #endif

		std::destroy(mpBegin, mpEnd);
		SetNewHeap(std::move(nNewHeap));
		// DoFree(mpBegin, (size_type)(mCapacity - mpBegin));

		// mpBegin    = pNewData;
		mpEnd      = pNewEnd;
		// mCapacity = pNewData + nNewSize;
	}

	/* static */
	template <typename T, typename Allocator>
	[[noreturn]]
	inline void vector<T, Allocator>::ThrowLengthException()
	{
		#if EASTL_EXCEPTIONS_ENABLED
			throw std::length_error("vector -- length_error");
		#elif EASTL_ASSERT_ENABLED
			EASTL_FAIL_MSG("vector -- length_error");
		#endif
	}


	/* static */
	template <typename T, typename Allocator>
	[[noreturn]]
	inline void vector<T, Allocator>::ThrowRangeException()
	{
		#if EASTL_EXCEPTIONS_ENABLED
			throw std::out_of_range("vector -- out of range");
		#elif EASTL_ASSERT_ENABLED
			EASTL_FAIL_MSG("vector -- out of range");
		#endif
	}


	/* static */
	template <typename T, typename Allocator>
	[[noreturn]]
	inline void vector<T, Allocator>::ThrowInvalidArgumentException()
	{
		#if EASTL_EXCEPTIONS_ENABLED
			throw std::invalid_argument("vector -- invalid argument");
		#elif EASTL_ASSERT_ENABLED
			EASTL_FAIL_MSG("vector -- invalid argument");
		#endif
	}

	/* static */
	template <typename T, typename Allocator>
	[[noreturn]]
	inline void vector<T, Allocator>::ThrowMaxSizeException()
	{
		#if EASTL_EXCEPTIONS_ENABLED
			throw std::out_of_range("vector -- size too big");
		#elif EASTL_ASSERT_ENABLED
			EASTL_FAIL_MSG("vector -- invalid argument");
		#endif
	}


	/* static */
	template <typename T, typename Allocator>
	inline typename vector<T, Allocator>::const_pointer_pair
	vector<T, Allocator>::CheckAndGet(csafe_it_arg itBegin, csafe_it_arg itEnd)
	{
		if(NODECPP_LIKELY(itBegin <= itEnd)) {
			const_pointer b = itBegin.get_raw_ptr();
			const_pointer e = itEnd.get_raw_ptr();

			return const_pointer_pair(b, e);
		}

		ThrowInvalidArgumentException();
	}

	template <typename T, typename Allocator>
	inline typename vector<T, Allocator>::const_pointer vector<T, Allocator>::CheckMineAndGet(csafe_it_arg it) const
	{
		if(NODECPP_LIKELY(it <= end())) {
			return it.get_raw_ptr();
		}

		ThrowInvalidArgumentException();
	}

	template <typename T, typename Allocator>
	inline typename vector<T, Allocator>::const_pointer_pair
	vector<T, Allocator>::CheckMineAndGet(csafe_it_arg itBegin, csafe_it_arg itEnd) const
	{
		if(NODECPP_LIKELY(itBegin <= itEnd && itEnd <= end())) {
			const_pointer b = itBegin.get_raw_ptr();
			const_pointer e = itEnd.get_raw_ptr();

			return const_pointer_pair(b, e);
		}

		ThrowInvalidArgumentException();
	}

	template <typename T, typename Allocator>
	inline bool vector<T, Allocator>::validate() const EA_NOEXCEPT
	{
		if(mpEnd < mpBegin)
			return false;
		if(mCapacity < mpEnd)
			return false;
		return true;
	}


	template <typename T, typename Allocator>
	inline int vector<T, Allocator>::validate_iterator(const_pointer i) const EA_NOEXCEPT
	{
		if(i >= mpBegin)
		{
			if(i < mpEnd)
				return (isf_valid | isf_current | isf_can_dereference);

			if(i <= mpEnd)
				return (isf_valid | isf_current);
		}

		return isf_none;
	}

	template <typename T, typename Allocator>
	inline int vector<T, Allocator>::validate_iterator(csafe_it_arg i) const EA_NOEXCEPT
	{
		auto p = CheckMineAndGet(i);
		return validate_iterator(p);
	}


	///////////////////////////////////////////////////////////////////////
	// global operators
	///////////////////////////////////////////////////////////////////////

	template <typename T, typename Allocator>
	inline bool operator==(const vector<T, Allocator>& a, const vector<T, Allocator>& b)
	{
		return ((a.size() == b.size()) && std::equal(a.begin(), a.end(), b.begin()));
	}


	template <typename T, typename Allocator>
	inline bool operator!=(const vector<T, Allocator>& a, const vector<T, Allocator>& b)
	{
		return ((a.size() != b.size()) || !std::equal(a.begin(), a.end(), b.begin()));
	}


	template <typename T, typename Allocator>
	inline bool operator<(const vector<T, Allocator>& a, const vector<T, Allocator>& b)
	{
		return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end());
	}


	template <typename T, typename Allocator>
	inline bool operator>(const vector<T, Allocator>& a, const vector<T, Allocator>& b)
	{
		return b < a;
	}


	template <typename T, typename Allocator>
	inline bool operator<=(const vector<T, Allocator>& a, const vector<T, Allocator>& b)
	{
		return !(b < a);
	}


	template <typename T, typename Allocator>
	inline bool operator>=(const vector<T, Allocator>& a, const vector<T, Allocator>& b)
	{
		return !(a < b);
	}


	template <typename T, typename Allocator>
	inline void swap(vector<T, Allocator>& a, vector<T, Allocator>& b) EA_NOEXCEPT_IF(EA_NOEXCEPT_EXPR(a.swap(b)))
	{
		a.swap(b);
	}


} // namespace safememory


#ifdef _MSC_VER
	#pragma warning(pop)
#endif


#endif // Header include guard











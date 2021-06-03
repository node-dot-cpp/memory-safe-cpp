/////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////


#include "EASTLTest.h"
#include "TestMap.h"
#include "TestSet.h"
// #include <safememory/unordered_set.h>
#include <safememory/unordered_map.h>
// #include <EASTL/unordered_set.h>
// #include <EASTL/unordered_map.h>
#include <map>
#include <string>
#include <algorithm>
#include <vector>

EA_DISABLE_ALL_VC_WARNINGS()
#include <string.h>
EA_RESTORE_ALL_VC_WARNINGS()


// using safememory::unordered_map;
// using safememory::unordered_multimap;
//using safememory::unordered_map;
//using safememory::unordered_multimap;

// using safememory::unordered_set;
// using safememory::unordered_multiset;
//using safememory::unordered_set;
//using safememory::unordered_multiset;
// using safememory::detail::iterator_validity;


namespace std
{
	template <> 
	struct hash<Align32>
	{
		size_t operator()(const Align32& a32) const 
			{ return static_cast<size_t>(a32.mX); }
	};

	// extension to hash an eastl::pair
	template <typename T1, typename T2>
	struct hash<pair<T1, T2>>
	{
		size_t operator()(const pair<T1, T2>& c) const
		{
			return static_cast<size_t>(hash<T1>()(c.first) ^ hash<T2>()(c.second));
		}
	};
}

// For regression code below.
class HashRegressionA { public: int x; };
class HashRegressionB { public: int y; };


// For regression code below.
struct Struct {
	char8_t name[128];
};



// What we are doing here is creating a special case of a hashtable where the key compare
// function is not the same as the value operator==. 99% of the time when you create a 
// hashtable the key compare (predicate) is simply key_equal or something else that's
// identical to operator== for the hashtable value type. But for some tests we want
// to exercise the case that these aren't different. A result of this difference is that
// you can lookup an element in a hash table and the returned value is not == to the 
// value you looked up, because it succeeds the key compare but not operator==.
struct HashtableValue
{
	HashtableValue(std::size_t d = 0, std::size_t e = 0) : mData(d), mExtra(e){}
	void Set(std::size_t d, std::size_t e = 0) { mData = d; mExtra = e; }

	std::size_t mData;
	std::size_t mExtra;
};

bool operator==(const HashtableValue& htv1, const HashtableValue& htv2)
{
	return (htv1.mData == htv2.mData) && (htv1.mExtra == htv2.mExtra); // Fully compare the HashTableValue.
}

struct HashtableValuePredicate
{
	bool operator()(const HashtableValue& htv1, const HashtableValue& htv2) const
		{ return (htv1.mData == htv2.mData); } // Compare just the mData portion of HashTableValue.
};

struct HashtableValueHash
{
	size_t operator()(const HashtableValue& htv) const 
		{ return static_cast<size_t>(htv.mData); }
};




// Explicit Template instantiations.
// These tell the compiler to compile all the functions for the given class.
// template class eastl::hashtable<int,
//                                 std::pair<const int, int>,
// 								safememory::memory_safety::safe,
//                                 safememory::detail::use_first<std::pair<const int, int>>,
//                                 std::equal_to<int>,
//                                 std::hash<int>,
//                                 safememory::detail::mod_range_hashing,
//                                 safememory::detail::default_ranged_hash,
//                                 safememory::detail::prime_rehash_policy,
//                                 true, // bCacheHashCode
//                                 true, // bMutableIterators
//                                 true  // bUniqueKeys
//                                 >;
// template class eastl::hashtable<int,
// 								std::pair<const int, int>,
// 								safememory::memory_safety::safe,
// 								safememory::detail::use_first<std::pair<const int, int>>,
// 								std::equal_to<int>,
// 								std::hash<int>,
// 								safememory::detail::mod_range_hashing,
// 								safememory::detail::default_ranged_hash,
// 								safememory::detail::prime_rehash_policy,
// 								false, // bCacheHashCode
// 								true,  // bMutableIterators
// 								true   // bUniqueKeys
// 								>;
// TODO(rparolin): known compiler error, we should fix this.
// template class eastl::hashtable<int,
//                                 eastl::pair<const int, int>,
//                                 eastl::allocator,
//                                 eastl::use_first<eastl::pair<const int, int>>,
//                                 eastl::equal_to<int>,
//                                 eastl::hash<int>,
//                                 mod_range_hashing,
//                                 default_ranged_hash,
//                                 prime_rehash_policy,
//                                 false, // bCacheHashCode
//                                 true,  // bMutableIterators
//                                 false  // bUniqueKeys
//                                 >;

// Note these will only compile non-inherited functions.  We provide explicit
// template instantiations for the hashtable base class above to get compiler
// coverage of those inherited hashtable functions.
// template class eastl::unordered_set<int>;
// template class eastl::unordered_multiset<int>;
//template typename safememory::unordered_map<int, int>;
//template typename safememory::unordered_multimap<int, int>;
// template class eastl::unordered_set<Align32>;
// template class eastl::unordered_multiset<Align32>;
//template typename safememory::unordered_map<Align32, Align32>;
//template typename safememory::unordered_multimap<Align32, Align32>;

// validate static assumptions about hashtable core types
// typedef safememory::detail::hash_node<int, safememory::memory_safety::safe, false> HashNode1;
// typedef safememory::detail::hash_node<int, safememory::memory_safety::safe, true> HashNode2;
// typedef safememory::detail::hash_node<int, safememory::memory_safety::none, false> HashNode3;
// typedef safememory::detail::hash_node<int, safememory::memory_safety::none, true> HashNode4;

// static_assert(std::is_default_constructible<HashNode1>::value, "hash_node static error");
// static_assert(std::is_default_constructible<HashNode2>::value, "hash_node static error");
// static_assert(std::is_default_constructible<HashNode3>::value, "hash_node static error");
// static_assert(std::is_default_constructible<HashNode4>::value, "hash_node static error");
// static_assert(std::is_copy_constructible<HashNode1>::value, "hash_node static error");
// static_assert(std::is_copy_constructible<HashNode2>::value, "hash_node static error");
// static_assert(std::is_move_constructible<HashNode1>::value, "hash_node static error");
// static_assert(std::is_move_constructible<HashNode2>::value, "hash_node static error");
// static_assert(std::is_move_constructible<HashNode3>::value, "hash_node static error");
// static_assert(std::is_move_constructible<HashNode4>::value, "hash_node static error");

// A custom hash function that has a high number of collisions is used to ensure many keys share the same hash value.
struct colliding_hash
{
	size_t operator()(const int& val) const 
		{ return static_cast<size_t>(val % 3); }
};


template<template<typename> typename SET, template<typename, typename, typename> typename SET2>
int TestHashSet()
{   
	int nErrorCount = 0;

	{  // Test declarations
		SET<int>           hashSet;

		SET<int> hashSet2(hashSet);
		EATEST_VERIFY(hashSet2.size() == hashSet.size());
		EATEST_VERIFY(hashSet2 == hashSet);



		// const key_equal& key_eq() const;
		// key_equal&       key_eq();
		// unordered_set<int>       hs;
		// const unordered_set<int> hsc;

		// const unordered_set<int>::key_equal& ke = hsc.key_eq();
		// hs.key_eq() = ke;


		// const char*     get_name() const;
		// void            set_name(const char* pName);
		// #if EASTL_NAME_ENABLED
		// 	hashMap.get_allocator().set_name("test");
		// 	const char* pName = hashMap.get_allocator().get_name();
		// 	EATEST_VERIFY(equal(pName, pName + 5, "test"));
		// #endif
	}


	{
		SET<int> hashSet;

		// Clear a newly constructed, already empty container.
		hashSet.clear(true);
		EATEST_VERIFY(hashSet.validate());
		EATEST_VERIFY(hashSet.size() == 0);
		EATEST_VERIFY(hashSet.bucket_count() == 1);

		for(int i = 0; i < 100; ++i)
			hashSet.insert(i);
		EATEST_VERIFY(hashSet.validate());
		EATEST_VERIFY(hashSet.size() == 100);

		hashSet.clear(true);
		EATEST_VERIFY(hashSet.validate());
		EATEST_VERIFY(hashSet.size() == 0);
		EATEST_VERIFY(hashSet.bucket_count() == 1);

		for(int i = 0; i < 100; ++i)
			hashSet.insert(i);
		EATEST_VERIFY(hashSet.validate());
		EATEST_VERIFY(hashSet.size() == 100);

		hashSet.clear(true);
		EATEST_VERIFY(hashSet.validate());
		EATEST_VERIFY(hashSet.size() == 0);
		EATEST_VERIFY(hashSet.bucket_count() == 1);
	}


	{   // Test unordered_set

		// size_type          size() const
		// bool               empty() const
		// insert_return_type insert(const value_type& value);
		// insert_return_type insert(const value_type& value, hash_code_t c, node_type* pNodeNew = NULL);
		// iterator           insert(const_iterator, const value_type& value);
		// iterator           find(const key_type& k);
		// const_iterator     find(const key_type& k) const;
		// size_type          count(const key_type& k) const;

		typedef SET<int> HashSetInt;

		HashSetInt hashSet;
		const typename HashSetInt::size_type kCount = 10000;

		EATEST_VERIFY(hashSet.empty());
		EATEST_VERIFY(hashSet.size() == 0);
		EATEST_VERIFY(hashSet.count(0) == 0);

		for(int i = 0; i < (int)kCount; i++)
			hashSet.insert(i);

		EATEST_VERIFY(!hashSet.empty());
		EATEST_VERIFY(hashSet.size() == kCount);
		EATEST_VERIFY(hashSet.count(0) == 1);

		for(typename HashSetInt::iterator it = hashSet.begin(); it != hashSet.end(); ++it)
		{
			int value = *it;
			EATEST_VERIFY(value < (int)kCount);
		}

		for(int i = 0; i < (int)kCount * 2; i++)
		{
			typename HashSetInt::iterator it = hashSet.find(i);

			if(i < (int)kCount)
				EATEST_VERIFY(it != hashSet.end());
			else
				EATEST_VERIFY(it == hashSet.end());
		}

		// insert_return_type insert(const value_type& value, hash_code_t c, node_type* pNodeNew = NULL);
		// typename HashSetInt::node_type* pNode = hashSet.allocate_uninitialized_node();
		// typename HashSetInt::insert_return_type r = hashSet.insert(std::hash<int>()(999999), pNode, 999999);
		// EATEST_VERIFY(r.second == true);
		// pNode = hashSet.allocate_uninitialized_node();
		// r = hashSet.insert(std::hash<int>()(999999), pNode, 999999);
		// EATEST_VERIFY(r.second == false);
		// hashSet.free_uninitialized_node(pNode);
		// hashSet.erase(999999);


		// iterator       begin();
		// const_iterator begin() const;
		// iterator       end();
		// const_iterator end() const;

		int* const pIntArray = new int[kCount];
		memset(pIntArray, 0, kCount * sizeof(int)); // We want to make sure each element is present only once.
		int nCount = 0;

		for(typename HashSetInt::iterator it = hashSet.begin(); it != hashSet.end(); ++it, ++nCount)
		{
			int i = *it;

			EATEST_VERIFY((i >= 0) && (i < (int)kCount) && (pIntArray[i] == 0));
			pIntArray[i] = 1;
		}

		EATEST_VERIFY(nCount == (int)kCount);
		delete[] pIntArray;
	}


	{
		// size_type bucket_count() const
		// size_type bucket_size(size_type n) const
		// float load_factor() const
		// float get_max_load_factor() const;
		// void  set_max_load_factor(float fMaxLoadFactor);
		// void rehash(size_type n);
		// const RehashPolicy& rehash_policy() const
		// void  rehash_policy(const RehashPolicy& rehashPolicy);

		typedef SET<int> HashSetInt;

		HashSetInt hashSet;

		float fLoadFactor = hashSet.load_factor();
		EATEST_VERIFY(fLoadFactor == 0.f);

		hashSet.set_max_load_factor(65536.f * 512.f);
		float fMaxLoadFactor = hashSet.get_max_load_factor();
		EATEST_VERIFY(fMaxLoadFactor == (65536.f * 512.f));

		hashSet.rehash(20);
		typename HashSetInt::size_type n = hashSet.bucket_count();
		EATEST_VERIFY((n >= 20) && (n < 25));
		
		for(int i = 0; i < 100000; i++)
			hashSet.insert(i); // This also tests for high loading.

		typename HashSetInt::size_type n2 = hashSet.bucket_count();
		EATEST_VERIFY(n2 == n); // Verify no rehashing has occured, due to our high load factor.

		n = hashSet.bucket_size(0);
		EATEST_VERIFY(n >= ((hashSet.size() / hashSet.bucket_count()) / 2)); // It will be some high value. We divide by 2 to give it some slop.
		EATEST_VERIFY(hashSet.validate());

		// unordered_set<int>::rehash_policy_type rp = hashSet.rehash_policy();
		// rp.mfGrowthFactor = 1.5f;
		// hashSet.rehash_policy(rp);
		// EATEST_VERIFY(hashSet.validate());


		// local_iterator       begin(size_type n);
		// local_iterator       end(size_type n);
		// const_local_iterator begin(size_type n) const;
		// const_local_iterator end(size_type n) const;

		typename HashSetInt::size_type b = hashSet.bucket_count() - 1;
		eastl::hash<int> IntHash;
		for(typename HashSetInt::const_local_iterator cli = hashSet.begin(b); cli != hashSet.end(b); ++cli)
		{
			int v = *cli;
			EATEST_VERIFY((IntHash(v) % hashSet.bucket_count()) == b);
		}


		// clear();

		hashSet.clear();
		EATEST_VERIFY(hashSet.validate());
		EATEST_VERIFY(hashSet.empty());
		EATEST_VERIFY(hashSet.size() == 0);
		EATEST_VERIFY(hashSet.count(0) == 0);

		hashSet.clear(true);
		EATEST_VERIFY(hashSet.validate());
		EATEST_VERIFY(hashSet.bucket_count() == 1);
	}


	{
		// void reserve(size_type nElementCount);
		nErrorCount += HashContainerReserveTest<SET<int>>()();
	}


	{   // Test unordered_set with cached hash code.

		// insert_return_type insert(const value_type& value) ;
		// iterator       find(const key_type& k);
		// const_iterator find(const key_type& k) const;

		typedef SET<int> HashSetIntC;

		HashSetIntC hashSet;
		const int kCount = 10000;

		for(int i = 0; i < kCount; i++)
			hashSet.insert(i);

		for(typename HashSetIntC::iterator it = hashSet.begin(); it != hashSet.end(); ++it)
		{
			int value = *it;
			EATEST_VERIFY(value < kCount);
		}

		for(int i = 0; i < kCount * 2; i++)
		{
			typename HashSetIntC::iterator it = hashSet.find(i);
			if(i < kCount)
				EATEST_VERIFY(it != hashSet.end());
			else
				EATEST_VERIFY(it == hashSet.end());
		}
	}

	// {
	// 	// ENABLE_IF_HASHCODE_U32(HashCodeT, iterator)       find_by_hash(HashCodeT c)
	// 	// ENABLE_IF_HASHCODE_U32(HashCodeT, const_iterator) find_by_hash(HashCodeT c) const
	// 	{
	// 		// NOTE(rparolin):
	// 		// these overloads of find_by_hash contains a static assert that forces a compiler error in the event it is
	// 		// used with a hashtable configured to not cache the hash value in the node.
	// 	}

	// 	// iterator                                          find_by_hash(const key_type& k, hash_code_t c)
	// 	// const_iterator                                    find_by_hash(const key_type& k, hash_code_t c) const
	// 	#ifdef EA_COMPILER_CPP14_ENABLED 
	// 	{
	// 		auto FindByHashTest = [&nErrorCount](auto& hashSet)
	// 		{
	// 			const int kCount = 10000;
	// 			for(int i = 0; i < kCount; i++)
	// 				hashSet.insert(i);

	// 			for(int i = 0; i < kCount * 2; i++)
	// 			{
	// 				auto it = hashSet.find_by_hash(i, i);

	// 				if(i < kCount)
	// 					EATEST_VERIFY(it != hashSet.end());
	// 				else
	// 					EATEST_VERIFY(it == hashSet.end());
	// 			}
	// 		};

	// 		{
	// 			typedef unordered_set<int, std::hash<int>, std::equal_to<int>, std::allocator<int>, true> HashSetIntC;
	// 			HashSetIntC hashSetC;
	// 			FindByHashTest(hashSetC);

	// 			typedef unordered_set<int, std::hash<int>, std::equal_to<int>, std::allocator<int>, false> HashSetInt;
	// 			HashSetInt hashSet;
	// 			FindByHashTest(hashSet);
	// 		}
	// 	}
	// 	#endif
	// }


	// {
	// 	// unordered_set(const allocator_type& allocator);
	// 	// hashtable& operator=(const this_type& x);
	// 	// bool validate() const;

	// 	unordered_set<int> hashSet1(EASTLAllocatorType("unordered_set name"));
	// 	unordered_set<int> hashSet2(hashSet1);

	// 	for(int i = 0; i < 10; i++)
	// 	{
	// 		hashSet1.insert(i);
	// 		hashSet2.insert(i);
	// 	}

	// 	hashSet1 = hashSet2;

	// 	EATEST_VERIFY(hashSet1.validate());
	// 	EATEST_VERIFY(hashSet2.validate());
	// }


	{
		// unordered_set(size_type nBucketCount, const Hash& hashFunction = Hash(), const Predicate& predicate = Predicate(), const allocator_type& allocator);
		// hashtable(const hashtable& x);
		// hashtable& operator=(const this_type& x);
		// void swap(this_type& x);
		// bool validate() const;
		{
			SET<int> hashSet3(0);
			SET<int> hashSet4(1);
			SET<int> hashSet5(2);
			SET<int> hashSet6(3);
			SET<int> hashSet7(4);

			hashSet4 = hashSet3;
			hashSet6 = hashSet5;
			hashSet3 = hashSet7;

			for(int i = 0; i < 10; i++)
			{
				hashSet3.insert(i);
				hashSet4.insert(i);
				hashSet5.insert(i);
				hashSet6.insert(i);
				hashSet7.insert(i);
			}

			hashSet4 = hashSet3;
			hashSet6 = hashSet5;
			hashSet3 = hashSet7;

			EATEST_VERIFY(hashSet3.validate());
			EATEST_VERIFY(hashSet4.validate());
			EATEST_VERIFY(hashSet5.validate());
			EATEST_VERIFY(hashSet6.validate());
			EATEST_VERIFY(hashSet7.validate());

			// using namespace std;
			swap(hashSet4, hashSet3);
			swap(hashSet6, hashSet5);
			swap(hashSet3, hashSet7);

			EATEST_VERIFY(hashSet3.validate());
			EATEST_VERIFY(hashSet4.validate());
			EATEST_VERIFY(hashSet5.validate());
			EATEST_VERIFY(hashSet6.validate());
			EATEST_VERIFY(hashSet7.validate());

			SET<int> hashSet8(hashSet6);
			SET<int> hashSet9(hashSet7);
			SET<int> hashSet10(hashSet8);

			EATEST_VERIFY(hashSet8.validate());
			EATEST_VERIFY(hashSet9.validate());
			EATEST_VERIFY(hashSet10.validate());
		}
		
		// test hashtable::swap using different allocator instances
		// {
		// 	typedef unordered_set<int, std::hash<int>, std::equal_to<int>, InstanceAllocator> HS;
		// 	HS hashSet1(InstanceAllocator("unordered_set1 name", 111));
		// 	HS hashSet2(InstanceAllocator("unordered_set2 name", 222));

		// 	for(int i = 0; i < 10; i++)
		// 	{
		// 		hashSet1.insert(i);
		// 		hashSet2.insert(i+10);
		// 	}

		// 	hashSet2.swap(hashSet1);

		// 	EATEST_VERIFY(hashSet1.validate());
		// 	EATEST_VERIFY(hashSet2.validate());

		// 	EATEST_VERIFY(hashSet1.get_allocator().mInstanceId == 222);
		// 	EATEST_VERIFY(hashSet2.get_allocator().mInstanceId == 111);

		// 	EATEST_VERIFY(std::all_of(std::begin(hashSet2), std::end(hashSet2), [](int i) { return i < 10; }));
		// 	EATEST_VERIFY(std::all_of(std::begin(hashSet1), std::end(hashSet1), [](int i) { return i >= 10; }));
		// }
	}


	{
		// unordered_set(InputIterator first, InputIterator last, size_type nBucketCount = 8, const Hash& hashFunction = Hash(), const Predicate& predicate = Predicate(), const allocator_type& allocator);
		// bool validate() const;

		// std::vector<int> intArray;
		// for(int i = 0; i < 1000; i++)
		// 	intArray.push_back(i);

		// unordered_set<int> hashSet1(intArray.begin(), intArray.end(), 0);
		// unordered_set<int> hashSet2(intArray.begin(), intArray.end(), 1);
		// unordered_set<int> hashSet3(intArray.begin(), intArray.end(), 2);
		// unordered_set<int> hashSet4(intArray.begin(), intArray.end(), 3);

		// EATEST_VERIFY(hashSet1.validate());
		// EATEST_VERIFY(hashSet2.validate());
		// EATEST_VERIFY(hashSet3.validate());
		// EATEST_VERIFY(hashSet4.validate());


		// bool validate_iterator(const_iterator i) const;
		// unordered_set<int> hashSet1({1,2,3,4});
		// unordered_set<int> hashSet2({1,2,3,4});

		// unordered_set<int>::iterator it;
		// auto result = hashSet1.validate_iterator(it);
		// EATEST_VERIFY(result == iterator_validity::Null);

		// it = hashSet1.begin();
		// result = hashSet2.validate_iterator(it);
		// EATEST_VERIFY(result == iterator_validity::InvalidZoombie);
		// result = hashSet1.validate_iterator(it);
		// EATEST_VERIFY(result == iterator_validity::ValidCanDeref);

		// it = hashSet1.end();
		// result = hashSet1.validate_iterator(it);
		// EATEST_VERIFY(result == iterator_validity::ValidEnd);


		// void reset_lose_memory();
		// hashSet1.reset_lose_memory();
		// hashSet1 = hashSet2;

		// EATEST_VERIFY(hashSet1.validate());
		// EATEST_VERIFY(hashSet2.validate());

		// hashSet3.reset_lose_memory();
		// hashSet4 = hashSet3;

		// EATEST_VERIFY(hashSet3.validate());
		// EATEST_VERIFY(hashSet4.validate());

		// hashSet2.reset_lose_memory();
		// hashSet3.reset_lose_memory();
		// swap(hashSet2, hashSet3);

		// EATEST_VERIFY(hashSet3.validate());
		// EATEST_VERIFY(hashSet4.validate());

		// hashSet2 = hashSet3;
		// EATEST_VERIFY(hashSet2.validate());
	}


	{
		// void insert(InputIterator first, InputIterator last);
		// std::vector<int> intArray1;
		// std::vector<int> intArray2;

		// for(int i = 0; i < 1000; i++)
		// {
		// 	intArray1.push_back(i + 0);
		// 	intArray2.push_back(i + 500);
		// }

		// unordered_set<int> hashSet1(intArray1.begin(), intArray1.end());
		// hashSet1.insert(intArray2.begin(), intArray2.end());
		// EATEST_VERIFY(hashSet1.validate());

		// unordered_set<int> hashSet2;
		// hashSet2.insert(intArray1.begin(), intArray1.end());
		// hashSet2.insert(intArray2.begin(), intArray2.end());
		// EATEST_VERIFY(hashSet2.validate());

		// EATEST_VERIFY(hashSet1 == hashSet2);


		// // insert_return_type insert(const_iterator, const value_type& value)
		// for(int j = 0; j < 1000; j++)
		// 	hashSet1.insert(hashSet1.begin(), j);

		// std::insert_iterator< safememory::unordered_set<int> > ii(hashSet1, hashSet1.begin());
		// for(int j = 0; j < 1000; j++)
		// 	*ii++ = j;
	}


	{
		// C++11 emplace and related functionality
		// nErrorCount += TestSetCpp11<safememory::SET<TestObject, hash_TestObject>>();
	}

	{
		// initializer_list support.
		// unordered_set(std::initializer_list<value_type> ilist, size_type nBucketCount = 0, const Hash& hashFunction = Hash(), 
		//            const Predicate& predicate = Predicate(), const allocator_type& allocator = EASTL_unordered_set_DEFAULT_ALLOCATOR)
		// this_type& operator=(std::initializer_list<value_type> ilist);
		// void insert(std::initializer_list<value_type> ilist);
		#if !defined(EA_COMPILER_NO_INITIALIZER_LISTS)
			SET<int> intHashSet = { 12, 13, 14 };
			EATEST_VERIFY(intHashSet.size() == 3);
			EATEST_VERIFY(intHashSet.find(12) != intHashSet.end());
			EATEST_VERIFY(intHashSet.find(13) != intHashSet.end());
			EATEST_VERIFY(intHashSet.find(14) != intHashSet.end());

			intHashSet = { 22, 23, 24 };
			EATEST_VERIFY(intHashSet.size() == 3);
			EATEST_VERIFY(intHashSet.find(22) != intHashSet.end());
			EATEST_VERIFY(intHashSet.find(23) != intHashSet.end());
			EATEST_VERIFY(intHashSet.find(24) != intHashSet.end());

			intHashSet.insert({ 42, 43, 44 });
			EATEST_VERIFY(intHashSet.size() == 6);
			EATEST_VERIFY(intHashSet.find(42) != intHashSet.end());
			EATEST_VERIFY(intHashSet.find(43) != intHashSet.end());
			EATEST_VERIFY(intHashSet.find(44) != intHashSet.end());
		#endif
	}

	{
		// eastl::pair<iterator, iterator>             equal_range(const key_type& k);
		// eastl::pair<const_iterator, const_iterator> equal_range(const key_type& k) const;
		// const_iterator  erase(const_iterator, const_iterator);
		// size_type       erase(const key_type&);
		// To do.
	}


	// {   
	// 	// template <typename U, typename UHash, typename BinaryPredicate>
	// 	// iterator find_as(const U& u, UHash uhash, BinaryPredicate predicate);
	// 	// template <typename U, typename UHash, typename BinaryPredicate>
	// 	// const_iterator find_as(const U& u, UHash uhash, BinaryPredicate predicate) const;
	// 	// template <typename U>
	// 	// iterator find_as(const U& u);
	// 	// template <typename U>
	// 	// const_iterator find_as(const U& u) const;

	// 	typedef SET<std::string> HashSetString;

	// 	HashSetString hashSet;
	// 	const int kCount = 100;

	// 	for(int i = 0; i < kCount; i++)
	// 	{
	// 		// string::CtorSprintf cs; // GCC 2.x doesn't like this value being created in the ctor below.
	// 		std::string s = std::to_string(i);
	// 		hashSet.insert(s);
	// 	}

	// 	for(int i = 0; i < kCount * 2; i++)
	// 	{
	// 		char pString[32];
	// 		sprintf(pString, "%d", i);

	// 		HashSetString::iterator it = hashSet.find_as(pString);
	// 		if(i < kCount)
	// 			EATEST_VERIFY(it != hashSet.end());
	// 		else
	// 			EATEST_VERIFY(it == hashSet.end());

	// 		it = hashSet.find_as(pString, hash<const char*>(), equal_to_2<string, const char*>());
	// 		if(i < kCount)
	// 			EATEST_VERIFY(it != hashSet.end());
	// 		else
	// 			EATEST_VERIFY(it == hashSet.end());
	// 	}
	// }

	// {
	// 	// Test const containers.
	// 	const SET<int> constHashSet;

	// 	SET<int>::const_iterator i = constHashSet.begin();
	// 	SET<int>::const_iterator i3 = i;
	// 	SET<int>::iterator i2;
	// 	i3 = i2;

	// 	EATEST_VERIFY(i3 == i2);

	// 	//const std::tr1::unordered_set<int> constUSet;
	// 	//std::tr1::unordered_set<int>::const_iterator i = constUSet.begin();
	// 	//*i = 0;
	// }

	{
		// global operator ==, !=
		EASTLTest_Rand rng(GetRandSeed());
		const std::size_t kIterationCount = 100;
		const std::size_t kDataRange = 50;

		{
			typedef SET2<HashtableValue, HashtableValueHash, HashtableValuePredicate> HashSet;
			HashtableValue value;

			HashSet h1;
			HashSet h2;
			EATEST_VERIFY(h1 == h2);

			for(std::size_t i = 0; i < kIterationCount; i++)
			{
				value.mData = rng.RandLimit(kDataRange);
				h1.insert(value);  // Leave value.mExtra as 0.
			}

			EATEST_VERIFY(h1 != h2);
			h2 = h1;
			EATEST_VERIFY(h1 == h2);

			// Test the case of the containers being the same size but having a single different value, despite that it's key compare yields equal.
			HashSet h2Saved(h2);
			typename HashSet::iterator it = h2.find(value);
			HashtableValue valueModified(value.mData, 1);
			h2.erase(it);
			h2.insert(valueModified);
			EATEST_VERIFY(h1 != h2);
			h2 = h2Saved;

			// Test the case of the containers being the same size but having a single different key.
			h2Saved = h2;
			h2.erase(h2.find(value));
			h2.insert(kDataRange); // Insert something that could not have been in h2.
			EATEST_VERIFY(h1 != h2);
			h2 = h2Saved;

			h1.erase(h1.find(value)); // Erase from h1 whatever the last value was.
			EATEST_VERIFY(h1 != h2);
		}

	}

	// test hashtable holding move-only types
	#if !defined(EA_COMPILER_MSVC_2013)
	{
		struct Movable
		{
			Movable() {}
			Movable(Movable&&) = default;
			Movable& operator=(Movable&&) = default;
			Movable(const Movable&) = delete;
			Movable& operator=(const Movable&) = delete;

			bool operator==(Movable) const { return true; }

			struct Hash
			{
				size_t operator()(Movable) const { return 0; }
			};
		};

		// using namespace std;
		SET2<Movable, typename Movable::Hash, safememory::equal_to<Movable>> a, b;
		swap(a,b);
	}
	#endif

	{
		// hashtable(this_type&& x);
		// hashtable(this_type&& x, const allocator_type& allocator);
		// this_type& operator=(this_type&& x);

		// template <class... Args>
		// insert_return_type emplace(Args&&... args);

		// template <class... Args>
		// iterator emplace_hint(const_iterator position, Args&&... args);

		// template <class P> // Requires that "value_type is constructible from forward<P>(otherValue)."
		// insert_return_type insert(P&& otherValue);

		// iterator insert(const_iterator hint, value_type&& value);

		// Regression of user reported compiler error in hashtable sfinae mechanism 
		{
			TestObject::Reset();
			SET2<TestObject, hash_TestObject, std::equal_to<TestObject>> toSet;
			toSet.emplace(3, 4, 5);
		}
	}

	return nErrorCount;
}

template<template<typename> typename MSET, template<typename, typename, typename> typename MSET2>
int TestHashMultiSet()
{   
	int nErrorCount = 0;

	{  // Test declarations
		MSET<int>      hashMultiSet;

		MSET<int> hashMultiSet2(hashMultiSet);
		EATEST_VERIFY(hashMultiSet2.size() == hashMultiSet.size());
		EATEST_VERIFY(hashMultiSet2 == hashMultiSet);
	}

	{
		// void reserve(size_type nElementCount);
		nErrorCount += HashContainerReserveTest<MSET<int>>()();
	}

	{
		// C++11 emplace and related functionality
		// nErrorCount += TestMultisetCpp11<safememory::MSET<TestObject, hash_TestObject>>();
	}

	{
		// global operator ==, !=
		EASTLTest_Rand rng(GetRandSeed());
		const std::size_t kIterationCount = 100;
		const std::size_t kDataRange = 50;

		{
			typedef MSET2<HashtableValue, HashtableValueHash, HashtableValuePredicate> HashSet;
			HashtableValue value;

			HashSet h1;
			HashSet h2;
			EATEST_VERIFY(h1 == h2);

			for(std::size_t i = 0; i < kIterationCount; i++)
			{
				value.mData = rng.RandLimit(kDataRange);
				h1.insert(value);  // Leave value.mExtra as 0.
			}

			EATEST_VERIFY(h1 != h2);
			h2 = h1;
			EATEST_VERIFY(h1 == h2);

			// Test the case of the containers being the same size but having a single different value, despite that it's key compare yields equal.
			HashSet h2Saved(h2);
			typename HashSet::iterator it = h2.find(value);
			HashtableValue valueModified(value.mData, 1);
			h2.erase(it);
			h2.insert(valueModified);
			EATEST_VERIFY(h1 != h2);
			h2 = h2Saved;

			// Test the case of the containers being the same size but having a single different key.
			h2Saved = h2;
			h2.erase(h2.find(value));
			h2.insert(kDataRange); // Insert something that could not have been in h2.
			EATEST_VERIFY(h1 != h2);
			h2 = h2Saved;

			h1.erase(h1.find(value)); // Erase from h1 whatever the last value was.
			EATEST_VERIFY(h1 != h2);
		}

	}

	// {
	// 	typedef safememory::unordered_multimap<int> HashMultisetInt;

	// 	HashMultisetInt hashMultiSet;

	// 	// insert_return_type insert(const value_type& value, hash_code_t c, node_type* pNodeNew = NULL);
	// 	HashMultisetInt::node_type* pNode = hashMultiSet.allocate_uninitialized_node();
	// 	HashMultisetInt::iterator it1 = hashMultiSet.insert(std::hash<int>()(999999), pNode, 999999);
	// 	EATEST_VERIFY(it1 != hashMultiSet.end());
	// 	pNode = hashMultiSet.allocate_uninitialized_node();
	// 	HashMultisetInt::iterator it2 = hashMultiSet.insert(std::hash<int>()(999999), pNode, 999999);
	// 	EATEST_VERIFY(it2 != hashMultiSet.end() && it2 != it1);
	// }


	return nErrorCount;
}

template<template<typename, typename> typename MAP, template<typename, typename, typename, typename> typename MAP2>
int TestHashMap()
{   
	int nErrorCount = 0;

	{  // Test declarations
		MAP<int, int>      hashMap;

		MAP<int, int> hashMap2(hashMap);
		EATEST_VERIFY(hashMap2.size() == hashMap.size());
		EATEST_VERIFY(hashMap2 == hashMap);

		// const char*     get_name() const;
		// void            set_name(const char* pName);
		// #if EASTL_NAME_ENABLED
		// 	hashMap.get_allocator().set_name("test");
		// 	const char* pName = hashMap.get_allocator().get_name();
		// 	EATEST_VERIFY(equal(pName, pName + 5, "test"));
		// #endif
	}

	{
		// void reserve(size_type nElementCount);
		nErrorCount += HashContainerReserveTest<MAP<int, int>>()();
	}

	{

		// C++11 emplace and related functionality
		nErrorCount += TestMapCpp11<MAP<int, TestObject>>();
		nErrorCount += TestMapCpp11NonCopyable<MAP<int, NonCopyable>>();
	}

	{
		// C++17 try_emplace and related functionality
		nErrorCount += TestMapCpp17<MAP<int, TestObject>>();
	}

	{
		// eastl::pair<iterator, iterator>             equal_range(const key_type& k);
		// eastl::pair<const_iterator, const_iterator> equal_range(const key_type& k) const;
		// const_iterator  erase(const_iterator, const_iterator);
		// size_type       erase(const key_type&);
		// To do.
	}

	{   // Test unordered_map

		// insert_return_type insert(const value_type& value);
		// insert_return_type insert(const key_type& key);
		// iterator       find(const key_type& k);
		// const_iterator find(const key_type& k) const;

		typedef MAP<int, int> HashMapIntInt;
		HashMapIntInt hashMap;
		const int kCount = 10000;

		for(int i = 0; i < kCount; i++)
		{
			typename HashMapIntInt::value_type vt(i, i);
			hashMap.insert(vt);
		}

		const HashMapIntInt const_hashMap = hashMap; // creating a const version to test for const correctness

		for(auto& e : hashMap)
		{
			int k = e.first;
			int v = e.second;
			EATEST_VERIFY(k < kCount);
			EATEST_VERIFY(v == k);
			EATEST_VERIFY(hashMap.at(k) == k);
			EATEST_VERIFY(const_hashMap.at(k) == k);
			hashMap.at(k) = k << 4;
		}

		for(auto& e : hashMap)
		{
			int k = e.first;
			int v = e.second;
			EATEST_VERIFY(k < kCount);
			EATEST_VERIFY(v == (k << 4));
		}

		for(int i = 0; i < kCount * 2; i++)
		{
			typename HashMapIntInt::iterator it = hashMap.find(i);

			if(i < kCount)
			{
				EATEST_VERIFY(it != hashMap.end());

				int k = (*it).first;
				int v = (*it).second;
				EATEST_VERIFY(v == (k << 4));
			}
			else
				EATEST_VERIFY(it == hashMap.end());
		}

		for(int i = 0; i < kCount; i++)
		{
			int v = hashMap.at(i);
			EATEST_VERIFY(v == (i << 4));
		}

		#if EASTL_EXCEPTIONS_ENABLED
			try
			{
				hashMap.at(kCount);
				EATEST_VERIFY(false);
			}
			catch(std::out_of_range&) { EATEST_VERIFY(true); }
			catch (nodecpp::error::memory_error&) { EATEST_VERIFY(true); }
			catch(...) { EATEST_VERIFY(false); }
		#endif
		// typename HashMapIntInt::insert_return_type result = hashMap.insert(88888);
		// EATEST_VERIFY(result.second == true);
		// result = hashMap.insert(88888);
		// EATEST_VERIFY(result.second == false);
		// result.first->second = 0;

		// const_iterator erase(const_iterator);
		size_t nExpectedSize = hashMap.size();

		typename HashMapIntInt::iterator it50 = hashMap.find(50);
		EATEST_VERIFY(it50 != hashMap.end());

		typename HashMapIntInt::iterator itNext = hashMap.erase(it50);
		nExpectedSize--;
		EATEST_VERIFY(itNext != hashMap.end()); // Strictly speaking, this isn't guaranteed to be so. But statistically it is very likely. We'll fix this if it becomes a problem.
		EATEST_VERIFY(hashMap.size() == nExpectedSize);

		typename HashMapIntInt::size_type n = hashMap.erase(10);
		nExpectedSize--;
		EATEST_VERIFY(n == 1);
		EATEST_VERIFY(hashMap.size() == nExpectedSize);

		typename HashMapIntInt::iterator it60 = hashMap.find(60);
		EATEST_VERIFY(itNext != hashMap.end());

		typename HashMapIntInt::iterator it60Incremented(it60);
		for(int i = 0; (i < 5) && (it60Incremented != hashMap.end()); ++i)
		{
			++it60Incremented;
			--nExpectedSize;
		}

		hashMap.erase(it60, it60Incremented);
		EATEST_VERIFY(hashMap.size() == nExpectedSize);


		// insert_return_type insert(const value_type& value, hash_code_t c, node_type* pNodeNew = NULL);
		// typename HashMapIntInt::node_type* pNode = hashMap.allocate_uninitialized_node();
		// typename HashMapIntInt::insert_return_type r = hashMap.insert(std::hash<int>()(999999), pNode, typename HashMapIntInt::value_type(999999, 999999));
		// EATEST_VERIFY(r.second == true);
		// pNode = hashMap.allocate_uninitialized_node();
		// r = hashMap.insert(std::hash<int>()(999999), pNode, typename HashMapIntInt::value_type(999999, 999999));
		// EATEST_VERIFY(r.second == false);
		// hashMap.free_uninitialized_node(pNode);
		// hashMap.erase(999999);


		// mapped_type& operator[](const key_type& key)
		// unordered_map is unique among the map/set containers in having this function.
		hashMap.clear();

		int x = hashMap[0]; // A default-constructed int (i.e. 0) should be returned.
		EATEST_VERIFY(x == 0); 
   
		hashMap[1] = 1;
		x = hashMap[1];
		EATEST_VERIFY(x == 1);     // Verify that the value we assigned is returned and a default-constructed value is not returned.
	 
		hashMap[0] = 10;    // Overwrite our previous 0 with 10.
		hashMap[1] = 11;
		x = hashMap[0];
		EATEST_VERIFY(x == 10);    // Verify the value is as expected.
		x = hashMap[1];
		EATEST_VERIFY(x == 11);
	}


	// {   // Test unordered_map

	// 	// Aligned objects should be CustomAllocator instead of the default, because the 
	// 	// EASTL default might be unable to do aligned allocations, but CustomAllocator always can.
	// 	unordered_map<Align32, int, std::hash<Align32>, std::equal_to<Align32>, CustomAllocator> hashMap;
	// 	const int kCount = 10000;

	// 	for(int i = 0; i < kCount; i++)
	// 	{
	// 		Align32 a32(i); // GCC 2.x doesn't like the Align32 object being created in the ctor below.
	// 		unordered_map<Align32, int>::value_type vt(a32, i);
	// 		hashMap.insert(vt);
	// 	}

	// 	for(unordered_map<Align32, int>::iterator it = hashMap.begin(); it != hashMap.end(); ++it)
	// 	{
	// 		const Align32& k = (*it).first;
	// 		int            v = (*it).second;
	// 		EATEST_VERIFY(k.mX < 10000);
	// 		EATEST_VERIFY(v == k.mX);
	// 	}

	// 	for(int i = 0; i < kCount * 2; i++)
	// 	{
	// 		unordered_map<Align32, int>::iterator it = hashMap.find(Align32(i));

	// 		if(i < kCount)
	// 		{
	// 			EATEST_VERIFY(it != hashMap.end());

	// 			const Align32& k = (*it).first;
	// 			int            v = (*it).second;
	// 			EATEST_VERIFY(v == k.mX);
	// 		}
	// 		else
	// 			EATEST_VERIFY(it == hashMap.end());
	// 	}
	// }



	{
		// global operator ==, !=
		EASTLTest_Rand rng(GetRandSeed());
		const std::size_t kIterationCount = 100;
		const std::size_t kDataRange = 50;

		{
			// For simplicity we duplicate the HashtableValue::mData member as the hash map key.
			typedef MAP2<std::size_t, HashtableValue, HashtableValueHash, HashtableValuePredicate> HashMap;
			HashtableValue value;

			HashMap h1;
			HashMap h2;
			EATEST_VERIFY(h1 == h2);

			for(std::size_t i = 0; i < kIterationCount; i++)
			{
				value.mData = rng.RandLimit(kDataRange);
				h1.insert(typename HashMap::value_type(value.mData, value));  // Leave value.mExtra as 0.
			}

			EATEST_VERIFY(h1 != h2);
			h2 = h1;
			EATEST_VERIFY(h1 == h2);

			// Test the case of the containers being the same size but having a single different value, despite that it's key compare yields equal.
			HashMap h2Saved(h2);
			typename HashMap::iterator it = h2.find(value.mData); // We are using value.mData as the key as well, so we can do a find via it.
			HashtableValue valueModified(value.mData, 1);
			h2.erase(it);
			h2.insert(typename HashMap::value_type(valueModified.mData, valueModified));
			EATEST_VERIFY(h1 != h2);
			h2 = h2Saved;

			// Test the case of the containers being the same size but having a single different key.
			h2Saved = h2;
			h2.erase(h2.find(value.mData));
			h2.insert(typename HashMap::value_type(kDataRange, HashtableValue(kDataRange))); // Insert something that could not have been in h2.
			EATEST_VERIFY(h1 != h2);
			h2 = h2Saved;

			h1.erase(h1.find(value.mData)); // Erase from h1 whatever the last value was.
			EATEST_VERIFY(h1 != h2);
		}

	}

	{ 
		// Regression of user-reported crash.
		MAP<int, std::string*>* _hmTextureList;
		_hmTextureList = new MAP<int, std::string*>();
		std::string* a = NULL;
		(*_hmTextureList)[0] = a;
		delete _hmTextureList;
	}

	{
		// Regression of user-reported GCC 4.8 compile failure.
		typedef MAP<int64_t, Struct> AuditByBlazeIdMap;

		AuditByBlazeIdMap auditBlazeIds;
		AuditByBlazeIdMap tempAuditBlazeIds;

		auditBlazeIds.swap(tempAuditBlazeIds); // This line was generating an unexpected compiler failure.
		EATEST_VERIFY(auditBlazeIds.empty() && tempAuditBlazeIds.empty());
	}

	// {
	// 	// This test is designed to designed to use the find_range_by_hash method to walk over all keys in a hash bucket (located by a hash value).
		
	// 	// Use the 'colliding_hash' hash function to intentionally create lots of collisions in a predictable way.
	// 	typedef unordered_map<int, int, colliding_hash> HM;
	// 	HM hashMap;

	// 	// Add some numbers to the hashMap.
	// 	for(int i=0; i<90; i++)
	// 	{
	// 		hashMap[i] = i;
	// 	}

	// 	// Try to find a hash value that doesn't exist
	// 	{
	// 		std::pair<HM::iterator, HM::iterator> i = hashMap.find_range_by_hash(1000);
	// 		EATEST_VERIFY(i.first == hashMap.end());
	// 		EATEST_VERIFY(i.second == hashMap.end());
	// 	}

	// 	{
	// 		int iterations = 0;
	// 		for(std::pair<HM::iterator, HM::iterator> i = hashMap.find_range_by_hash(1); i.first != i.second; i.first++)
	// 		{
	// 			int nodeValue = i.first.get_node()->mValue.first;
	// 			EATEST_VERIFY(nodeValue % 3 == 1);   // Verify the hash of the node matches the expected value
	// 			iterations++;
	// 		}
	// 		EATEST_VERIFY(iterations == 30);
	// 	}

	// 	{
	// 		const HM &constHashMap = hashMap;
	// 		int iterations = 0;
	// 		for(std::pair<HM::const_iterator, HM::const_iterator> i = constHashMap.find_range_by_hash(1); i.first != i.second; i.first++)
	// 		{
	// 			int nodeValue = i.first.get_node()->mValue.first;
	// 			EATEST_VERIFY(nodeValue % 3 == 1);   // Verify the hash of the node matches the expected value
	// 			iterations++;
	// 		}
	// 		EATEST_VERIFY(iterations == 30);
	// 	}
	// }


	{
		// initializer_list support.
		// unordered_map(std::initializer_list<value_type> ilist, size_type nBucketCount = 0, const Hash& hashFunction = Hash(), 
		//            const Predicate& predicate = Predicate(), const allocator_type& allocator = EASTL_unordered_map_DEFAULT_ALLOCATOR)
		// this_type& operator=(std::initializer_list<value_type> ilist);
		// void insert(std::initializer_list<value_type> ilist);
		
		// VS2013 has a known issue when dealing with std::initializer_lists
		// https://connect.microsoft.com/VisualStudio/feedback/details/792355/compiler-confused-about-whether-to-use-a-initializer-list-assignment-operator
		#if !defined(EA_COMPILER_NO_INITIALIZER_LISTS) && !(defined(_MSC_VER) && _MSC_VER == 1800)
			MAP<int, double> intHashMap = { {12,12.0}, {13,13.0}, {14,14.0} };
			EATEST_VERIFY(intHashMap.size() == 3);
			EATEST_VERIFY(intHashMap.find(12) != intHashMap.end());
			EATEST_VERIFY(intHashMap.find(13) != intHashMap.end());
			EATEST_VERIFY(intHashMap.find(14) != intHashMap.end());

			intHashMap = { {22,22.0}, {23,23.0}, {24,24.0} };
			EATEST_VERIFY(intHashMap.size() == 3);
			EATEST_VERIFY(intHashMap.find(22) != intHashMap.end());
			EATEST_VERIFY(intHashMap.find(23) != intHashMap.end());
			EATEST_VERIFY(intHashMap.find(24) != intHashMap.end());

			intHashMap.insert({ {42,42.0}, {43,43.0}, {44,44.0} });
			EATEST_VERIFY(intHashMap.size() == 6);
			EATEST_VERIFY(intHashMap.find(42) != intHashMap.end());
			EATEST_VERIFY(intHashMap.find(43) != intHashMap.end());
			EATEST_VERIFY(intHashMap.find(44) != intHashMap.end());
		#endif
	}

	// Can't use move semantics with unordered_map::operator[]
	//
	// GCC has a bug with overloading rvalue and lvalue function templates.
	// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=54425
	// 
	// error: 'eastl::pair<T1, T2>::pair(T1&&) [with T1 = const int&; T2 = const int&]' cannot be overloaded
	// error: with 'eastl::pair<T1, T2>::pair(const T1&) [with T1 = const int&; T2 = const int&]'
	#if !defined(EA_COMPILER_GNUC)
	{
		EA_DISABLE_VC_WARNING(4626)
		struct Key
		{
			Key() {}
			Key(Key&&) {}
			Key(const Key&&) {}
			bool operator==(const Key&) const { return true; }

		private:
			Key(const Key&) {}
		};
		EA_RESTORE_VC_WARNING()

		struct Hash
		{
			std::size_t operator()(const Key&) const { return 0; }
		};

		Key key1, key2;
		MAP2<Key, int, Hash, std::equal_to<Key>> hm;
		hm[std::move(key1)] = 12345;

		EATEST_VERIFY(hm[std::move(key2)] == 12345);
	}
	#endif

	// {
	// 	using AllocatorType = CountingAllocator;
	// 	using String = std::basic_string<char8_t, AllocatorType>;
	// 	using StringStringMap = std::map<String, String, std::equal_to<String>, AllocatorType>;
	// 	using StringStringHashMap = safememory::unordered_map<String, String, eastl::string_hash<String>, eastl::equal_to<String>, AllocatorType>;
	// 	AllocatorType::resetCount();

	// 	{
	// 		StringStringHashMap myMap(5); // construct map with 5 buckets, so we don't rehash on insert
	// 		String key("mykey01234567890000000000000000000000000000");
	// 		String value("myvalue01234567890000000000000000000000000000");
	// 		AllocatorType::resetCount();

	// 		myMap.insert(eastl::make_pair(eastl::move(key), eastl::move(value)));
	// 		EATEST_VERIFY(AllocatorType::getTotalAllocationCount() == 1);
	// 	}
	// 	{
	// 		StringStringHashMap myMap(5); // construct map with 5 buckets, so we don't rehash on insert
	// 		String key("mykey01234567890000000000000000000000000000");
	// 		String value("myvalue01234567890000000000000000000000000000");
	// 		AllocatorType::resetCount();

	// 		myMap.emplace(eastl::move(key), eastl::move(value));
	// 		EATEST_VERIFY(AllocatorType::getTotalAllocationCount() == 1);
	// 	}
	// 	{
	// 		StringStringMap myMap;
	// 		String key("mykey01234567890000000000000000000000000000");
	// 		String value("myvalue01234567890000000000000000000000000000");
	// 		AllocatorType::resetCount();

	// 		myMap.insert(eastl::make_pair(eastl::move(key), eastl::move(value)));
	// 		EATEST_VERIFY(AllocatorType::getTotalAllocationCount() == 1);
	// 	}
	// 	{
	// 		StringStringMap myMap;
	// 		String key("mykey01234567890000000000000000000000000000");
	// 		String value("myvalue01234567890000000000000000000000000000");
	// 		AllocatorType::resetCount();

	// 		myMap.emplace(eastl::move(key), eastl::move(value));
	// 		EATEST_VERIFY(AllocatorType::getTotalAllocationCount() == 1);
	// 	}
	// }

	
	{

		struct name_equals
		{
			bool operator()(const std::pair<int, const char*>& a, const std::pair<int, const char*>& b) const
			{
				if (a.first != b.first)
					return false;

				return strcmp(a.second, b.second) == 0;
			}
		};

		{
			int n = 42;
			const char* pCStrName = "electronic arts";
			MAP2<std::pair<int, const char*>, bool, std::hash<std::pair<int, const char*>>, name_equals> m_TempNames;
			m_TempNames[std::make_pair(n, pCStrName)] = true;

			auto isFound = (m_TempNames.find(std::make_pair(n, pCStrName)) != m_TempNames.end());
			VERIFY(isFound);
		}
	}

	return nErrorCount;
}

template<template<typename, typename> typename MMAP, template<typename, typename, typename, typename> typename MMAP2>
int TestHashMultiMap()
{   
	int nErrorCount = 0;

	{  // Test declarations
		MMAP<int, int> hashMultiMap;

		MMAP<int, int> hashMultiMap2(hashMultiMap);
		EATEST_VERIFY(hashMultiMap2.size() == hashMultiMap.size());
		EATEST_VERIFY(hashMultiMap2 == hashMultiMap);
	}

	{
		// C++11 emplace and related functionality
		// nErrorCount += TestMultimapCpp11<MMAP<int, TestObject>>();
	}


	{
		// global operator ==, !=
		EASTLTest_Rand rng(GetRandSeed());
		const std::size_t kIterationCount = 100;
		const std::size_t kDataRange = 50;

		{
			// For simplicity we duplicate the HashtableValue::mData member as the hash map key.
			typedef MMAP2<std::size_t, HashtableValue, HashtableValueHash, HashtableValuePredicate> HashMap;
			HashtableValue value;

			HashMap h1;
			HashMap h2;
			EATEST_VERIFY(h1 == h2);

			for(std::size_t i = 0; i < kIterationCount; i++)
			{
				value.mData = rng.RandLimit(kDataRange);
				h1.insert(typename HashMap::value_type(value.mData, value));  // Leave value.mExtra as 0.
			}

			EATEST_VERIFY(h1 != h2);
			h2 = h1;
			EATEST_VERIFY(h1 == h2);

			// Test the case of the containers being the same size but having a single different value, despite that it's key compare yields equal.
			HashMap h2Saved(h2);
			typename HashMap::iterator it = h2.find(value.mData); // We are using value.mData as the key as well, so we can do a find via it.
			HashtableValue valueModified(value.mData, 1);
			h2.erase(it);
			h2.insert(typename HashMap::value_type(valueModified.mData, valueModified));
			EATEST_VERIFY(h1 != h2);
			h2 = h2Saved;

			// Test the case of the containers being the same size but having a single different key.
			h2Saved = h2;
			h2.erase(h2.find(value.mData));
			h2.insert(typename HashMap::value_type(kDataRange, HashtableValue(kDataRange))); // Insert something that could not have been in h2.
			EATEST_VERIFY(h1 != h2);
			h2 = h2Saved;

			h1.erase(h1.find(value.mData)); // Erase from h1 whatever the last value was.
			EATEST_VERIFY(h1 != h2);
		}
	}

	// {
	// 	typedef safememory::unordered_multimap<int> HashMultisetInt;

	// 	HashMultisetInt hashMultiSet;

	// 	// insert_return_type insert(const value_type& value, hash_code_t c, node_type* pNodeNew = NULL);
	// 	HashMultisetInt::node_type* pNode = hashMultiSet.allocate_uninitialized_node();
	// 	HashMultisetInt::iterator it1 = hashMultiSet.insert(std::hash<int>()(999999), pNode, 999999);
	// 	EATEST_VERIFY(it1 != hashMultiSet.end());
	// 	pNode = hashMultiSet.allocate_uninitialized_node();
	// 	HashMultisetInt::iterator it2 = hashMultiSet.insert(std::hash<int>()(999999), pNode, 999999);
	// 	EATEST_VERIFY(it2 != hashMultiSet.end() && it2 != it1);
	// }

	{ 
		// Regression of compiler warning reported by Jeff Litz/Godfather regarding 
		// strict aliasing (EASTL 1.09.01) December 2007).
		typedef MMAP<uint32_t, uint32_t*> Map;
		Map* pMap = new Map;
		delete pMap;
	}

	{
		// Regression of user-reported Android compiler error.
		typedef MMAP<HashRegressionA*, HashRegressionB> HMM;
		HMM m_hash;

		// Section 1
		for (typename HMM::iterator it = m_hash.begin(); it != m_hash.end(); it++)
			it->second.y = 1;

		// Section 2
		HashRegressionA* pA = NULL;
		eastl::pair<typename HMM::iterator, typename HMM::iterator> pair = m_hash.equal_range(pA);
		(void)pair;
	}

	return nErrorCount;
}

// template <typename Key>
// using SET = safememory::unordered_set<Key>;

// template <typename Key, typename Hash, typename Predicate>
// using SET3 = safememory::unordered_set<Key, Hash, Predicate>;

// template <typename Key>
// using MSET = safememory::unordered_multiset<Key>;

// template <typename Key, typename Hash, typename Predicate>
// using MSET3 = safememory::unordered_multiset<Key, Hash, Predicate>;


template <typename Key, typename T>
using MAP = safememory::unordered_map<Key, T>;

template <typename Key, typename T, typename Hash, typename Predicate>
using MAP4 = safememory::unordered_map<Key, T, Hash, Predicate>;

template <typename Key, typename T>
using MAP_SAFE = safememory::unordered_map_safe<Key, T>;

template <typename Key, typename T, typename Hash, typename Predicate>
using MAP_SAFE4 = safememory::unordered_map_safe<Key, T, Hash, Predicate>;


template <typename Key, typename T>
using MMAP = safememory::unordered_multimap<Key, T>;

template <typename Key, typename T, typename Hash, typename Predicate>
using MMAP4 = safememory::unordered_multimap<Key, T, Hash, Predicate>;


int TestHash()
{
	int nErrorCount = 0;

	// nErrorCount += TestHashSet<SET, SET3>();

	// nErrorCount += TestHashMultiSet<MSET, MSET3>();

	nErrorCount += TestHashMap<MAP, MAP4>();
	nErrorCount += TestHashMap<MAP_SAFE, MAP_SAFE4>();

	nErrorCount += TestHashMultiMap<MMAP, MMAP4>();

	return nErrorCount;
}







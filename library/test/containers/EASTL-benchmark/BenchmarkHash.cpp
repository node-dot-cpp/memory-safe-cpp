/////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////


#include "EASTLBenchmark.h"
#include "EASTLTest.h"
#include "EAStopwatch.h"
// #include <EASTL/vector.h>
#include <safememory/unordered_map.h>
#include <EASTL/unordered_map.h>
#include <vector>
#include <string>
#include <algorithm>



EA_DISABLE_ALL_VC_WARNINGS()
#include <unordered_map>
#include <string>
#include <algorithm>
#include <stdio.h>
EA_RESTORE_ALL_VC_WARNINGS()



using namespace EA;


// HashString8
//
// We define a string
//
template <typename String>
struct HashString8
{
	// Defined for EASTL, STLPort, SGI, etc. and Metrowerks-related hash tables:
	size_t operator()(const String& s) const 
	{ 
		const uint8_t* p = (const uint8_t*) s.c_str();
		uint32_t c, stringHash = UINT32_C(2166136261);
		while((c = *p++) != 0)
			stringHash = (stringHash * 16777619) ^ c;
		return stringHash;
	}

	// Defined for Dinkumware-related (e.g. MS STL) hash tables:
	bool operator()(const String& s1, const String& s2) const
	{
		return s1 < s2;
	}

	// Defined for Dinkumware-related (e.g. MS STL) hash tables:
	enum {
		bucket_size = 7,
		min_buckets = 8
	};
};


namespace
{
	template <typename Container, typename Value>
	void TestInsert(EA::StdC::Stopwatch& stopwatch, Container& c, const Value* pArrayBegin, const Value* pArrayEnd)
	{
		stopwatch.Restart();
		c.insert(pArrayBegin, pArrayEnd);
		stopwatch.Stop();
	}

	template <typename ValueType, typename Container, typename Container2>
	void TestInsertEA(EA::StdC::Stopwatch& stopwatch, Container& c, const Container2& c2)
	{
		stopwatch.Restart();
		for(auto& Each : c2)
			c.insert(ValueType(Each.first, Each.second));
		stopwatch.Stop();
	}

	template <typename Container, typename Value>
	void TestIteration(EA::StdC::Stopwatch& stopwatch, const Container& c, const Value& findValue)
	{
		stopwatch.Restart();
		//typename Container::const_iterator it = std::find(c.begin(), c.end(), findValue);
		typename Container::const_iterator it = c.begin();
		typename Container::const_iterator last = c.end();
		for (; it != last; ++it) {
			if (*it == findValue) {
				break;
			}
		}

		stopwatch.Stop();
		if(it != last)
			sprintf(Benchmark::gScratchBuffer, "%p", &*it);
	}


	template <typename Container, typename Value>
	void TestBracket(EA::StdC::Stopwatch& stopwatch, Container& c, const Value* pArrayBegin, const Value* pArrayEnd)
	{
		stopwatch.Restart();
		while(pArrayBegin != pArrayEnd)
		{
			Benchmark::DoNothing(&c[pArrayBegin->first]);
			++pArrayBegin;
		}
		stopwatch.Stop();
	}


	template <typename Container, typename Value>
	void TestFind(EA::StdC::Stopwatch& stopwatch, Container& c, const Value* pArrayBegin, const Value* pArrayEnd)
	{
		stopwatch.Restart();
		while(pArrayBegin != pArrayEnd)
		{
			typename Container::iterator it = c.find(pArrayBegin->first);
			Benchmark::DoNothing(&it);
			++pArrayBegin;
		}
		stopwatch.Stop();
	}


	// template <typename Container, typename Value>
	// void TestFindAsStd(EA::StdC::Stopwatch& stopwatch, Container& c, const Value* pArrayBegin, const Value* pArrayEnd)
	// {
	// 	stopwatch.Restart();
	// 	while(pArrayBegin != pArrayEnd)
	// 	{
	// 		typename Container::iterator it = c.find(pArrayBegin->first.c_str());
	// 		Benchmark::DoNothing(&it);
	// 		++pArrayBegin;
	// 	}
	// 	stopwatch.Stop();
	// }


	// template <typename Container, typename Value>
	// void TestFindAsEa(EA::StdC::Stopwatch& stopwatch, Container& c, const Value* pArrayBegin, const Value* pArrayEnd)
	// {
	// 	stopwatch.Restart();
	// 	while(pArrayBegin != pArrayEnd)
	// 	{
	// 		typename Container::iterator it = c.find_as(pArrayBegin->first.c_str());
	// 		Benchmark::DoNothing(&it);
	// 		++pArrayBegin;
	// 	}
	// 	stopwatch.Stop();
	// }


	template <typename Container, typename Value>
	void TestCount(EA::StdC::Stopwatch& stopwatch, Container& c, const Value* pArrayBegin, const Value* pArrayEnd)
	{
		typename Container::size_type temp = 0;
		stopwatch.Restart();
		while(pArrayBegin != pArrayEnd)
		{
			temp += c.count(pArrayBegin->first);
			++pArrayBegin;
		}
		stopwatch.Stop();
		sprintf(Benchmark::gScratchBuffer, "%u", (unsigned)temp);
	}


	template <typename Container, typename Value>
	void TestEraseValue(EA::StdC::Stopwatch& stopwatch, Container& c, const Value* pArrayBegin, const Value* pArrayEnd)
	{
		stopwatch.Restart();
		while(pArrayBegin != pArrayEnd)
		{
			c.erase(pArrayBegin->first);
			++pArrayBegin;
		}
		stopwatch.Stop();
		sprintf(Benchmark::gScratchBuffer, "%u", (unsigned)c.size());
	}


	template <typename Container>
	void TestErasePosition(EA::StdC::Stopwatch& stopwatch, Container& c)
	{
		// typename Container::size_type j, jEnd;
		// typename Container::iterator it;

		stopwatch.Restart();
		typename Container::size_type j = 0;
		typename Container::size_type jEnd = c.size() / 3;
		typename Container::iterator it = c.begin();
		for(; j < jEnd; ++j)
		{
			// The erase fucntion is supposed to return an iterator, but the C++ standard was 
			// not initially clear about it and some STL implementations don't do it correctly.
			// #if (defined(_MSC_VER) || defined(_CPPLIB_VER)) // _CPPLIB_VER is something defined by Dinkumware STL.
				it = c.erase(it);
			// #else
			// 	// This pathway may execute at a slightly different speed than the 
			// 	// standard behaviour, but that's fine for the benchmark because the
			// 	// benchmark is measuring the speed of erasing while iterating, and 
			// 	// however it needs to get done by the given STL is how it is measured.
			// 	const typename Container::iterator itErase(it++);
			// 	c.erase(itErase);
			// #endif

			++it;
			++it;
		}

		stopwatch.Stop();
		sprintf(Benchmark::gScratchBuffer, "%p %p", &c, &it);
	}


	template <typename Container>
	void TestEraseRange(EA::StdC::Stopwatch& stopwatch, Container& c)
	{
		typename Container::size_type j, jEnd;
		typename Container::iterator  it1 = c.begin();
		typename Container::iterator  it2 = c.begin();

		for(j = 0, jEnd = c.size() / 3; j < jEnd; ++j)
			++it2;

		stopwatch.Restart();
		c.erase(it1, it2);
		stopwatch.Stop();
		sprintf(Benchmark::gScratchBuffer, "%p %p %p", &c, &it1, &it2);
	}


	template <typename Container>
	void TestClear(EA::StdC::Stopwatch& stopwatch, Container& c)
	{
		stopwatch.Restart();
		c.clear();
		stopwatch.Stop();
		sprintf(Benchmark::gScratchBuffer, "%u", (unsigned)c.size());
	}


} // namespace


template<int IX, template<typename, typename> typename Map1, template<typename, typename, typename> typename Map2>
void BenchmarkHashTempl()
{
	EASTLTest_Rand  rng(GetRandSeed());
	EA::StdC::Stopwatch stopwatch1(EA::StdC::Stopwatch::kUnitsCPUCycles);
	std::vector<   std::pair<uint32_t, TestObject> > stdVectorUT(10000);
	std::vector<   std::pair<  std::string, uint32_t> > stdVectorSU(10000);

	for(std::size_t i = 0, iEnd = stdVectorUT.size(); i < iEnd; i++)
	{
		const uint32_t n1 = rng.RandLimit((uint32_t)(iEnd / 2));
		const uint32_t n2 = rng.Rand();

		stdVectorUT[i] =   std::pair<uint32_t, TestObject>(n1, TestObject(n2));

		char str_n1[32];
		sprintf(str_n1, "%u", (unsigned)n1);

		stdVectorSU[i] =   std::pair<  std::string, uint32_t>(  std::string(str_n1), n2);
	}

	for(int i = 0; i < 2; i++)
	{
		Map1<uint32_t, TestObject> stdMapUint32TO;
		Map2<std::string, uint32_t, HashString8<std::string>> stdMapStrUint32;

		typedef typename Map1<uint32_t, TestObject>::value_type Vt1;
		typedef typename Map2<std::string, uint32_t, HashString8<std::string>>::value_type Vt2;

		///////////////////////////////
		// Test insert(const value_type&)
		///////////////////////////////

		TestInsertEA<Vt1>(stopwatch1, stdMapUint32TO, stdVectorUT);

		if(i == 1)
			Benchmark::AddResult("unordered_map<uint32_t, TestObject>/insert", IX, stopwatch1);

		TestInsertEA<Vt2>(stopwatch1, stdMapStrUint32, stdVectorSU);

		if(i == 1)
			Benchmark::AddResult("unordered_map<string, uint32_t>/insert", IX, stopwatch1);


		///////////////////////////////
		// Test iteration
		///////////////////////////////

		TestIteration(stopwatch1, stdMapUint32TO, Vt1(9999999, TestObject(9999999)));

		if(i == 1)
			Benchmark::AddResult("unordered_map<uint32_t, TestObject>/iteration", IX, stopwatch1);

		TestIteration(stopwatch1, stdMapStrUint32, Vt2(  std::string("9999999"), 9999999));

		if(i == 1)
			Benchmark::AddResult("unordered_map<string, uint32_t>/iteration", IX, stopwatch1);


		///////////////////////////////
		// Test operator[]
		///////////////////////////////

		TestBracket(stopwatch1, stdMapUint32TO, stdVectorUT.data(), stdVectorUT.data() + stdVectorUT.size());

		if(i == 1)
			Benchmark::AddResult("unordered_map<uint32_t, TestObject>/operator[]", IX, stopwatch1);

		TestBracket(stopwatch1, stdMapStrUint32, stdVectorSU.data(), stdVectorSU.data() + stdVectorSU.size());

		if(i == 1)
			Benchmark::AddResult("unordered_map<string, uint32_t>/operator[]", IX, stopwatch1);


		///////////////////////////////
		// Test find
		///////////////////////////////

		TestFind(stopwatch1, stdMapUint32TO, stdVectorUT.data(), stdVectorUT.data() + stdVectorUT.size());

		if(i == 1)
			Benchmark::AddResult("unordered_map<uint32_t, TestObject>/find", IX, stopwatch1);

		TestFind(stopwatch1, stdMapStrUint32, stdVectorSU.data(), stdVectorSU.data() + stdVectorSU.size());

		if(i == 1)
			Benchmark::AddResult("unordered_map<string, uint32_t>/find", IX, stopwatch1);


		///////////////////////////////
		// Test find_as
		///////////////////////////////

		// TestFindAsStd(stopwatch1, stdMapStrUint32, stdVectorSU.data(), stdVectorSU.data() + stdVectorSU.size());
		// TestFindAsEa(stopwatch2, eaMapStrUint32,    eaVectorSU.data(),  eaVectorSU.data() +  eaVectorSU.size());

		// if(i == 1)
		// 	Benchmark::AddResult("unordered_map<string, uint32_t>/find_as/char*", IX, stopwatch1);


		///////////////////////////////
		// Test count
		///////////////////////////////

		TestCount(stopwatch1, stdMapUint32TO, stdVectorUT.data(), stdVectorUT.data() + stdVectorUT.size());

		if(i == 1)
			Benchmark::AddResult("unordered_map<uint32_t, TestObject>/count", IX, stopwatch1);

		TestCount(stopwatch1, stdMapStrUint32, stdVectorSU.data(), stdVectorSU.data() + stdVectorSU.size());

		if(i == 1)
			Benchmark::AddResult("unordered_map<string, uint32_t>/count", IX, stopwatch1);


		///////////////////////////////
		// Test erase(const key_type& key)
		///////////////////////////////

		TestEraseValue(stopwatch1, stdMapUint32TO, stdVectorUT.data(), stdVectorUT.data() + (stdVectorUT.size() / 2));

		if(i == 1)
			Benchmark::AddResult("unordered_map<uint32_t, TestObject>/erase val", IX, stopwatch1);

		TestEraseValue(stopwatch1, stdMapStrUint32, stdVectorSU.data(), stdVectorSU.data() + (stdVectorSU.size() / 2));

		if(i == 1)
			Benchmark::AddResult("unordered_map<string, uint32_t>/erase val", IX, stopwatch1);


		///////////////////////////////
		// Test erase(iterator position)
		///////////////////////////////

		TestErasePosition(stopwatch1, stdMapUint32TO);

		if(i == 1)
			Benchmark::AddResult("unordered_map<uint32_t, TestObject>/erase pos", IX, stopwatch1);

		TestErasePosition(stopwatch1, stdMapStrUint32);

		if(i == 1)
			Benchmark::AddResult("unordered_map<string, uint32_t>/erase pos", IX, stopwatch1);


		///////////////////////////////
		// Test erase(iterator first, iterator last)
		///////////////////////////////

		TestEraseRange(stopwatch1, stdMapUint32TO);

		if(i == 1)
			Benchmark::AddResult("unordered_map<uint32_t, TestObject>/erase range", IX, stopwatch1);

		TestEraseRange(stopwatch1, stdMapStrUint32);

		if(i == 1)
			Benchmark::AddResult("unordered_map<string, uint32_t>/erase range", IX, stopwatch1);


		///////////////////////////////
		// Test clear()
		///////////////////////////////

		// Clear the containers of whatever they happen to have. We want the containers to have full data.
		TestClear(stopwatch1, stdMapUint32TO);
		TestClear(stopwatch1, stdMapStrUint32);

		// Re-set the containers with full data.
		TestInsertEA<Vt1>(stopwatch1, stdMapUint32TO, stdVectorUT);
		TestInsertEA<Vt2>(stopwatch1, stdMapStrUint32, stdVectorSU);

		// Now clear the data again, this time measuring it.
		TestClear(stopwatch1, stdMapUint32TO);

		if(i == 1)
			Benchmark::AddResult("unordered_map<uint32_t, TestObject>/clear", IX, stopwatch1);

		TestClear(stopwatch1, stdMapStrUint32);

		if(i == 1)
			Benchmark::AddResult("unordered_map<string, uint32_t>/clear", IX, stopwatch1);

	}
}

template<class K, class V>
using StdMap1 = std::unordered_map<K, V>;

template<class K, class V, class H>
using StdMap2 = std::unordered_map<K, V, H>;

template<class K, class V>
using EaMap1 = eastl::unordered_map<K, V>;

template<class K, class V, class H>
using EaMap2 = eastl::unordered_map<K, V, H>;

template<class K, class V>
using UnsafeMap1 = safememory::unordered_map<K, V, eastl::hash<K>, eastl::equal_to<K>, safememory::memory_safety::none>;

template<class K, class V, class H>
using UnsafeMap2 = safememory::unordered_map<K, V, H, eastl::equal_to<K>, safememory::memory_safety::none>;

template<class K, class V>
using SafeMap1 = safememory::unordered_map<K, V, eastl::hash<K>, eastl::equal_to<K>, safememory::memory_safety::safe>;

template<class K, class V, class H>
using SafeMap2 = safememory::unordered_map<K, V, H, eastl::equal_to<K>, safememory::memory_safety::safe>;

template<class K, class V>
using ReallySafeMap1 = safememory::unordered_map_safe<K, V, eastl::hash<K>, eastl::equal_to<K>, safememory::memory_safety::safe>;

template<class K, class V, class H>
using ReallySafeMap2 = safememory::unordered_map_safe<K, V, H, eastl::equal_to<K>, safememory::memory_safety::safe>;

void BenchmarkHash()
{
	EASTLTest_Printf("HashMap\n");

	// BenchmarkHashTempl<1, StdMap1, StdMap2>();
	BenchmarkHashTempl<1, EaMap1, EaMap2>();
	BenchmarkHashTempl<2, UnsafeMap1, UnsafeMap2>();
	BenchmarkHashTempl<3, SafeMap1, SafeMap2>();
	BenchmarkHashTempl<4, ReallySafeMap1, ReallySafeMap2>();
}


















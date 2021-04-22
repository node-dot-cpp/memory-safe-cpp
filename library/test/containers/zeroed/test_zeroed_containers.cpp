/* -------------------------------------------------------------------------------
* Copyright (c) 2018, OLogN Technologies AG
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

// TestSafePointers.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include "../../../3rdparty/lest/include/lest/lest.hpp"
#include <safememory/vector.h>
#include <safememory/string.h>
#include <safememory/unordered_map.h>
#include <safememory/safe_ptr.h>
#include <iibmalloc.h>
#include <nodecpp_assert.h>


int testWithLest( int argc, char * argv[] )
{
	const lest::test specification[] =
	{

//////////////////////////////////
		CASE( "vector, zeroed" )
		{
			// we need to start with zeroed memory
			typedef safememory::vector<int> V;
			char buff[sizeof(V) * 2] = {'\0'};

			V* vp1 = reinterpret_cast<V*>(&buff[0]);
			V* vp2 = reinterpret_cast<V*>(&buff[sizeof(V)]);

			new(vp1) V{};
			new(vp2) V{};
			 
			memset(vp2, 0, sizeof(V));
			EXPECT( memcmp(vp1, vp2, sizeof(V)) == 0);
		},
		CASE( "vector, dtor" )
		{
			// we need to start with zeroed memory
			typedef safememory::vector<int> V;
			char buff[sizeof(V) * 2] = {'\0'};

			V* vp1 = reinterpret_cast<V*>(&buff[0]);
			V* vp2 = reinterpret_cast<V*>(&buff[sizeof(V)]);

			new(vp1) V{};
			new(vp2) V{};
			 
			vp2->~V();
			EXPECT( memcmp(vp1, vp2, sizeof(V)) == 0);
		},

		CASE( "string, zeroed" )
		{
			// zeroed string bit to bit identical to 15 '\0' chars

			// we need to start with zeroed memory
			typedef safememory::string V;
			char buff[sizeof(V) * 2] = {'\0'};

			V* vp1 = reinterpret_cast<V*>(&buff[0]);
			V* vp2 = reinterpret_cast<V*>(&buff[sizeof(V)]);

			new(vp1) V(15, '\0');
			new(vp2) V{};
			 
			memset(vp2, 0, sizeof(V));
			EXPECT( memcmp(vp1, vp2, sizeof(V)) == 0);
		},
		CASE( "string, dtor" )
		{
			// we need to start with zeroed memory
			typedef safememory::string V;
			char buff[sizeof(V) * 2] = {'\0'};

			V* vp1 = reinterpret_cast<V*>(&buff[0]);
			V* vp2 = reinterpret_cast<V*>(&buff[sizeof(V)]);

			new(vp1) V{};
			new(vp2) V{};
			 
			vp2->~V();
			EXPECT( memcmp(vp1, vp2, sizeof(V)) == 0);
		},
		CASE( "string_literal, zeroed" )
		{
			// zeroed string bit to bit identical to 15 '\0' chars

			// we need to start with zeroed memory
			typedef safememory::string_literal V;
			char buff[sizeof(V) * 2] = {'\0'};

			V* vp1 = reinterpret_cast<V*>(&buff[0]);
			V* vp2 = reinterpret_cast<V*>(&buff[sizeof(V)]);

			new(vp1) V{};
			new(vp2) V{};
			 
			memset(vp2, 0, sizeof(V));
			EXPECT( memcmp(vp1, vp2, sizeof(V)) == 0);
		},
		CASE( "string_literal, dtor" )
		{
			// we need to start with zeroed memory
			typedef safememory::string_literal V;
			char buff[sizeof(V) * 2] = {'\0'};

			V* vp1 = reinterpret_cast<V*>(&buff[0]);
			V* vp2 = reinterpret_cast<V*>(&buff[sizeof(V)]);

			new(vp1) V{};
			new(vp2) V{};
			 
			vp2->~V();
			EXPECT( memcmp(vp1, vp2, sizeof(V)) == 0);
		},
		CASE( "iterator, zeroed" )
		{
			// we need to start with zeroed memory
			typedef safememory::vector<int>::iterator V;
			char buff[sizeof(V) * 2] = {'\0'};

			V* vp1 = reinterpret_cast<V*>(&buff[0]);
			V* vp2 = reinterpret_cast<V*>(&buff[sizeof(V)]);

			new(vp1) V{};
			new(vp2) V{};
			 
			memset(vp2, 0, sizeof(V));
			EXPECT( memcmp(vp1, vp2, sizeof(V)) == 0);
		},
		CASE( "iterator, dtor" )
		{
			// we need to start with zeroed memory
			typedef safememory::vector<int>::iterator V;
			char buff[sizeof(V) * 2] = {'\0'};

			V* vp1 = reinterpret_cast<V*>(&buff[0]);
			V* vp2 = reinterpret_cast<V*>(&buff[sizeof(V)]);

			new(vp1) V{};
			new(vp2) V{};
			 
			vp2->~V();
			EXPECT( memcmp(vp1, vp2, sizeof(V)) == 0);
		},

		// CASE( "iterator_safe, zeroed" )
		// {
		// 	// we need to start with zeroed memory
		// 	typedef safememory::vector<int>::iterator_safe V;
		// 	char buff[sizeof(V) * 2] = {'\0'};

		// 	V* vp1 = reinterpret_cast<V*>(&buff[0]);
		// 	V* vp2 = reinterpret_cast<V*>(&buff[sizeof(V)]);

		// 	new(vp1) V{};
		// 	new(vp2) V{};
			 
		// 	memset(vp2, 0, sizeof(V));
		// 	EXPECT( memcmp(vp1, vp2, sizeof(V)) == 0);
		// },
		CASE( "iterator_safe, dtor" )
		{
			// we need to start with zeroed memory
			typedef safememory::vector<int>::iterator_safe V;
			char buff[sizeof(V) * 2] = {'\0'};

			V* vp1 = reinterpret_cast<V*>(&buff[0]);
			V* vp2 = reinterpret_cast<V*>(&buff[sizeof(V)]);

			new(vp1) V{};
			new(vp2) V{};
			 
			vp2->~V();
			EXPECT( memcmp(vp1, vp2, sizeof(V)) == 0);
		},

		CASE( "unordered_map, dtor" )
		{
			// we need to start with zeroed memory
			typedef safememory::unordered_map<int, int> V;
			char buff[sizeof(V) * 2] = {'\0'};

			V* vp1 = reinterpret_cast<V*>(&buff[0]);
			V* vp2 = reinterpret_cast<V*>(&buff[sizeof(V)]);

			new(vp1) V{};
			new(vp2) V{};
			 
			vp2->~V();
			EXPECT( memcmp(vp1, vp2, sizeof(V)) == 0);
		},

		CASE( "unordered_map::iterator, dtor" )
		{
			// we need to start with zeroed memory
			typedef safememory::unordered_map<int, int>::iterator V;
			char buff[sizeof(V) * 2] = {'\0'};

			V* vp1 = reinterpret_cast<V*>(&buff[0]);
			V* vp2 = reinterpret_cast<V*>(&buff[sizeof(V)]);

			new(vp1) V{};
			new(vp2) V{};
			 
			vp2->~V();
			EXPECT( memcmp(vp1, vp2, sizeof(V)) == 0);
		},
		CASE( "unordered_map::iterator, zeroed" )
		{
			// we need to start with zeroed memory
			typedef safememory::unordered_map<int, int>::iterator V;
			char buff[sizeof(V) * 2] = {'\0'};

			V* vp1 = reinterpret_cast<V*>(&buff[0]);
			V* vp2 = reinterpret_cast<V*>(&buff[sizeof(V)]);

			new(vp1) V{};
			new(vp2) V{};
			 
			memset(vp2, 0, sizeof(V));
			EXPECT( memcmp(vp1, vp2, sizeof(V)) == 0);
		},

		CASE( "unordered_map::iterator_safe, dtor" )
		{
			// we need to start with zeroed memory
			typedef safememory::unordered_map<int, int>::iterator_safe V;
			char buff[sizeof(V) * 2] = {'\0'};

			V* vp1 = reinterpret_cast<V*>(&buff[0]);
			V* vp2 = reinterpret_cast<V*>(&buff[sizeof(V)]);

			new(vp1) V{};
			new(vp2) V{};
			 
			vp2->~V();
			EXPECT( memcmp(vp1, vp2, sizeof(V)) == 0);
		},
		// CASE( "unordered_map::iterator_safe, zeroed" )
		// {
		// 	// we need to start with zeroed memory
		// 	typedef safememory::unordered_map<int, int>::iterator_safe V;
		// 	char buff[sizeof(V) * 2] = {'\0'};

		// 	V* vp1 = reinterpret_cast<V*>(&buff[0]);
		// 	V* vp2 = reinterpret_cast<V*>(&buff[sizeof(V)]);

		// 	new(vp1) V{};
		// 	new(vp2) V{};
			 
		// 	memset(vp2, 0, sizeof(V));
		// 	EXPECT( memcmp(vp1, vp2, sizeof(V)) == 0);
		// },

	};

	int ret = lest::run( specification, argc, argv ); 
	return ret;
}

int main( int argc, char * argv[] )
{
	nodecpp::log::Log log;
	log.level = nodecpp::log::LogLevel::info;
	log.add( stdout );
	nodecpp::logging_impl::currentLog = &log;

	nodecpp::iibmalloc::ThreadLocalAllocatorT allocManager;
	nodecpp::iibmalloc::ThreadLocalAllocatorT* formerAlloc = nodecpp::iibmalloc::setCurrneAllocator( &allocManager );
//	ThreadLocalAllocatorT* formerAlloc = nullptr;

#ifndef NODECPP_DISABLE_ZOMBIE_ACCESS_EARLY_DETECTION
	NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, safememory::detail::doZombieEarlyDetection( true ) ); // enabled by default
#endif // NODECPP_DISABLE_ZOMBIE_ACCESS_EARLY_DETECTION


	int ret = testWithLest( argc, argv );
	safememory::detail::killAllZombies();

	nodecpp::log::default_log::fatal( "about to exit..." );

	setCurrneAllocator( formerAlloc );
	return ret;
}
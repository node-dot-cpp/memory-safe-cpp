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
#include <safememory/safe_ptr.h>
#include <iibmalloc.h>
#include <nodecpp_assert.h>


int testWithLest( int argc, char * argv[] )
{
	const lest::test specification[] =
	{
		CASE( "zombie regular iterator" )
		{
			safememory::vector<int> v;

			v.push_back(0);
			auto it = v.begin();

			v.pop_back();

			EXPECT_THROWS_AS( *it, nodecpp::error::memory_error );
		},

		CASE( "zombie safe iterator on the stack" )
		{
			safememory::vector<int> v;

			v.push_back(0);
			auto it = v.begin_safe();

			v.pop_back();

			EXPECT_THROWS_AS( *it, nodecpp::error::memory_error );
		},

		CASE( "zombie safe iterator on the stack, container destructed" )
		{
			safememory::vector<int>::iterator_safe it;
			{
				safememory::vector<int> v;
				v.push_back(0);
				it = v.begin_safe();
			}

			EXPECT_THROWS_AS( *it, nodecpp::error::memory_error );
		},

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
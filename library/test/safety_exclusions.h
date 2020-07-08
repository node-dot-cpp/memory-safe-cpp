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

#ifndef SAFETY_EXCLUSIONS_H
#define SAFETY_EXCLUSIONS_H

// NOTE:
//	this file (more precisely, a file path defined as NODECPP_MEMORY_SAFETY_EXCLUSIONS) is included within namespace nodecpp::safe_memory (see safe_ptr_common.h for details)
//	Threrefore, nodecpp::safe_memory::... qualification is nor required (moreover, gcc will be against that :) )

/* Sample of user-defined exclusion:
template<> struct safeness_declarator<double> { static constexpr memory_safety is_safe = memory_safety::none; };
*/


template<> struct safeness_declarator<double> { static constexpr memory_safety is_safe = memory_safety::none; };

namespace testing::dummy_objects {
struct StructureWithSoftPtrDeclaredUnsafe; // forward declaration
}
template<> struct safeness_declarator<testing::dummy_objects::StructureWithSoftPtrDeclaredUnsafe> { static constexpr memory_safety is_safe = memory_safety::none; }; // user-defined exclusion


#endif
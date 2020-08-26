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

#ifndef SAFE_MEMORY_CHECKER_ATTRIBUTES_H
#define SAFE_MEMORY_CHECKER_ATTRIBUTES_H

#ifdef SAFE_MEMORY_CHECKER_EXTENSIONS

#define NODECPP_MAY_EXTEND_TO_THIS [[safe_memory::may_extend_to_this]]
#define NODECPP_NO_AWAIT [[safe_memory::no_await]]
#define NODECPP_NAKED_STRUCT [[safe_memory::naked_struct]]
#define NODECPP_DEEP_CONST [[safe_memory::deep_const]]
#define SAFE_MEMORY_MAY_EXTEND_TO_THIS [[safe_memory::may_extend_to_this]]
#define SAFE_MEMORY_NO_AWAIT [[safe_memory::no_await]]
#define SAFE_MEMORY_AWAITABLE [[safe_memory::awaitable]]
#define SAFE_MEMORY_NAKED_STRUCT [[safe_memory::naked_struct]]
#define SAFE_MEMORY_DEEP_CONST [[safe_memory::deep_const]]
#define SAFE_MEMORY_DEEP_CONST_WHEN_PARAMS [[safe_memory::deep_const_when_params]]
#define SAFE_MEMORY_NO_SIDE_EFFECT [[safe_memory::no_side_effect]]
#define SAFE_MEMORY_NO_SIDE_EFFECT_WHEN_CONST [[safe_memory::no_side_effect_when_const]]
#define SAFE_MEMORY_CHECK_AS_USER_CODE [[safe_memory::check_as_user_code]]

#else

#define NODECPP_MAY_EXTEND_TO_THIS
#define NODECPP_NO_AWAIT
#define NODECPP_NAKED_STRUCT
#define NODECPP_DEEP_CONST
#define SAFE_MEMORY_MAY_EXTEND_TO_THIS
#define SAFE_MEMORY_NO_AWAIT
#define SAFE_MEMORY_AWAITABLE
#define SAFE_MEMORY_NAKED_STRUCT
#define SAFE_MEMORY_DEEP_CONST
#define SAFE_MEMORY_DEEP_CONST_WHEN_PARAMS
#define SAFE_MEMORY_NO_SIDE_EFFECT
#define SAFE_MEMORY_NO_SIDE_EFFECT_WHEN_CONST
#define SAFE_MEMORY_CHECK_AS_USER_CODE

#endif

#endif // SAFE_MEMORY_CHECKER_ATTRIBUTES_H

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

#ifndef SAFEMEMORY_DETAIL_CHECKER_ATTRIBUTES_H
#define SAFEMEMORY_DETAIL_CHECKER_ATTRIBUTES_H

#ifdef SAFEMEMORY_CHECKER_EXTENSIONS

#define SAFEMEMORY_MAY_EXTEND_TO_THIS [[safememory::may_extend_to_this]]
#define SAFEMEMORY_NO_AWAIT [[safememory::no_await]]
#define SAFEMEMORY_AWAITABLE [[safememory::awaitable]]
#define SAFEMEMORY_NAKED_STRUCT [[safememory::naked_struct]]
#define SAFEMEMORY_DEEP_CONST [[safememory::deep_const]]
#define SAFEMEMORY_DEEP_CONST_WHEN_PARAMS [[safememory::deep_const_when_params]]
#define SAFEMEMORY_NO_SIDE_EFFECT [[safememory::no_side_effect]]
#define SAFEMEMORY_NO_SIDE_EFFECT_WHEN_CONST [[safememory::no_side_effect_when_const]]
#define SAFEMEMORY_CHECK_AS_USER_CODE [[safememory::check_as_user_code]]

#else //SAFEMEMORY_CHECKER_EXTENSIONS

#define SAFEMEMORY_MAY_EXTEND_TO_THIS
#define SAFEMEMORY_NO_AWAIT
#define SAFEMEMORY_AWAITABLE
#define SAFEMEMORY_NAKED_STRUCT
#define SAFEMEMORY_DEEP_CONST
#define SAFEMEMORY_DEEP_CONST_WHEN_PARAMS
#define SAFEMEMORY_NO_SIDE_EFFECT
#define SAFEMEMORY_NO_SIDE_EFFECT_WHEN_CONST
#define SAFEMEMORY_CHECK_AS_USER_CODE

#endif //SAFEMEMORY_CHECKER_EXTENSIONS

#endif // SAFEMEMORY_DETAIL_CHECKER_ATTRIBUTES_H

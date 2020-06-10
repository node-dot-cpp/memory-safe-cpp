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

#ifndef SAFEMEMORY_UNORDERED_MAP_H
#define SAFEMEMORY_UNORDERED_MAP_H

namespace std {
    template<class Key>
    class hash {
        size_t operator()(const Key&) {return 0;}
    };

    template<class Key>
    class equal_to {
        bool operator()(const Key&l, const Key&r) {return l == r;}
    };
}

namespace safememory {

    template<class Key>
    class [[nodecpp::deep_const]] DefHash {
        [[nodecpp::no_side_effect]] size_t operator()(const Key&) {return 0;}
    };

    template<class Key>
    class [[nodecpp::deep_const]] DefEq {
        [[nodecpp::no_side_effect]] constexpr bool operator()(const Key&l, const Key&r) {return l == r;}
    };
    

    template<class Key, class T, class Hash = DefHash<Key>, class Eq = std::equal_to<Key>>
    class unordered_map {

    };

} // namespace nodecpp

#endif

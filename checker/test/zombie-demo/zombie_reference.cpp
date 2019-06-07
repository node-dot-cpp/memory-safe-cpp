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

#include <safe_ptr.h>
#include <safe_memory_error.h>
#include <stdio.h>

using namespace nodecpp::safememory;

struct TestOut;

struct TestIn {
    int value = 0;
    void getIn(TestOut& outter, TestIn& other);
};

struct TestOut {
    owning_ptr<TestIn> inner;
    owning_ptr<TestIn> inner2;

    void begin() {
        inner = make_owning<TestIn>();
        inner2 = make_owning<TestIn>();
        inner->getIn(*this, *inner2);
    }

    void getOut() {
        // here we create the zombie
        inner2.reset();
    }
};

void TestIn::getIn(TestOut& outter, TestIn& other) {
    outter.getOut();
 
    //at this point, 'other' is a zombie
    int v = other.value;
}

int main() {

    try {
        TestOut out;
        out.begin();
    }
    catch(nodecpp::error::memory_error&) {
        printf("\nZombie catched!\n");
        return 0;
    }

    printf("\nZombie bit you.\n");
    return 0;
}

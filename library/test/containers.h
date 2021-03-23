#ifndef SAFEMEMORY_TEST_CONTAINERS_H
#define SAFEMEMORY_TEST_CONTAINERS_H

#include <safememory/vector.h>
#include <safememory/string.h>


struct simple {
    int i;
};

void testVector() {

    safememory::vector<simple> vec = {{0}, {1}, {2}, {3}};

    vec.push_back(simple{4});
    vec.push_back(simple{5});
    vec.push_back(simple{6});
    vec.push_back(simple{7});

    NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, vec.validate() );
    NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, vec.size() == 8 );
    NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, vec.front().i == 0 );
    NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, vec.back().i == 7 );

    vec.clear();
    NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, vec.empty() );

}

void testString() {

    // sm::string_literal lit = "hello world";
    safememory::string s = safememory::string_literal("----");
    safememory::string s2("hello world");

    s.append("goodbye world");

    for(auto it = s.begin(); it != s.end(); ++it)
        s2 += *it;


    NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, s2 == safememory::string_literal("hello world----goodbye world") );

    s2.erase(s2.cbegin() + 11, s2.cbegin() + 14);
    s2[11] = '!';

    NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, s2 == safememory::string_literal("hello world!goodbye world") );
}

#endif //SAFEMEMORY_TEST_CONTAINERS_H
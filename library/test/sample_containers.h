#include <safememory/string.h>
#include <fmt/printf.h>
#include <fmt/format.h>
#include <safememory/string_format.h>
#include <safememory/vector.h>
#include <safememory/array.h>
#include <safememory/string.h>
#include <safememory/string_literal.h>
#include <safememory/string_format.h> //now has to be explicitly included... maybe shouldn't
#include <safememory/safe_ptr.h>
#include <safememory/unordered_map.h>

#include <vector>
#include <random>
#include <cassert>

#include <fmt/printf.h>

#include <iostream>

void sampleString() {

    safememory::string_literal lit = "abcdefgh";
    safememory::string s = safememory::string_literal("ijklmnop");
    safememory::string s2("qrstuvwx");
//    safememory::string s3 = "hola mundo"; //error

    s.append("---");

    for(auto it = s.begin(); it != s.end(); ++it)
        s2 += *it;

    for(auto its = s2.begin(); its != s2.end(); ++its) {
        s += *its;
    }

    s2.erase(s2.cbegin() + 2, s2.cend() - 2);

    // check that fmt::print knows how to format
    fmt::print("{}\n", safememory::to_string(42));
    fmt::print("{}\n", lit);
    fmt::print("{}\n", s2);

    // also check that std::iostream also does
    std::cout << lit << std::endl << s2 << std::endl;


    fmt::printf("done\n");
}

struct SomeStr {

    safememory::string_literal id;
    safememory::string name;
    safememory::array<int, 3> anArray;

    SomeStr() = default;
    SomeStr(safememory::string_literal&& t1, safememory::string&& t2, safememory::array<int, 3>&& t3)
        :id(std::move(t1)), name(std::move(t2)), anArray(std::move(t3)) {}

    void print() const {
        fmt::print("id:{}, name:{}, values:{}, {}, {}\n", id, name, anArray[0], anArray[1], anArray[2]);
    }
};


void sampleVector() {

    //most basic
    safememory::vector<int> vi;

    vi.push_back(3);
    vi.push_back(4);
    vi.push_back(5);



   //some more complex struct
 
    SomeStr S1{"id1", safememory::string("Jonh Doe"), {1, 2, 3}};

    safememory::vector<SomeStr> V1;
    V1.push_back(S1);
    V1.push_back(S1);
    V1.push_back(S1);

    //regular it
    for(auto it = V1.begin(); it != V1.end(); ++it) {
        it->print();
    }

    //safe it
    for(auto it = V1.begin_safe(); it != V1.end_safe(); ++it) {
        it->print();
    }

    // now more tricky, iterate safememory::array elements
    // here all safe iterators are hooked on the ControlBlock of the vector
    // vector::push_back correctly exposed the ControlBlock
    // array soft_this_ptr correctly grabed the ControlBlock
    for(auto it = V1.front().anArray.begin_safe(); it != V1.front().anArray.end_safe(); ++it) {
        fmt::print("{}\n", *it);
    }


    safememory::vector<safememory::owning_ptr<SomeStr>> V2;

    // we need a 'make_owning' that call a copy-ctor
    auto E1 = safememory::make_owning<SomeStr>("E1", safememory::string{"Name of E1"}, safememory::array<int, 3>{11, 12, 13});
    auto E2 = safememory::make_owning<SomeStr>("E2", safememory::string{"Name of E2"}, safememory::array<int, 3>{21, 22, 23});
    auto E3 = safememory::make_owning<SomeStr>("E3", safememory::string{"Name of E3"}, safememory::array<int, 3>{31, 32, 33});

    V2.push_back(std::move(E1));
    V2.push_back(std::move(E2));
    V2.push_back(std::move(E3));


    //regular it
    for(auto it = V2.begin(); it != V2.end(); ++it) {
        (*it)->print();
    }

    // again iterate safememory::array elements
    // now each SomeStr instance has its own ControlBlock and safe iterators hook there
    for(auto it = V2[1]->anArray.begin_safe(); it != V2[1]->anArray.end_safe(); ++it) {
        fmt::print("{}\n", *it);
    }

    fmt::printf("done\n");
}

template<class IT>
void printIt(IT it) {
    fmt::print("[{}] = {}\n", it->first, it->second);
}

void randomCheckMap(int sz) {

    // this test was intended for red-black tree maps

    safememory::unordered_map<int, bool> testMap;

    std::vector<int> v;
    v.resize(sz);
    for(int i = 0; i != sz; ++i) {
        v[i] = i;
    }

    std::random_device rd;
    std::mt19937 g(rd());
 
    std::shuffle(v.begin(), v.end(), g);


    // this will add elements in random order
    for(int j = 0; j < sz / 4 ; ++j) {

        testMap[v[j]] = true;
    }

    for(int k = 0; k < sz / 6; ++k) {
        testMap.erase(v[k]);
    }

    for(auto it = testMap.begin(); it != testMap.end(); ++it) {
        printIt(it);
    }


}

void sampleUnorderedMap() {


    safememory::unordered_map<int, int> aMap;

    aMap[1] = 1;
    aMap[2] = 2;
    // aMap.validate();
    aMap[3] = 3;
    // aMap.validate();
    aMap[4] = 4;
    aMap[5] = 5;

    for(auto it = aMap.begin(); it != aMap.end(); ++it) {
        printIt(it);
    }

    fmt::print("----------------\n");

    randomCheckMap(100);


    fmt::print("done\n");

}



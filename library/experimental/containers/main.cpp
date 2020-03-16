

#include "include/EASTL/map.h"
#include "include/EASTL/unordered_map.h"
#include "include/EASTL/string.h"
#include "include/EASTL/vector.h"

#include <vector>
#include <random>

template<class IT>
void printIt(IT it) {
    printf("[%d] = %d\n", it->first, it->second);
}

template<class T>
void checkMap(T& testMap) {
    if(!testMap.validate()) {
        printf("failed!!!\n");
        assert(false);
    }
}

void randomCheckMap(int sz, int ini) {

    nodecpp::unordered_map<int, bool> testMap;

    nodecpp::vector<int> v;
    v.resize(sz);
    for(int i = 0; i != sz; ++i) {
        v[i] = i;
    }

    std::random_device rd;
    std::mt19937 g(rd());
 
    std::shuffle(v.begin(), v.end(), g);

    // std::vector<int> v2;
    // v2.resize(ini);
    int j = 0;
    for(; j != 4 * ini; ++j) {

        testMap[v[j]] = true;
        // v2[j] = v[j];
        checkMap(testMap);
    }

    std::shuffle(v.begin(), v.begin() + j, g);

    int k = 0;
    for(; k != ini; ++k) {
        testMap.erase(v[k]);
        checkMap(testMap);
    }

    printf("done\n");

}

void mainForMap() {


    nodecpp::unordered_map<int, int> aMap;

    aMap[1] = 1;
    aMap[2] = 2;
    // aMap.validate();
    aMap[3] = 3;
    // aMap.validate();
    // aMap[4] = 4;
    // aMap[5] = 5;

    for(auto it = aMap.begin(); it != aMap.end(); ++it) {
        printIt(it);
    }

    // one element iteration bck
    // auto jt = aMap.end();
    // while(jt != aMap.begin()) {
    //     --jt;
    //     printIt(jt);
    // }
    // for(auto it = aMap.rbegin(); it != aMap.rend(); ++it) {
    //     printIt(it);
    // }



    // printf("----------------\n");
    // try {
    //     auto jt = aMap.begin();
    //     --jt;
    // }
    // catch(...) {
    //     printf("catched!\n");
    // }

    printf("----------------\n");

    // try {
    //     printIt(aMap.end());
    // }
    // catch(...) {
    //     printf("catched!\n");
    // }

    printf("----------------\n");

    // try {
    //     auto jt = aMap.end();
    //     ++jt;
    // }
    // catch(...) {
    //     printf("catched!\n");
    // }

    for(size_t i = 0; i != 10; ++i)
        randomCheckMap(511, 127);

}


void testString() {

    safememory::string s = safememory::string_literal("hola mundo");
    safememory::string s2 = "hola mundo";

    s.append("! - ");

    s2 += " cruel! - ";

    for(auto it = s.begin(); it != s.end(); ++it)
        s2 += *it;

    for(auto its = s2.begin(); its != s2.end(); ++its) {
        s += *its;
    }

    printf(s2.c_str());

    s2.erase(s2.cbegin() + 7, s2.cend());
    printf(s2.c_str());

}

int main() {

    testString();
    // mainForMap();
    printf("done\n");
    return 0;
}




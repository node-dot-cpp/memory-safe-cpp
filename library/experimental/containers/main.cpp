

#include "include/EASTL/map.h"
#include <vector>
#include <random>

template<class IT>
void printIt(IT it) {
    printf("[%d] = %d\n", it->first, it->second);
}

void checkMap(nodecpp::map<int, bool>& testMap) {
    if(!testMap.validate()) {
        printf("failed!!!\n");
        assert(false);
    }
}

void randomCheck(int sz, int ini) {

    nodecpp::map<int, bool> testMap;

    std::vector<int> v;
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




int main() {

    nodecpp::map<int, int> aMap;

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
    auto jt = aMap.end();
    while(jt != aMap.begin()) {
        --jt;
        printIt(jt);
    }
    // for(auto it = aMap.rbegin(); it != aMap.rend(); ++it) {
    //     printIt(it);
    // }



    printf("----------------\n");
    try {
        auto jt = aMap.begin();
        --jt;
    }
    catch(...) {
        printf("catched!\n");
    }

    printf("----------------\n");

    try {
        printIt(aMap.end());
    }
    catch(...) {
        printf("catched!\n");
    }

    printf("----------------\n");

    try {
        auto jt = aMap.end();
        ++jt;
    }
    catch(...) {
        printf("catched!\n");
    }

    for(size_t i = 0; i != 10; ++i)
        randomCheck(511, 127);

    return 0;
}




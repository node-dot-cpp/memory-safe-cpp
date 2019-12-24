

#include "include/EASTL/map.h"

template<class IT>
void printIt(IT it) {
    printf("[%d] = %d\n", it->first, it->second);
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

    return 0;
}




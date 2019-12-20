

#include "include/EASTL/map.h"

template<class IT>
void printIt(IT it) {
    printf("[%d] = %d\n", it->first, it->second);
}

int main() {

    nodecpp::map<int, int> aMap;

    aMap[1] = 1;
    // one element iteration fwd
    for(auto it = aMap.begin(); it != aMap.end(); ++it) {
        printIt(it);
    }

    // one element iteration bck
    for(auto it = aMap.rbegin(); it != aMap.rend(); ++it) {
        printIt(it);
    }

    aMap[5] = 5;
    // two elements iteration fwd
    for(auto it = aMap.begin(); it != aMap.end(); ++it) {
        printIt(it);
    }

    // two elements iteration bck
    for(auto it = aMap.rbegin(); it != aMap.rend(); ++it) {
        printIt(it);
    }

    aMap.insert(std::pair<int, int>(3,3));
    // three elements iteration
    for(auto it = aMap.begin(); it != aMap.end(); ++it) {
        printIt(it);
    }


    aMap.insert_or_assign(4,8);
    aMap.insert(std::pair<int, int>(10,10));
    aMap.insert_or_assign(4,4);
    for(auto it = aMap.cbegin(); it != aMap.cend(); ++it) {
        printIt(it);
    }
    aMap.erase(3);

    for(auto it = aMap.rbegin(); it != aMap.rend(); ++it) {
        printIt(it);
    }

    for(auto it = aMap.crbegin(); it != aMap.crend(); ++it) {
        printIt(it);
    }

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




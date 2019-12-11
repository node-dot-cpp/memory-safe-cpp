

#include "include/EASTL/map.h"

template<class IT>
void printIt(IT it) {
    printf("[%d] = %d\n", it->first, it->second);
}

int main() {

    nodecpp::map<int, int> aMap;

    aMap[1] = 1;
    aMap[5] = 5;

    for(auto it = aMap.begin(); it != aMap.end(); ++it) {
        printIt(it);
    }

    aMap.insert(std::pair<int, int>(3,3));
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
    auto jt = aMap.begin();
    printIt(jt);
    --jt;
    printIt(jt);
    --jt;
    printIt(jt);
    --jt;
    printIt(jt);

    printf("----------------\n");

    jt = aMap.end();
    printIt(jt);
    ++jt;
    printIt(jt);
    ++jt;
    printIt(jt);
    ++jt;
    printIt(jt);

    return 0;
}




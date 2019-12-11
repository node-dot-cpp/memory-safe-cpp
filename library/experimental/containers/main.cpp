

#include "include/EASTL/map.h"


int main() {

    eastl::map<int, int> aMap;

    aMap[1] = 1;
    aMap[5] = 5;

    for(auto it = aMap.begin(); it != aMap.end(); ++it) {
        printf("[%d] = %d\n", it->first, it->second);
    }

    aMap.insert(std::pair<int, int>(3,3));
    aMap.insert_or_assign(4,8);
    aMap.insert(std::pair<int, int>(10,10));
    aMap.insert_or_assign(4,4);
    for(auto it = aMap.cbegin(); it != aMap.cend(); ++it) {
        printf("[%d] = %d\n", it->first, it->second);
    }
    aMap.erase(3);

    for(auto it = aMap.rbegin(); it != aMap.rend(); ++it) {
        printf("[%d] = %d\n", it->first, it->second);
    }

    for(auto it = aMap.crbegin(); it != aMap.crend(); ++it) {
        printf("[%d] = %d\n", it->first, it->second);
    }



    return 0;
}




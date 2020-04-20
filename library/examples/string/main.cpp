
#include <safememory/string.h>
#include <fmt/printf.h>


int main() {

    safememory::string s = safememory::string_literal("hola mundo");
    safememory::string s2 = "hola mundo";

    s.append("! - ");

    s2 += " cruel! - ";

    for(auto it = s.begin(); it != s.end(); ++it)
        s2 += *it;

    for(auto its = s2.begin(); its != s2.end(); ++its) {
        s += *its;
    }

    fmt::printf(s2.c_str());

    s2.erase(s2.cbegin() + 7, s2.cend());
    fmt::printf(s2.c_str());

    fmt::printf("done\n");
    return 0;
}




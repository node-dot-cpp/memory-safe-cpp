
#include <safe_memory/string.h>
#include <fmt/printf.h>
#include <fmt/format.h>
#include <safe_memory/string_format.h>

#include <iostream>

namespace sm = safe_memory;

int main() {

    sm::string_literal lit = "hola mundo";
    sm::string s = sm::string_literal("hola mundo");
    sm::string s2("hola mundo");
//    safe_memory::string s3 = "hola mundo"; //error

    s.append("! - ");

    for(auto it = s.begin(); it != s.end(); ++it)
        s2 += *it;

    for(auto its = s2.begin(); its != s2.end(); ++its) {
        s += *its;
    }

    fmt::print("{}\n", sm::to_string(42)); //sm::string_literal
    fmt::print("{}\n", lit); //sm::string_literal
    fmt::print("{}\n", s2); //sm::string

    std::cout << lit << std::endl << s2 << std::endl;

    fmt::printf(s2.c_str());

    s2.erase(s2.cbegin() + 7, s2.cend());
    fmt::printf(s2.c_str());

    fmt::printf("done\n");
    return 0;
}




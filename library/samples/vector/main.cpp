
#include <safe_memory/vector.h>
#include <fmt/printf.h>


int main() {

    safe_memory::vector<int> vi;

    vi.push_back(3);
    vi.push_back(4);
    vi.push_back(5);


    fmt::printf("done\n");
    return 0;
}




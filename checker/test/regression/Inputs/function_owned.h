#ifndef FUNCTION_OWNED_H
#define FUNCTION_OWNED_H


#include <functional>

namespace nodecpp {

template<typename ...ARGS>
using function_owned_arg0 = std::function<ARGS...>;
}

#endif //FUNCTION_OWNED_H

// RUN: nodecpp-checker %s -- -std=c++11 -nostdinc -isystem %S/Inputs -isystem %S/../../3rdparty/clang/lib/Headers | FileCheck %s -implicit-check-not="{{warning|error}}:"

#include <function_owned.h>

void my_sort(std::function<void()>) {}

class S57 {
public:
    void on(std::function<void()> cb [[nodecpp::may_extend_to_this]]) {}

    void prohibit() {

        int i = 0;
        auto l = [&i]() {};
// CHECK: :[[@LINE-1]]:20: warning: unsafe capture to extend scope
        this->on(l);
    }

    void allow() {

        auto l1 = [this]() {};
        this->on(l1);


        int i = 0;
        auto l2 = [&i]() {};
        my_sort(l2);         
    }
};


// RUN: nodecpp-instrument %s | FileCheck %s -allow-empty -implicit-check-not="{{warning|error}}:"



void func(int* ip, int& ir) {

    int* ip2 = ip;
    int& ir2 = ir;

    int i = *ip2;
    i = ir2;
}


/*******************************************************************************
  Copyright (C) 2016 OLogN Technologies AG
*******************************************************************************/

#ifndef RAIISTDIOFILE_H_INCLUDED
#define RAIISTDIOFILE_H_INCLUDED

#include <stdio.h>

class RaiiStdioFile {
    FILE* f;

public:
    RaiiStdioFile(FILE* f_)
        : f(f_) {
    }
    ~RaiiStdioFile() {
        if(f)
            fclose(f);
    }

    RaiiStdioFile(const RaiiStdioFile&) = delete;
    RaiiStdioFile& operator =(const RaiiStdioFile&) = delete;

    FILE* get() {
        return f;
    }
};

#endif //RAIISTDIOFILE_H_INCLUDED

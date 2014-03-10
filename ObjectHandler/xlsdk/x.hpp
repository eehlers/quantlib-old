
#ifndef oh_x_hpp
#define oh_x_hpp

#include <xlsdk/xlsdkdefines.hpp>

DLLEXPORT void xx(const char *c0, const char *c1, const char *c2 = "");
struct AA {
    AA(const char *c) : c_(c) {
        xx(">", c_, "STRT");
    }
    ~AA() {
        xx("<", c_, "STOP");
    }
    const char *c_;
};

#endif


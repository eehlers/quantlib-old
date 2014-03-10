
#include <xlsdk/x.hpp>
#include <sstream>
#include <iostream>
#include <fstream>

#include "windows.h"
#include "psapi.h"

std::ofstream &os() {
    static std::ofstream s("C:\\Users\\erik\\Documents\\junk\\x3.txt");
    return s;
}

SIZE_T a1 = 0;
long i = 0;

DLLEXPORT void xx(const char *c0, const char *c1, const char *c2) {
    PROCESS_MEMORY_COUNTERS_EX pmc;
    GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
    SIZE_T m = pmc.PrivateUsage;
    //SIZE_T m = pmc.WorkingSetSize;
    std::ofstream &os1 = os();
    if (m > a1) {
        os1 << i++ << " *******INCREASE" << std::endl;
        a1 = m;
    } else if (m < a1) {
        os1 << i++ << " *******DECREASE" << std::endl;
        a1 = m;
    }
    os1 << i++ << " " << m << " " << c0 << " " << c1 << " " << c2 << std::endl;
}


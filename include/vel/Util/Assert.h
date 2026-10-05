#pragma once

#include <cstdio>
#include <cstdlib>
#include <iostream>

#ifdef ENABLE_VEL_ASSERT

#define VEL_ASSERT(condition, message) \
    do \
    { \
        if (!(condition)) \
        { \
            std::fprintf(stderr, "%s\n", message); \
            /*std::cin.get();*/ \
            std::abort(); \
        } \
    } while (0)

#else

#define VEL_ASSERT(condition, message) ((void)0)

#endif
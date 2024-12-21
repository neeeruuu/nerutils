#pragma once

#include <cstdlib>

void nutil_error(const char* err);

#ifdef _MSC_VER
    #include <intrin.h>
    #define DEBUGBREAK() __debugbreak()
    #define ASSUME(cond) __assume(cond)
#elif defined(__GNUC__) || defined(__clang__)
    #include <signal.h>
    #define DEBUGBREAK() raise(SIGTRAP)
    #define ASSUME(cond) __builtin_assume(cond)
#else
    #define DEBUGBREAK()                                                                                               \
        do                                                                                                             \
        {                                                                                                              \
        } while (0)
    #define ASSUME(cond)                                                                                               \
        do                                                                                                             \
        {                                                                                                              \
        } while (0)
#endif

#ifdef _DEBUG
    #define ACTUAL_STRINGIZE(x) #x
    #define STRINGIZE(x) ACTUAL_STRINGIZE(x)

    #define ASSERT(cond, msg)                                                                                          \
        do                                                                                                             \
        {                                                                                                              \
            if (!(cond))                                                                                               \
            {                                                                                                          \
                nutil_error(msg "\nfile:" __FILE__ "\nline:" STRINGIZE(__LINE__));                                     \
                DEBUGBREAK();                                                                                          \
                exit(0);                                                                                               \
            }                                                                                                          \
        } while (false);                                                                                               \
        ASSUME(cond);
#else
    #define ASSERT(cond, msg)                                                                                          \
        do                                                                                                             \
        {                                                                                                              \
            if (!(cond))                                                                                               \
            {                                                                                                          \
                nutil_error(msg);                                                                                      \
                DEBUGBREAK();                                                                                          \
            }                                                                                                          \
        } while (false);                                                                                               \
        ASSUME(cond);
#endif

#define PANIC(msg)                                                                                                     \
    do                                                                                                                 \
    {                                                                                                                  \
        nutil_error(msg);                                                                                              \
        DEBUGBREAK();                                                                                                  \
    } while (false)

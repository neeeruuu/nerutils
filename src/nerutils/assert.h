#pragma once

void nutil_error(const char* err);

#ifdef _MSC_VER
    #define ASSUME(cond) __assume(cond)
#elif defined(__GNUC__) || defined(__clang__)
    #define ASSUME(cond) __builtin_assume(cond)
#else
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
            }                                                                                                          \
        } while (false) ASSUME(cond);
#else
    #define ASSERT(cond, msg)                                                                                          \
        do                                                                                                             \
        {                                                                                                              \
            if (!(cond))                                                                                               \
            {                                                                                                          \
                nutil_error(msg);                                                                                      \
            }                                                                                                          \
        } while (false) ASSUME(cond);
#endif

#define PANIC(msg) nutil_error(msg);

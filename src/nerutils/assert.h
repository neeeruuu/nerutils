#pragma once

void nutil_error(const char *err);

#ifdef _DEBUG
#define ACTUAL_STRINGIZE(x) #x
#define STRINGIZE(x) ACTUAL_STRINGIZE(x)

#define ASSERT(cond, msg)                                                      \
    do                                                                         \
    {                                                                          \
        if (!(cond))                                                           \
        {                                                                      \
            nutil_error(msg "\nfile:" __FILE__ "\nline:" STRINGIZE(__LINE__)); \
        }                                                                      \
    } while (false)
#else
#define ASSERT(cond, msg)     \
    do                        \
    {                         \
        if (!(cond))          \
        {                     \
            nutil_error(msg); \
        }                     \
    } while (false)
#endif

#define PANIC(msg) nutil_error(msg);

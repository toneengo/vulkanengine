#pragma once
#include <fmt/printf.h>
#include <cassert>
#include <vulkan/vk_enum_string_helper.h>
#define VK_CHECK(x)                                                     \
    do {                                                                \
        VkResult err = x;                                               \
        if (err) {                                                      \
            fmt::print("Detected Vulkan error: {}", string_VkResult(err)); \
            abort();                                                    \
        }                                                               \
    } while (0)

inline void log_line(const char* source)
{
    char buf[512];

    uint32_t linenum = 1;
    uint32_t bufpos = 0;

    for (const char* c = source; *c; c++)
    {
        assert(bufpos < 512);
        if (*c == '\n')
        {
            buf[bufpos] = '\0';
            fmt::printf("%5u | %s\n", linenum, buf);
            linenum++;
            bufpos = 0;
            continue;
        }
        
        buf[bufpos++] = *c;
    }
}

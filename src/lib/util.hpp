#pragma once
#include <cassert>
#include <vulkan/vk_enum_string_helper.h>
#define VK_CHECK(x)                                                     \
    do {                                                                \
        VkResult err = x;                                               \
        if (err) {                                                      \
            printf("Detected Vulkan error: %s", string_VkResult(err)); \
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
            printf("%5u | %s\n", linenum, buf);
            linenum++;
            bufpos = 0;
            continue;
        }
        
        buf[bufpos++] = *c;
    }
}

#define NS_PER_SEC 1000000000
#define MS_PER_SEC 1000
#define NS_PER_MS  1000000

#define write_uniform_buffer_descriptor(ds, src, size) do {\
    spock::Buffer buf = create_buffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);\
    frame->destroyQueue.push(buf);\
    void* dst = buf.info.pMappedData;\
    memcpy(dst, src, size);\
    update_descriptor_sets({}, {{ds, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, buf.buffer, 0, size}});\
} while(0)


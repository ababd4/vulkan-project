#pragma once

#include <fmt/core.h>
#include "../Util/types.h"
#include <vulkan/vk_enum_string_helper.h>


// macro to check for errors
#define VK_CHECK(x)                                                     \
    do {                                                                \
        VkResult err = x;                                               \
        if (err) {                                                      \
             fmt::print("Detected Vulkan error: {}", string_VkResult(err)); \
            abort();                                                    \
        }                                                               \
    } while (0)

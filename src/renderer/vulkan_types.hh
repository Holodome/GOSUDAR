/*
Author: Holodome
Date: 03.10.2021
File: src/renderer/vulkan_types.hh
Version: 0
*/

#pragma once

#include "lib/general.hh"
#include <MoltenVK/mvk_vulkan.h>

#define VK_CHECK_CALL(_func, ...) do { \
    VkResult result = _func(__VA_ARGS__); \
    if (result != VK_SUCCESS) { \
        log_error("%s failed with code %d", #_func, (int)result); \
        DBG_BREAKPOINT; \
    } \
} while (0);

struct VulkanCtx {
    VkAllocationCallbacks* allocator;
    
    VkInstance instance; 
    VkDebugUtilsMessengerEXT debug_messanger; 
    
    VkPhysicalDeviceProperties physical_device_properties;
    VkPhysicalDeviceFeatures physical_device_features;
    VkPhysicalDeviceMemoryProperties physical_device_memory_properties;
    VkPhysicalDevice physical_device;
    u32 graphics_family_idx;
    u32 present_family_idx;
    u32 transfer_family_idx;
    
    VkSurfaceKHR surface;
    VkDevice logical_device;
    VkQueue graphics_queue;
    VkQueue present_queue;
    VkQueue transfer_queue;
};

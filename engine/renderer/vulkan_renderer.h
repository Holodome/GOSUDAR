/*
Author: Holodome
Date: 06.10.2021
File: engine/renderer/vulkan_types.h
Version: 0
*/
#pragma once 
#include "lib/general.h"
#include "logging.h"

#include "renderer/renderer.h"

#include <MoltenVK/mvk_vulkan.h>

const char *vulkan_get_result_str(VkResult result);
const char *vulkan_get_result_str_verbose(VkResult result);
bool vulkan_result_is_success(VkResult result);

#define VK_CHECK_CALL(_func, ...) do { \
    VkResult result = _func(__VA_ARGS__); \
    if (!vulkan_result_is_success(result)) { \
        log_error("%s failed with code %d (%s)", \
            #_func, (int)result, vulkan_get_result_str_verbose(result)); \
        DBG_BREAKPOINT; \
    } \
} while (0);


typedef struct Vulkan_Ctx {
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
} Vulkan_Ctx;

typedef Vulkan_Ctx Renderer_Internal;

void vulkan_init(Renderer *renderer, struct Window_State *window);
void vulkan_execute_commands(Renderer *renderer, Renderer_Commands *commands);
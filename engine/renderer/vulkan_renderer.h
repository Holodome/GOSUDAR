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

typedef struct {
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR *formats; // da
    VkPresentModeKHR *present_modes; // da
} Vulkan_Swapchain_Support;

Vulkan_Swapchain_Support get_swapchain_support(VkPhysicalDevice device, VkSurfaceKHR surface);
void free_swapchain_support(Vulkan_Swapchain_Support support);

enum {
    VULKAN_PHYSICAL_DEVICE_GRAPHICS_BIT = 0x1,  
    VULKAN_PHYSICAL_DEVICE_PRESENT_BIT = 0x2, 
    // VULKAN_PHYSICAL_DEVICE_COMPUTE_BIT = 0x4, 
    VULKAN_PHYSICAL_DEVICE_TRANSFER_BIT = 0x8,
    VULKAN_PHYSICAL_DEVICE_SAMPLER_ANISOTROPY_BIT = 0x10,
};

typedef struct {
    u32 flags;
    const char **device_extensions;
    u32 extension_count;
} Vulkan_Physical_Device_Requirements;

typedef struct {
    bool is_valid;
    VkPhysicalDevice handle;
    Vulkan_Swapchain_Support swapchain_support;
    
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceMemoryProperties memory_properties;
    u32 graphics_family_idx;
    u32 present_family_idx;
    u32 transfer_family_idx;
} Vulkan_Physical_Device;

Vulkan_Physical_Device pick_physical_device(VkInstance instance, VkSurfaceKHR surface, Vulkan_Physical_Device_Requirements *requirements);
void log_physical_device_info(Vulkan_Physical_Device *device);

typedef struct {
    VkDevice handle;
    VkFormat depth_format;
    VkCommandPool graphics_command_pool;

    VkQueue graphics_queue;
    VkQueue present_queue;
    VkQueue transfer_queue;
    
    Vulkan_Swapchain_Support swapchain_support;
} Vulkan_Device;

Vulkan_Device create_vulkan_device(VkInstance instance, VkSurfaceKHR surface, 
    Vulkan_Physical_Device *physical_device);

typedef struct {
    VkImage handle;
    VkDeviceMemory memory;
    VkImageView view;
    u32 width;
    u32 height;
} Vulkan_Image;

typedef struct {
    VkSwapchainKHR handle;
    VkSurfaceFormatKHR image_format;
    u8 max_frames_in_flight;
    VkImage *images; // da
    VkImageView *image_views; // da
    
} Vulkan_Swapchain;


typedef struct Vulkan_Ctx {
    VkAllocationCallbacks* allocator;
    VkDebugUtilsMessengerEXT debug_messanger; 
    
    VkInstance instance; 
    VkSurfaceKHR surface;
    
    Vulkan_Physical_Device physical_device;
    Vulkan_Device device;
} Vulkan_Ctx;

typedef Vulkan_Ctx Renderer_Internal;

void vulkan_init(Renderer *renderer, struct Window_State *window);
void vulkan_execute_commands(Renderer *renderer, Renderer_Commands *commands);
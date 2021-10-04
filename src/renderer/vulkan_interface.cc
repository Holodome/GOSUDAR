#include "renderer/vulkan_interface.hh"

#include "lib/strings.hh"
#include "lib/lists.hh"
#include "lib/logging.hh"

#include <vulkan/vulkan_macos.h>

static VulkanCtx ctx;

const char *required_layers[] = {
    "VK_LAYER_KHRONOS_validation"
};

static VKAPI_ATTR VkBool32 VKAPI_CALL 
vk_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_types,
    const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
    void *user_data) {
    (void)user_data;
    (void)message_types;
    switch (message_severity) {
    default:
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: {
        log_error("VK: %s", callback_data->pMessage);
    } break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: {
        log_warn("VK: %s", callback_data->pMessage);
    } break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: 
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: {
        log_info("VK: %s", callback_data->pMessage);
    } break;
    }
    return false;
}

static void 
create_instance() {
    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.apiVersion = VK_API_VERSION_1_2;
    
    const char *required_extensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
#if INTERNAL_BUILD
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
        VK_MVK_MACOS_SURFACE_EXTENSION_NAME,
    };
    
    u32 avialable_layer_count = 0;
    VK_CHECK_CALL(vkEnumerateInstanceLayerProperties, &avialable_layer_count, 0);
    auto aviable_layers = TempArray<VkLayerProperties>(avialable_layer_count);
    VK_CHECK_CALL(vkEnumerateInstanceLayerProperties, &avialable_layer_count, aviable_layers.data);
    for (uptr i = 0; i < ARRAY_SIZE(required_layers); ++i) {
        const char *layer_name = required_layers[i];
        bool is_found = false;
        for (uptr j = 0; j < avialable_layer_count; ++j) {
            if (str_eq(aviable_layers[j].layerName, layer_name)) {
                is_found = true;
                break;
            }
        }
        
        if (!is_found) {
            log_error("Required validation layer is missing: %s'", layer_name);
        }
    }
    
    VkInstanceCreateInfo instance_create_info = {};
    instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.pApplicationInfo = &app_info;
    instance_create_info.enabledExtensionCount = ARRAY_SIZE(required_extensions);
    instance_create_info.ppEnabledExtensionNames = required_extensions;
    // @TODO(hl): see why it is not working
    // instance_create_info.enabledLayerCount = ARRAY_SIZE(required_layers);
    instance_create_info.ppEnabledLayerNames = required_layers;
    
    VK_CHECK_CALL(vkCreateInstance, &instance_create_info, ctx.allocator, &ctx.instance);
}
    
static void 
setup_debug_messanger() {
    u32 log_severity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
    VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {};
    debug_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debug_create_info.messageSeverity = log_severity;
    debug_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    debug_create_info.pfnUserCallback = vk_debug_callback;
    
    PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = 
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(ctx.instance, "vkCreateDebugUtilsMessengerEXT");
    assert(vkCreateDebugUtilsMessengerEXT);
    vkCreateDebugUtilsMessengerEXT(ctx.instance, &debug_create_info, ctx.allocator, &ctx.debug_messanger);
    
}
    
static void
pick_physical_device() {
    u32 device_count = 0;
    VK_CHECK_CALL(vkEnumeratePhysicalDevices, ctx.instance, &device_count, 0);
    if (!device_count) {
        log_error("Unable to find GPUs with Vulkan support");
        return;
    }
    auto devices = TempArray<VkPhysicalDevice>(device_count);
    VK_CHECK_CALL(vkEnumeratePhysicalDevices, ctx.instance, &device_count, devices.data);
   
    for (uptr i = 0; i < device_count; ++i) {
        VkPhysicalDevice test_device = devices[i];
        
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(test_device, &properties);
        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures(test_device, &features);
        VkPhysicalDeviceMemoryProperties memory;
        vkGetPhysicalDeviceMemoryProperties(test_device, &memory);
        
        u32 queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(test_device, &queue_family_count, 0);
        auto queue_families = TempArray<VkQueueFamilyProperties>(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(test_device, &queue_family_count, queue_families.data);
        
        bool is_graphics_family_set = false;
        bool is_present_family_set = false;
        bool is_transfer_family_set = false;
        for (uptr j = 0; j < queue_family_count; ++j) {
            VkQueueFamilyProperties *queue_properties = &queue_families[j];
            VkBool32 present_support = false;
            VK_CHECK_CALL(vkGetPhysicalDeviceSurfaceSupportKHR, test_device, j, ctx.surface, &present_support);
            if (present_support) {
                ctx.present_family_idx = j;
                is_present_family_set = true;
            }
            if (queue_properties->queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                ctx.graphics_family_idx = j;
                is_graphics_family_set = true;
            }
            if (queue_properties->queueFlags & VK_QUEUE_TRANSFER_BIT) {
                ctx.transfer_family_idx = j;
                is_transfer_family_set = true;
            }
        }
        
        if (is_present_family_set && is_graphics_family_set && is_transfer_family_set) {
            log_info("Selected device name: %s", properties.deviceName);
            switch (properties.deviceType) {
            default:
            case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                log_info("GPU type is Unknown.");
                break;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                log_info("GPU type is Integrated.");
                break;
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                log_info("GPU type is Descrete.");
                break;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                log_info("GPU type is Virtual.");
                break;
            case VK_PHYSICAL_DEVICE_TYPE_CPU:
                log_info("GPU type is CPU.");
                break;
            }
            log_info("GPU driver version: %d.%d.%d",
                VK_VERSION_MAJOR(properties.driverVersion),
                VK_VERSION_MINOR(properties.driverVersion),
                VK_VERSION_PATCH(properties.driverVersion));
            log_info("Vulkan version: %d.%d.%d",
                VK_VERSION_MAJOR(properties.apiVersion),
                VK_VERSION_MINOR(properties.apiVersion),
                VK_VERSION_PATCH(properties.apiVersion));
                
            for (u32 j = 0; j < memory.memoryHeapCount; ++j) {
                f32 memory_size_gib = (((f32)memory.memoryHeaps[j].size) / 1024.0f / 1024.0f / 1024.0f);
                if (memory.memoryHeaps[j].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                    log_info("Local GPU memory: %.2f GiB", memory_size_gib);
                } else {
                    log_info("Shared System memory: %.2f GiB", memory_size_gib);
                }
            }
            
            ctx.physical_device = test_device;
            ctx.physical_device_properties = properties;
            ctx.physical_device_features = features;
            ctx.physical_device_memory_properties = memory;
            break;
        }
    }
    assert(ctx.physical_device);
}

static void
create_logical_device() {
    u32 queue_create_info_count = 0;
    f32 priority = 1.0f;
    VkDeviceQueueCreateInfo queue_create_infos[3];
    {
        VkDeviceQueueCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        info.queueFamilyIndex = ctx.graphics_family_idx;
        info.queueCount = 1;
        info.pQueuePriorities = &priority;
        queue_create_infos[queue_create_info_count++] = info;
    }
    if (ctx.graphics_family_idx != ctx.present_family_idx) {
        VkDeviceQueueCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        info.queueFamilyIndex = ctx.present_family_idx;
        info.queueCount = 1;
        info.pQueuePriorities = &priority;
        queue_create_infos[queue_create_info_count++] = info;
    }
    if (ctx.graphics_family_idx != ctx.transfer_family_idx) {
        VkDeviceQueueCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        info.queueFamilyIndex = ctx.transfer_family_idx;
        info.queueCount = 1;
        info.pQueuePriorities = &priority;
        queue_create_infos[queue_create_info_count++] = info;
    }
    
    VkPhysicalDeviceFeatures device_features = {};
    
    // @TODO(hl): Check on extension aviability
    const char *device_extensions[] = {
        "VK_KHR_swapchain"
    };
    
    VkDeviceCreateInfo device_create_info = {};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pQueueCreateInfos = queue_create_infos;
    device_create_info.queueCreateInfoCount = queue_create_info_count;
    device_create_info.pEnabledFeatures = &device_features;
    device_create_info.enabledExtensionCount = ARRAY_SIZE(device_extensions);
    device_create_info.ppEnabledExtensionNames = device_extensions;
    // device_create_info.enabledLayerCount = ARRAY_SIZE(required_layers);
    // device_create_info.ppEnabledLayerNames = required_layers;
    
    VK_CHECK_CALL(vkCreateDevice, ctx.physical_device, &device_create_info, 0, &ctx.logical_device);
    
    vkGetDeviceQueue(ctx.logical_device, ctx.graphics_family_idx, 0, &ctx.graphics_queue);
    vkGetDeviceQueue(ctx.logical_device, ctx.present_family_idx, 0, &ctx.present_queue);
    vkGetDeviceQueue(ctx.logical_device, ctx.transfer_family_idx, 0, &ctx.transfer_queue);
}   

static void 
create_surface(void *content_view) {
    VkMacOSSurfaceCreateInfoMVK surface_create_info = {};
    surface_create_info.sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK;
    surface_create_info.pView = content_view;
    VK_CHECK_CALL(vkCreateMacOSSurfaceMVK, ctx.instance, &surface_create_info, ctx.allocator, &ctx.surface);
}
 
void 
init_vulkan(void *content_view) {
    create_instance();
    setup_debug_messanger();
    create_surface(content_view);
    pick_physical_device();
    create_logical_device();
    
}

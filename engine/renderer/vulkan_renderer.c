#include "renderer/vulkan_renderer.h"

#include "lib/memory.h"
#include "lib/lists.h"
#include "lib/strings.h"
#include "platform/window.h"

#include "logging.h"

const char *required_layers[] = {
    "VK_LAYER_KHRONOS_validation"
};

const char *
vulkan_get_result_str(VkResult result) {
    const char *str = "(unknown)";
    switch (result) {
    case VK_SUCCESS: {
        str = "VK_SUCCESS";
    } break;
    case VK_NOT_READY: {
        str = "VK_NOT_READY";
    } break;
    case VK_TIMEOUT: {
        str = "VK_TIMEOUT";
    } break;
    case VK_EVENT_SET: {
        str = "VK_EVENT_SET";
    } break;
    case VK_EVENT_RESET: {
        str = "VK_EVENT_RESET";
    } break;
    case VK_INCOMPLETE: {
        str = "VK_INCOMPLETE";
    } break;
    case VK_SUBOPTIMAL_KHR: {
        str = "VK_SUBOPTIMAL_KHR";
    } break;
    case VK_THREAD_IDLE_KHR: {
        str = "VK_THREAD_IDLE_KHR";
    } break;
    case VK_THREAD_DONE_KHR: {
        str = "VK_THREAD_DONE_KHR";
    } break;
    case VK_OPERATION_DEFERRED_KHR: {
        str = "VK_OPERATION_DEFERRED_KHR";
    } break;
    case VK_OPERATION_NOT_DEFERRED_KHR: {
        str = "VK_OPERATION_NOT_DEFERRED_KHR";
    } break;
    case VK_PIPELINE_COMPILE_REQUIRED_EXT: {
        str = "VK_PIPELINE_COMPILE_REQUIRED_EXT";
    } break;
    case VK_ERROR_OUT_OF_HOST_MEMORY: {
        str = "VK_ERROR_OUT_OF_HOST_MEMORY";
    } break;
    case VK_ERROR_OUT_OF_DEVICE_MEMORY: {
        str = "VK_ERROR_OUT_OF_DEVICE_MEMORY";
    } break;
    case VK_ERROR_INITIALIZATION_FAILED: {
        str = "VK_ERROR_INITIALIZATION_FAILED";
    } break;
    case VK_ERROR_DEVICE_LOST: {
        str = "VK_ERROR_DEVICE_LOST";
    } break;
    case VK_ERROR_MEMORY_MAP_FAILED: {
        str = "VK_ERROR_MEMORY_MAP_FAILED";
    } break;
    case VK_ERROR_LAYER_NOT_PRESENT: {
        str = "VK_ERROR_LAYER_NOT_PRESENT";
    } break;
    case VK_ERROR_EXTENSION_NOT_PRESENT: {
        str = "VK_ERROR_EXTENSION_NOT_PRESENT";
    } break;
    case VK_ERROR_FEATURE_NOT_PRESENT: {
        str = "VK_ERROR_FEATURE_NOT_PRESENT";
    } break;
    case VK_ERROR_INCOMPATIBLE_DRIVER: {
        str = "VK_ERROR_INCOMPATIBLE_DRIVER";
    } break;
    case VK_ERROR_TOO_MANY_OBJECTS: {
        str = "VK_ERROR_TOO_MANY_OBJECTS";
    } break;
    case VK_ERROR_FORMAT_NOT_SUPPORTED: {
        str = "VK_ERROR_FORMAT_NOT_SUPPORTED";
    } break;
    case VK_ERROR_FRAGMENTED_POOL: {
        str = "VK_ERROR_FRAGMENTED_POOL";
    } break;
    case VK_ERROR_SURFACE_LOST_KHR: {
        str = "VK_ERROR_SURFACE_LOST_KHR";
    } break;
    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: {
        str = "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
    } break;
    case VK_ERROR_OUT_OF_DATE_KHR: {
        str = "VK_ERROR_OUT_OF_DATE_KHR";
    } break;
    case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: {
        str = "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
    } break;
    case VK_ERROR_INVALID_SHADER_NV: {
        str = "VK_ERROR_INVALID_SHADER_NV";
    } break;
    case VK_ERROR_OUT_OF_POOL_MEMORY: {
        str = "VK_ERROR_OUT_OF_POOL_MEMORY";
    } break;
    case VK_ERROR_INVALID_EXTERNAL_HANDLE: {
        str = "VK_ERROR_INVALID_EXTERNAL_HANDLE";
    } break;
    case VK_ERROR_FRAGMENTATION: {
        str = "VK_ERROR_FRAGMENTATION";
    } break;
    case VK_ERROR_INVALID_DEVICE_ADDRESS_EXT: {
        str = "VK_ERROR_INVALID_DEVICE_ADDRESS_EXT";
    } break;
    case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT: {
        str = "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
    } break;
    case VK_ERROR_UNKNOWN: {
        str = "VK_ERROR_UNKNOWN";
    } break;
    case VK_ERROR_NOT_PERMITTED_EXT: {
        str = "VK_ERROR_NOT_PERMITTED_EXT";
    } break;
    case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT: {
        str = "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
    } break;
    case VK_ERROR_VALIDATION_FAILED_EXT: {
        str = "VK_ERROR_VALIDATION_FAILED_EXT";
    } break;
    default: {}
    }
    return str;
}

const char *
vulkan_get_result_str_verbose(VkResult result) {
    const char *str = "(unknown)";
    switch (result) {
    case VK_SUCCESS: {
        str = "VK_SUCCESS Command successfully completed";
    } break;
    case VK_NOT_READY: {
        str =  "VK_NOT_READY A fence or query has not yet completed";
    } break;
    case VK_TIMEOUT: {
        str =  "VK_TIMEOUT A wait operation has not completed in the specified time";
    } break;
    case VK_EVENT_SET: {
        str =  "VK_EVENT_SET An event is signaled";
    } break;
    case VK_EVENT_RESET: {
        str =  "VK_EVENT_RESET An event is unsignaled";
    } break;
    case VK_INCOMPLETE: {
        str =  "VK_INCOMPLETE A return array was too small for the result";
    } break;
    case VK_SUBOPTIMAL_KHR: {
        str =  "VK_SUBOPTIMAL_KHR A swapchain no longer matches the surface properties exactly, but can still be used to present to the surface successfully.";
    } break;
    case VK_THREAD_IDLE_KHR: {
        str =  "VK_THREAD_IDLE_KHR A deferred operation is not complete but there is currently no work for this thread to do at the time of this call.";
    } break;
    case VK_THREAD_DONE_KHR: {
        str =  "VK_THREAD_DONE_KHR A deferred operation is not complete but there is no work remaining to assign to additional threads.";
    } break;
    case VK_OPERATION_DEFERRED_KHR: {
        str =  "VK_OPERATION_DEFERRED_KHR A deferred operation was requested and at least some of the work was deferred.";
    } break;
    case VK_OPERATION_NOT_DEFERRED_KHR: {
        str =  "VK_OPERATION_NOT_DEFERRED_KHR A deferred operation was requested and no operations were deferred.";
    } break;
    case VK_PIPELINE_COMPILE_REQUIRED_EXT: {
        str =  "VK_PIPELINE_COMPILE_REQUIRED_EXT A requested pipeline creation would have required compilation, but the application requested compilation to not be performed.";
    } break;

    // Error codes
    case VK_ERROR_OUT_OF_HOST_MEMORY: {
        str =  "VK_ERROR_OUT_OF_HOST_MEMORY A host memory allocation has failed.";
    } break;
    case VK_ERROR_OUT_OF_DEVICE_MEMORY: {
        str =  "VK_ERROR_OUT_OF_DEVICE_MEMORY A device memory allocation has failed.";
    } break;
    case VK_ERROR_INITIALIZATION_FAILED: {
        str =  "VK_ERROR_INITIALIZATION_FAILED Initialization of an object could not be completed for implementation-specific reasons.";
    } break;
    case VK_ERROR_DEVICE_LOST: {
        str =  "VK_ERROR_DEVICE_LOST The logical or physical device has been lost. See Lost Device";
    } break;
    case VK_ERROR_MEMORY_MAP_FAILED: {
        str =  "VK_ERROR_MEMORY_MAP_FAILED Mapping of a memory object has failed.";
    } break;
    case VK_ERROR_LAYER_NOT_PRESENT: {
        str =  "VK_ERROR_LAYER_NOT_PRESENT A requested layer is not present or could not be loaded.";
    } break;
    case VK_ERROR_EXTENSION_NOT_PRESENT: {
        str =  "VK_ERROR_EXTENSION_NOT_PRESENT A requested extension is not supported.";
    } break;
    case VK_ERROR_FEATURE_NOT_PRESENT: {
        str = "VK_ERROR_FEATURE_NOT_PRESENT A requested feature is not supported.";
    } break;
    case VK_ERROR_INCOMPATIBLE_DRIVER: {
        str = "VK_ERROR_INCOMPATIBLE_DRIVER The requested version of Vulkan is not supported by the driver or is otherwise incompatible for implementation-specific reasons.";
    } break;
    case VK_ERROR_TOO_MANY_OBJECTS: {
        str = "VK_ERROR_TOO_MANY_OBJECTS Too many objects of the type have already been created.";
    } break;
    case VK_ERROR_FORMAT_NOT_SUPPORTED: {
        str = "VK_ERROR_FORMAT_NOT_SUPPORTED A requested format is not supported on this device.";
    } break;
    case VK_ERROR_FRAGMENTED_POOL: {
        str =  "VK_ERROR_FRAGMENTED_POOL A pool allocation has failed due to fragmentation of the pool’s memory. This must only be returned if no attempt to allocate host or device memory was made to accommodate the new allocation. This should be returned in preference to VK_ERROR_OUT_OF_POOL_MEMORY, but only if the implementation is certain that the pool allocation failure was due to fragmentation.";
    } break;
    case VK_ERROR_SURFACE_LOST_KHR: {
        str = "VK_ERROR_SURFACE_LOST_KHR A surface is no longer available.";
    } break;
    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: {
        str =  "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR The requested window is already in use by Vulkan or another API in a manner which prevents it from being used again.";
    } break;
    case VK_ERROR_OUT_OF_DATE_KHR: {
        str =  "VK_ERROR_OUT_OF_DATE_KHR A surface has changed in such a way that it is no longer compatible with the swapchain, and further presentation requests using the swapchain will fail. Applications must query the new surface properties and recreate their swapchain if they wish to continue presenting to the surface.";
    } break;
    case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: {
        str =  "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR The display used by a swapchain does not use the same presentable image layout, or is incompatible in a way that prevents sharing an image.";
    } break;
    case VK_ERROR_INVALID_SHADER_NV: {
        str =  "VK_ERROR_INVALID_SHADER_NV One or more shaders failed to compile or link. More details are reported back to the application via VK_EXT_debug_report if enabled.";
    } break;
    case VK_ERROR_OUT_OF_POOL_MEMORY: {
        str =  "VK_ERROR_OUT_OF_POOL_MEMORY A pool memory allocation has failed. This must only be returned if no attempt to allocate host or device memory was made to accommodate the new allocation. If the failure was definitely due to fragmentation of the pool, VK_ERROR_FRAGMENTED_POOL should be returned instead.";
    } break;
    case VK_ERROR_INVALID_EXTERNAL_HANDLE: {
        str =  "VK_ERROR_INVALID_EXTERNAL_HANDLE An external handle is not a valid handle of the specified type.";
    } break;
    case VK_ERROR_FRAGMENTATION: {
        str =  "VK_ERROR_FRAGMENTATION A descriptor pool creation has failed due to fragmentation.";
    } break;
    case VK_ERROR_INVALID_DEVICE_ADDRESS_EXT: {
        str =  "VK_ERROR_INVALID_DEVICE_ADDRESS_EXT A buffer creation failed because the requested address is not available.";
    } break;
    case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT: {
        str = "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT An operation on a swapchain created with VK_FULL_SCREEN_EXCLUSIVE_APPLICATION_CONTROLLED_EXT failed as it did not have exlusive full-screen access. This may occur due to implementation-dependent reasons, outside of the application’s control.";
    } break;
    case VK_ERROR_UNKNOWN: {
        str = "VK_ERROR_UNKNOWN An unknown error has occurred; either the application has provided invalid input, or an implementation failure has occurred.";
    } break;
    case VK_ERROR_NOT_PERMITTED_EXT: {
        str = "VK_ERROR_NOT_PERMITTED_EXT";
    } break;
    case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT: {
        str = "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
    } break;
    case VK_ERROR_VALIDATION_FAILED_EXT: {
        str = "VK_ERROR_VALIDATION_FAILED_EXT";
    } break;
    default: {}
    }
    return str;
}

bool 
vulkan_result_is_success(VkResult vkresult) {
    bool result = false;
    switch (vkresult) {
    case VK_SUCCESS:
    case VK_NOT_READY:
    case VK_TIMEOUT:
    case VK_EVENT_SET:
    case VK_EVENT_RESET:
    case VK_INCOMPLETE:
    case VK_SUBOPTIMAL_KHR:
    case VK_THREAD_IDLE_KHR:
    case VK_THREAD_DONE_KHR:
    case VK_OPERATION_DEFERRED_KHR:
    case VK_OPERATION_NOT_DEFERRED_KHR:
    case VK_PIPELINE_COMPILE_REQUIRED_EXT: {
        result = true;
    } break;
    default: {}
    }
    return result;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL 
vk_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_types,
    const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
    void *user_data) {
    UNUSED(user_data);
    UNUSED(message_types);
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

Vulkan_Swapchain_Support 
get_swapchain_support(VkPhysicalDevice device, VkSurfaceKHR surface) {
    Vulkan_Swapchain_Support support = {0};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &support.capabilities);
    u32 format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, 0);
    support.formats = da_reserve(VkSurfaceFormatKHR, format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, support.formats);
    u32 present_mode_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, 0);
    support.present_modes = da_reserve(VkPresentModeKHR, present_mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, support.present_modes);
    return support;
}

void 
free_swapchain_support(Vulkan_Swapchain_Support support) {
    da_free(support.formats);
    da_free(support.present_modes);
}

Vulkan_Physical_Device 
pick_physical_device(VkInstance instance, VkSurfaceKHR surface, Vulkan_Physical_Device_Requirements *requirements) {
    Vulkan_Physical_Device result = {0};
    u32 device_count = 0;
    vkEnumeratePhysicalDevices(instance, &device_count, 0);
    VkPhysicalDevice *devices = da_reserve(VkPhysicalDevice, device_count);
    vkEnumeratePhysicalDevices(instance, &device_count, devices);
    for (u32 device_idx = 0; 
         device_idx < device_count;
         ++device_idx) {
        VkPhysicalDevice device = devices[device_idx];
        
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(device, &properties);
        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures(device, &features);
        VkPhysicalDeviceMemoryProperties memory_properties;
        vkGetPhysicalDeviceMemoryProperties(device, &memory_properties);
        
        if (requirements->flags & VULKAN_PHYSICAL_DEVICE_SAMPLER_ANISOTROPY_BIT && 
            !features.samplerAnisotropy) {
            continue;
        }
        
        u32 queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, 0);
        VkQueueFamilyProperties *queue_families = da_reserve(VkQueueFamilyProperties, queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families);
        
        u32 flags = requirements->flags;
        bool graphics_required = TO_BOOL(flags & VULKAN_PHYSICAL_DEVICE_GRAPHICS_BIT);
        bool present_required = TO_BOOL(flags & VULKAN_PHYSICAL_DEVICE_PRESENT_BIT);
        bool transfer_required = TO_BOOL(flags & VULKAN_PHYSICAL_DEVICE_TRANSFER_BIT);
        u32 graphics_idx = 0;
        u32 present_idx = 0;
        u32 transfer_idx = 0;
        bool graphics_set = false;
        bool present_set = false;
        bool transfer_set = false;
        u32 min_transfer_score = UINT32_MAX;
        for (u32 queue_family_idx = 0;
             queue_family_idx < queue_family_count;
             ++queue_family_idx) {
            VkQueueFamilyProperties queue = queue_families[queue_family_idx];
            
            u32 score = 0;
            if (graphics_required && queue.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                graphics_idx = queue_family_idx;
                graphics_set = true;
                ++score;
            }
            if (transfer_required && queue.queueFlags & VK_QUEUE_TRANSFER_BIT &&
                score < min_transfer_score) {
                min_transfer_score = score;
                transfer_idx = queue_family_idx;
                transfer_set = true;
            }
            
            if (graphics_required) {
                VkBool32 supports_present = 0;
                vkGetPhysicalDeviceSurfaceSupportKHR(device, queue_family_idx, surface, &supports_present);
                if (supports_present) {
                    present_idx = queue_family_idx;
                    present_set = true;
                }
            }
        }
        
        if (graphics_set == transfer_required && transfer_set == transfer_required && present_set == present_required) {
            Vulkan_Swapchain_Support support = get_swapchain_support(device, surface);
            if (!support.formats || !support.present_modes) {
                // @SPEED We can just not allocate if not needed...
                free_swapchain_support(support);
                continue;
            }
            
            if (requirements->extension_count) {
                u32 device_extension_count = 0;
                vkEnumerateDeviceExtensionProperties(device, 0, &device_extension_count, 0);
                VkExtensionProperties *extensions = da_reserve(VkExtensionProperties, device_extension_count);
                vkEnumerateDeviceExtensionProperties(device, 0, &device_extension_count, extensions);
                
                bool all_found = true;
                for (u32 required_idx = 0;
                     required_idx < requirements->extension_count && all_found;
                     ++required_idx) {
                    bool is_found = false;
                    for (VkExtensionProperties *extension = extensions;
                         extension < extensions + device_extension_count;
                         ++extension) {
                        if (str_eq(extension->extensionName, requirements->device_extensions[required_idx])) {
                            is_found = true;
                            break;
                        }
                    }
                    
                    all_found = is_found;
                }
                da_free(extensions);
                
                if (all_found) {
                    result.is_valid = true;
                    result.handle = device;
                    result.properties = properties;
                    result.features = features;
                    result.memory_properties = memory_properties;
                    result.graphics_family_idx = graphics_idx;
                    result.transfer_family_idx = transfer_idx;
                    result.present_family_idx = present_idx;
                    result.swapchain_support = support;
                    break;
                }
            }
        }
        
        da_free(queue_families);
    }
    da_free(devices);
    return result;    
}

void 
log_physical_device_info(Vulkan_Physical_Device *device) {
    log_info("Selected device name: %s", device->properties.deviceName);
    switch (device->properties.deviceType) {
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
        VK_VERSION_MAJOR(device->properties.driverVersion),
        VK_VERSION_MINOR(device->properties.driverVersion),
        VK_VERSION_PATCH(device->properties.driverVersion));
    log_info("Vulkan version: %d.%d.%d",
        VK_VERSION_MAJOR(device->properties.apiVersion),
        VK_VERSION_MINOR(device->properties.apiVersion),
        VK_VERSION_PATCH(device->properties.apiVersion));
        
    for (u32 j = 0; j < device->memory_properties.memoryHeapCount; ++j) {
        f32 memory_size_gib = (((f32)device->memory_properties.memoryHeaps[j].size) / 1024.0f / 1024.0f / 1024.0f);
        if (device->memory_properties.memoryHeaps[j].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
            log_info("Local GPU memory: %.2f GiB", memory_size_gib);
        } else {
            log_info("Shared System memory: %.2f GiB", memory_size_gib);
        }
    }
}

Vulkan_Device 
create_vulkan_device(VkInstance instance, VkSurfaceKHR surface, 
    Vulkan_Physical_Device *physical_device) {
    Vulkan_Device device = {0};
    
    return device;
}

static void 
create_instance(Vulkan_Ctx *ctx) {
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
    VkLayerProperties *aviable_layers = da_reserve(VkLayerProperties, avialable_layer_count);
    VK_CHECK_CALL(vkEnumerateInstanceLayerProperties, &avialable_layer_count, aviable_layers);
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
    // instance_create_info.enabledLayerCount = ARRAY_SIZE(required_layers);
    instance_create_info.ppEnabledLayerNames = required_layers;
    
    VK_CHECK_CALL(vkCreateInstance, &instance_create_info, ctx->allocator, &ctx->instance);
    da_free(aviable_layers);
}
    
static void 
setup_debug_messanger(Vulkan_Ctx *ctx) {
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
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(ctx->instance, "vkCreateDebugUtilsMessengerEXT");
    assert(vkCreateDebugUtilsMessengerEXT);
    vkCreateDebugUtilsMessengerEXT(ctx->instance, &debug_create_info, ctx->allocator, &ctx->debug_messanger);
}

// static void
// create_logical_device(Vulkan_Ctx *ctx) {
//     u32 queue_create_info_count = 0;
//     f32 priority = 1.0f;
//     VkDeviceQueueCreateInfo queue_create_infos[3];
//     {
//         VkDeviceQueueCreateInfo info = {};
//         info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
//         info.queueFamilyIndex = ctx->graphics_family_idx;
//         info.queueCount = 1;
//         info.pQueuePriorities = &priority;
//         queue_create_infos[queue_create_info_count++] = info;
//     }
//     if (ctx->graphics_family_idx != ctx->present_family_idx) {
//         VkDeviceQueueCreateInfo info = {};
//         info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
//         info.queueFamilyIndex = ctx->present_family_idx;
//         info.queueCount = 1;
//         info.pQueuePriorities = &priority;
//         queue_create_infos[queue_create_info_count++] = info;
//     }
//     if (ctx->graphics_family_idx != ctx->transfer_family_idx) {
//         VkDeviceQueueCreateInfo info = {};
//         info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
//         info.queueFamilyIndex = ctx->transfer_family_idx;
//         info.queueCount = 1;
//         info.pQueuePriorities = &priority;
//         queue_create_infos[queue_create_info_count++] = info;
//     }
    
//     VkPhysicalDeviceFeatures device_features = {};
    
//     // @TODO(hl): Check on extension aviability
//     const char *device_extensions[] = {
//         VK_KHR_SWAPCHAIN_EXTENSION_NAME,
//     };
    
//     VkDeviceCreateInfo device_create_info = {};
//     device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
//     device_create_info.pQueueCreateInfos = queue_create_infos;
//     device_create_info.queueCreateInfoCount = queue_create_info_count;
//     device_create_info.pEnabledFeatures = &device_features;
//     device_create_info.enabledExtensionCount = ARRAY_SIZE(device_extensions);
//     device_create_info.ppEnabledExtensionNames = device_extensions;
//     // device_create_info.enabledLayerCount = ARRAY_SIZE(required_layers);
//     // device_create_info.ppEnabledLayerNames = required_layers;
    
//     VK_CHECK_CALL(vkCreateDevice, ctx->physical_device, &device_create_info, 0, &ctx->logical_device);
    
//     vkGetDeviceQueue(ctx->logical_device, ctx->graphics_family_idx, 0, &ctx->graphics_queue);
//     vkGetDeviceQueue(ctx->logical_device, ctx->present_family_idx, 0, &ctx->present_queue);
//     vkGetDeviceQueue(ctx->logical_device, ctx->transfer_family_idx, 0, &ctx->transfer_queue);
// }   

// static void 
// create_swapchain(Vulkan_Ctx *ctx, u32 width, u32 height) {
//     Vulkan_Swapchain_Support swapchain_support = query_swapchain_support(ctx->physical_device, ctx->surface);
    
//     VkExtent2D swapchain_extend;
//     if (swapchain_support.capabilities.currentExtent.width == UINT32_MAX) {
//         swapchain_extend.width = width;
//         swapchain_extend.height = height;
//     } else {
//         swapchain_extend = swapchain_support.capabilities.currentExtent;
//     }
    
//     bool is_swap_surface_format_found = false;
//     for (u32 format_idx = 0;
//          format_idx < da_size(swapchain_support.formats);
//          ++format_idx) {
//         VkSurfaceFormatKHR format = swapchain_support.formats[format_idx];
//         if (format.format == VK_FORMAT_B8G8R8A8_UNORM && 
//             format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
//             ctx->sw
//         }        
//     }
// }

void
vulkan_init(Renderer *renderer, struct Window_State *window) {
    Vulkan_Ctx *ctx = mem_alloc(sizeof(Vulkan_Ctx));
    renderer->internal = ctx;
    create_instance(ctx);
    setup_debug_messanger(ctx);
    create_vulkan_surface(window, ctx);
    
    const char *physical_device_extensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    Vulkan_Physical_Device_Requirements physical_device_requirements = {0};
    physical_device_requirements.flags = VULKAN_PHYSICAL_DEVICE_GRAPHICS_BIT | 
        VULKAN_PHYSICAL_DEVICE_PRESENT_BIT |
        VULKAN_PHYSICAL_DEVICE_TRANSFER_BIT |
        VULKAN_PHYSICAL_DEVICE_SAMPLER_ANISOTROPY_BIT;
    physical_device_requirements.device_extensions = physical_device_extensions;
    physical_device_requirements.extension_count = ARRAY_SIZE(physical_device_extensions);
    ctx->physical_device = pick_physical_device(ctx->instance, ctx->surface, &physical_device_requirements);
    assert(ctx->physical_device.is_valid);
    log_physical_device_info(&ctx->physical_device);
    
    ctx->device = create_vulkan_device(ctx->instance, ctx->surface, &ctx->physical_device);
}

void 
vulkan_execute_commands(Renderer *renderer, Renderer_Commands *commands) {
    UNUSED(renderer);
    UNUSED(commands);
}
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
    // @TODO(hl): see why it is not working
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
    
static void
pick_physical_device(Vulkan_Ctx *ctx) {
    u32 device_count = 0;
    VK_CHECK_CALL(vkEnumeratePhysicalDevices, ctx->instance, &device_count, 0);
    if (!device_count) {
        log_error("Unable to find GPUs with Vulkan support");
    }
    VkPhysicalDevice *devices = da_reserve(VkPhysicalDevice, device_count);
    VK_CHECK_CALL(vkEnumeratePhysicalDevices, ctx->instance, &device_count, devices);
   
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
        VkQueueFamilyProperties *queue_families = da_reserve(VkQueueFamilyProperties, queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(test_device, &queue_family_count, queue_families);
        
        bool is_graphics_family_set = false;
        bool is_present_family_set = false;
        bool is_transfer_family_set = false;
        for (uptr j = 0; j < queue_family_count; ++j) {
            VkQueueFamilyProperties *queue_properties = queue_families + j;
            VkBool32 present_support = false;
            VK_CHECK_CALL(vkGetPhysicalDeviceSurfaceSupportKHR, test_device, j, ctx->surface, &present_support);
            if (present_support) {
                ctx->present_family_idx = j;
                is_present_family_set = true;
            }
            if (queue_properties->queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                ctx->graphics_family_idx = j;
                is_graphics_family_set = true;
            }
            if (queue_properties->queueFlags & VK_QUEUE_TRANSFER_BIT) {
                ctx->transfer_family_idx = j;
                is_transfer_family_set = true;
            }
        }
        da_free(queue_families);
        
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
            
            ctx->physical_device = test_device;
            ctx->physical_device_properties = properties;
            ctx->physical_device_features = features;
            ctx->physical_device_memory_properties = memory;
            break;
        }
    }
    da_free(devices);
    assert(ctx->physical_device);
}

static void
create_logical_device(Vulkan_Ctx *ctx) {
    u32 queue_create_info_count = 0;
    f32 priority = 1.0f;
    VkDeviceQueueCreateInfo queue_create_infos[3];
    {
        VkDeviceQueueCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        info.queueFamilyIndex = ctx->graphics_family_idx;
        info.queueCount = 1;
        info.pQueuePriorities = &priority;
        queue_create_infos[queue_create_info_count++] = info;
    }
    if (ctx->graphics_family_idx != ctx->present_family_idx) {
        VkDeviceQueueCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        info.queueFamilyIndex = ctx->present_family_idx;
        info.queueCount = 1;
        info.pQueuePriorities = &priority;
        queue_create_infos[queue_create_info_count++] = info;
    }
    if (ctx->graphics_family_idx != ctx->transfer_family_idx) {
        VkDeviceQueueCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        info.queueFamilyIndex = ctx->transfer_family_idx;
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
    
    VK_CHECK_CALL(vkCreateDevice, ctx->physical_device, &device_create_info, 0, &ctx->logical_device);
    
    vkGetDeviceQueue(ctx->logical_device, ctx->graphics_family_idx, 0, &ctx->graphics_queue);
    vkGetDeviceQueue(ctx->logical_device, ctx->present_family_idx, 0, &ctx->present_queue);
    vkGetDeviceQueue(ctx->logical_device, ctx->transfer_family_idx, 0, &ctx->transfer_queue);
}   

void
vulkan_init(Renderer *renderer, struct Window_State *window) {
    Vulkan_Ctx *ctx = mem_alloc(sizeof(Vulkan_Ctx));
    renderer->internal = ctx;
    create_instance(ctx);
    setup_debug_messanger(ctx);
    create_vulkan_surface(window, ctx);
    pick_physical_device(ctx);
    create_logical_device(ctx);
}

void 
vulkan_execute_commands(Renderer *renderer, Renderer_Commands *commands) {
    UNUSED(renderer);
    UNUSED(commands);
}
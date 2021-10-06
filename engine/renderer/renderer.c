#include "renderer/renderer.h"

#include "vulkan_renderer.h"

void 
renderer_init(Renderer *renderer, struct Window_State *window) {
    vulkan_init(renderer, window);
}

void 
renderer_execute_commands(Renderer *renderer, Renderer_Commands *commands) {
    vulkan_execute_commands(renderer, commands);
}
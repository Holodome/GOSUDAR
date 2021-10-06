/*
Author: Holodome
Date: 06.10.2021
File: engine/renderer/renderer.h
Version: 0
*/
#pragma once 
#include "lib/general.h"
#include "math/vec.h"

#define RENDERER_INDEX_TYPE u16
#define RENDERER_MAX_INDEX  MAX_VALUE(RENDERER_INDEX_TYPE)

struct Window_State;
struct Renderer_Commands_Header;
struct Renderer_Setup;
struct Renderer_Texture;

typedef struct {
    Vec3 p;
    Vec2 uv;
    Vec3 n;
    Vec4 c;
    u16 tex;
} Vertex;

typedef struct {
    Vec2 display_size;
    u64 vertex_buffer_size;
    u64 index_buffer_size;
} Renderer_Settings;

typedef struct {
    u64 command_memory_size;
    u64 command_memory_used;
    u8 *command_memory;
    
    u64 max_vertex_count;
    u64 vertex_count;
    Vertex *vertices;
    
    u64 max_index_count;
    u64 index_count;
    RENDERER_INDEX_TYPE *indices;
    
    struct Renderer_Commands_Header *last_header;
    struct Renderer_Setup *last_setup;
    
    struct Renderer_Texture *white_texture;
} Renderer_Commands;

typedef struct Renderer {
    void *internal;
    
    // Pre-allocated commands storage
    Renderer_Commands commands;
    Renderer_Settings settings;
} Renderer;

void renderer_init(Renderer *renderer, struct Window_State *window);
void renderer_execute_commands(Renderer *renderer, Renderer_Commands *commands);
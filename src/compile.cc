#include "general.hh"
#if BUILD_WITHOUT_CRT

u32 infinity_hex = 0x7F800000;
f32 F32_INFINITY = *(f32 *)&infinity_hex;

extern "C" {
    int _fltused = 0x9875;
    
#pragma function(memset)
    void *__cdecl memset(void *ptr, int value, size_t num) {
        unsigned char *dest = (unsigned char *)ptr;
        while (num--) {
            *dest++ = value;
        }
        return ptr;
    }
    
#pragma function(memcpy)
    void *__cdecl memcpy(void *dst_p, const void *src_p, size_t num) {
        unsigned char *src = (unsigned char *)src_p;
        unsigned char *dst = (unsigned char *)dst_p;
        while (num--) {
            *dst++ = *src++;
        }
        return dst_p;
    }
    
#pragma function(memcmp)
    int __cdecl memcmp(const void *ptr1, const void *ptr2, size_t num) {
        int result = 0;
        unsigned char *a = (unsigned char *)ptr1;
        unsigned char *b = (unsigned char *)ptr2;
        while (num--) {
            unsigned char av = *a++;
            unsigned char bv = *b++;
            if (av != bv) {
                result = av - bv;
                break;
            }
        }
        
        return result;
    }
}

#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"
#endif 

#include "debug.cc"
#include "mips.cc"
#include "renderer.cc"
#include "renderer_api.cc"
#include "assets.cc"
#include "audio.cc"
#include "dev_ui.cc"
#include "world.cc"
#include "sim_region.cc"
#include "world_state.cc"
#include "orders.cc"
#include "particle_system.cc"
#include "game.cc"
#include "interface.cc"
#include "main.cc"
#include "os.cc"
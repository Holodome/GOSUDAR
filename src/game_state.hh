#if !defined(GAME_WORLD_HH)

#include "lib.hh"
#include "world.hh"
#include "assets.hh"
#include "input.hh"
#include "renderer.hh"
#include "interface.hh"

enum {
    PLAYER_INTERACTION_KIND_NONE,
    // Make this separate from destroy not to overcomplicate things trying to use less names
    PLAYER_INTERACTION_KIND_MINE_RESOURCE,
    PLAYER_INTERACTION_KIND_BUILD,
    PLAYER_INTERACTION_KIND_SENTINEL  
};

struct FrameData {
    Assets *assets;
    Vec2 mouse_projection;
    SimEntity *camera_followed_entity;
    SimRegion *sim;
    Mat4x4 view;
    Mat4x4 projection;
    InputManager *input;
};  

enum {
    WORLD_OBJECT_SETTINGS_FLAG_IS_RESOURCE = 0x1,
    WORLD_OBJECT_SETTINGS_FLAG_IS_BUILDING = 0x2,
};

struct WorldObjectSettings {
    u32 resource_gain;
    u32 resource_kind;
    u32 flags;
};  

struct PlayingSound {
    AssetID sound_id;
    f64 play_cursor;
    bool is_finished;
    PlayingSound *next;
};

#define GAME_WORLD_HH 1
#endif

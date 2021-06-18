#if !defined(WORLD_HH)

#include "lib/lib.hh"

#include "game/camera.hh"

typedef u32 EntityId;

enum struct EntityKind {
    None = 0x0,
    Player,
    Tree,
};  

enum EntityFlags {
    EntityFlags_IsUpdatable = 0x1,
    EntityFlags_IsDrawable  = 0x2
};

struct Entity {
    EntityId id;
    EntityKind kind;
    u32 flags; // EntityFlags
    
    Vec2 pos;
    const char *texture_name;
    f32 health;
    u32 chops_left;
};

struct World {
    Camera camera = {};

    f32 tile_size = 2.0f;
    Vec2i map_size = Vec2i(10, 10);
    Vec3 point_on_plane = Vec3(0);

    EntityId player_id;
    EntityId camera_id;
    
    MemoryArena world_arena;
    Entity *entities;
    size_t entity_count;
    size_t max_entity_count;
    
    void get_billboard_positions(Vec3 mid_bottom, f32 width, f32 height, Vec3 out[4]);
    u32 add_entity(const Entity *entity);
    void add_tree_entity(Vec2 pos);
    void add_player_enitity();
    void get_tile_v(Vec2i coord, Vec3 out[4]);
    
    static Vec3 map_pos_to_world_pos(Vec2 map);
};  

#define WORLD_HH 1
#endif

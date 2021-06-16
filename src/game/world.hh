#if !defined(WORLD_HH)

#include "lib/lib.hh"

#include "game/camera.hh"

enum struct EntityKind {
    None = 0x0,
    Player,
    Tree
};  

enum EntityFlags {
    EntityFlags_IsUpdatable = 0x1,
    EntityFlags_IsDrawable  = 0x2
};

struct Entity {
    u32 id = (u32)-1;
    EntityKind kind = EntityKind::None;
    u32 flags = 0; // EntityFlags
    
    Vec2 pos;
    Str texture_name;
};

struct World {
    Camera camera = {};

    f32 tile_size = 2.0f;
    Vec2i map_size = Vec2i(10, 10);
    Vec3 point_on_plane = Vec3(0);

    Array<Entity> entities;
    u32 player_entity_id;
    
    void init();
    void cleanup();
    
    void update();
    void render();
    
    void get_billboard_positions(Vec3 mid_bottom, f32 width, f32 height, Vec3 out[4]);
    u32 add_entity(const Entity *entity);
    void add_tree_entity(Vec2 pos);
    void add_player_enitity();
};  

#define WORLD_HH 1
#endif

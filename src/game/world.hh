#if !defined(WORLD_HH)

#include "lib/lib.hh"

struct Tile {
    bool is_walkable;    
};

struct World {
    Vec2i size;
    Array<Tile> tiles;
    
    World(Vec2i size);
    ~World();
    
    u32 tile_index(Vec2i pos);
};  

#define WORLD_HH 1
#endif

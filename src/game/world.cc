#include "game/world.hh"


World(Vec2i size) 
    : size(size), tiles(size.x * size.y) {
    
}
~World() {
    
}
    
u32 tile_index(Vec2i pos) {
    return pos.y * size.y + pos.x;
}
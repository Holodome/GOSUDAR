#if !defined(GAME_STATE_HH)

#include "lib/lib.hh"

#include "game/world.hh"

struct SpriteDescription {
    Str name;
    Str filename;
    Vec2i image_size;    
};

struct GameObjectDescription {
    Str name;
    Str sprite_name;
    Vec2 sprite_size; // Game units
};  

struct Settings {
    bool fullscreen = false;
    bool enable_devui = false; // Press f3 to enable, f8 to focus mouse 
    bool focus_devui = true;
};  

struct GameState {
    DevUI local_dev_ui = {};
    
    Settings settings = {};
    World world = {};
    
    void init();
    void cleanup();
    
    void update();
    
    void update_logic();
    void render();
};

#define GAME_STATE_HH 1
#endif

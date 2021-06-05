#if !defined(GAME_STATE_HH)

#include "lib/lib.hh"

enum struct GameStateKind {
    None = 0x0,
    Game,
    Menu
};

struct Camera {
    Vec3 pos = Vec3(0); 
    Vec3 rot = Vec3(0);
    Mat4x4 view = Mat4x4::identity();
    Mat4x4 projection = Mat4x4::identity();
};  

struct GameState {
    GameStateKind state_kind;
    bool enable_devui = false; // Press f3 to enable, f8 to focus mouse 
    bool focus_devui = false;
    
    Camera camera;

    Mesh *cube = 0;
    Mesh *rect = 0;
    Mesh *map  = 0;
    

    void init();
    void cleanup();
    
    void update();
    // private
    void update_camera();
    void update_input();
    void render();
};

#define GAME_STATE_HH 1
#endif

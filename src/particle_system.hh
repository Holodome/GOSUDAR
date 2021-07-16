//
// Particle system defines interface for interacting with particles
// In its core particle system has simple function - render lots of similar particles
// and update them during the game
// Creating particles starts with defining emitter.
// Emitter describes of what type particles are created - how they are updated,
// how they are drawed and in what quantity
// Emitters can be accessed by id, that can be used in-game to interface with particles
// For example, when fire starts, we create particle system and store its id in burning entity
// and then when entity finished burning, stored id is used to delete emitter
// 
// Particles are represented as number of data fields. They define equation of motion,
// how color is changed and how much energy particle has. Energy is decreased with time,
// and when goes below zero particle is destroyed
// There was possibility to unite some fields - like color can be determined with energy or 
// change of energy can be determined with emitter. 
// But having each of these values vary can help to produce complex particle systems 
// with less limitations
//
// Each individual emitter can have set number of particles at same time
// Particle system is responsible for recycling particle storage - when one's 
// energy is finished, we need to reuse this particle's storage slot
//
// Emitters should be stored in way that supports fast deletion by id
// Hash table would seem too complex choice as data structure - since number of emiiters is going to 
// be quite small for now
// So we can just have linked list of emitters that is cycled through each time emitter is deleted
// 
// Particle system can utilize SIMD for a lot of its functionality during updating
// 
// Particles in system have its own coordinate system that is different from camera one's
// when it is changed. 
// So each frame camera position should be accounted for when calculating new position
// It seems like subtracting camera delta of last frame will do the job
//
// @TODO there is complex case for particle system that will not be accounted for some time
// Particles should preferable have collision detection. Due to the nature of the game
// it can be detection of collisions with ground, but who knows what will be next
// For now we can just delete particle if its y goes below zero
#if !defined(PARTICLE_SYSTEM_HH)

#include "lib.hh"

#define MAX_PARTICLES_PER_EMITTER 4096

enum { 
    PARTICLE_EMITTER_KIND_NONE,
    PARTICLE_EMITTER_KIND_FOUNTAIN,
};

struct ParticleEmtitterSpec {
    u32 kind;
    // In what range settings of particles are uniformly choosed
    // Settings for position values contain length of vector since direction 
    // is determined by emitter itself
    f32 ddp_range_low;
    f32 ddp_range_high;
    f32 dp_range_low;
    f32 dp_range_high;
    // @TODO it is not clear how colors should be chosed - 
    // should there each individual component by chosen randomly or all color lerped uniformly
    Vec4 c_range_low;
    Vec4 c_range_high;
    Vec4 dc_range_low;
    Vec4 dc_range_high;
    f32 e_range_low;
    f32 e_range_high;
    f32 de_range_low;
    f32 de_range_high;
    
};

struct Particle {
    Vec3 p;
    Vec3 dp;
    Vec3 ddp;
    Vec4 c;
    Vec4 dc;
    f32 e;
    f32 de;
};

struct ParticleEmitter {
    u32 particle_count;
    Particle particles[MAX_PARTICLES_PER_EMITTER];
    
    ParticleEmitter *next;
};

struct ParticleSystem {
    MemoryArena *arena;
    Entropy entropy;
    u32 max_emitter_id;
    
    ParticleEmitter *emitter_list;
    
    ParticleEmitter *first_free_emitter;
};

void init_particle_system(ParticleSystem *sys, MemoryArena *arena);
ParticleEmitterID add_particle_emitter(ParticleSystem *sys, u32 kind, Vec4 color);
void delete_particle_emitter(ParticleSystem *sys, ParticleEmitterID id);

#define PARTICLE_SYSTEM_HH 1
#endif 
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
//
// It was decided that particle systems can have different number of particles depending on their size
// each use case knows size in advance, so it will not be a problem
// However, to avoid using dynamic size list updating of any kind
// all particles will be updated each frame, even ones that don't exist
// they won't be rendered however
#if !defined(PARTICLE_SYSTEM_HH)

#include "lib.hh"

#define MAX_PARTICLES_PER_EMITTER 256

enum {
    PARTICLE_SYSTEM_SIZE_SMALL,
    PARTICLE_SYSTEM_SIZE_MEDIUM,
    PARTICLE_SYSTEM_SIZE_BIG,
};

enum { 
    PARTICLE_EMITTER_KIND_NONE,
    PARTICLE_EMITTER_KIND_FOUNTAIN,
};

struct ParticleEmitterSpec {
    u32 kind;
    f32 spawn_rate;
    vec3 p;
};

struct Particle_4x {
    vec3_4x p;
    f32_4x e;
};

struct ParticleEmitter {
    ParticleEmitterSpec spec;
    f32 spawn_cursor;
    u32 particle_cursor;
    Particle_4x particles[MAX_PARTICLES_PER_EMITTER / 4];
    
    ParticleEmitter *next;
};

struct ParticleSystem {
    MemoryArena *arena;
    Entropy entropy;
    u32 max_emitter_id;
    
    ParticleEmitter emitter;
    //ParticleEmitter *emitter_list;
    
    ParticleEmitter *first_free_emitter;
};

void init_particle_system(ParticleSystem *sys, MemoryArena *arena);
// ParticleEmitterID add_particle_emitter(ParticleSystem *sys, u32 kind, vec4 color);
// void delete_particle_emitter(ParticleSystem *sys, ParticleEmitterID id);
void update_and_render_particles(ParticleSystem *sys, RenderGroup *render_group, f32 dt);

#define PARTICLE_SYSTEM_HH 1
#endif 
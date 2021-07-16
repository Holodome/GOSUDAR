#include "particle_system.hh"


void init_particle_system(ParticleSystem *sys, MemoryArena *arena) {
    sys->arena = arena;
    sys->entropy = { 987654321 };
}

ParticleEmitterID add_particle_emitter(ParticleSystem *sys, u32 kind, Vec4 color) {
    
}

void delete_particle_emitter(ParticleSystem *sys, ParticleEmitterID id) {
    
}

#include "particle_system.hh"


void init_particle_system(ParticleSystem *sys, MemoryArena *arena) {
    sys->arena = arena;
    sys->entropy = { 987654321 };
}


void update_and_render_particles(ParticleSystem *sys, RenderGroup *render_group, f32 dt) {
    TIMED_FUNCTION();
    ParticleEmitter *emitter = &sys->emitter;
    // First, create new particles 
    f32 delta_spawn = emitter->spec.spawn_rate * dt;
    emitter->spawn_cursor += delta_spawn;
    u32 to_create = (u32)emitter->spawn_cursor;
    emitter->spawn_cursor -= to_create;
    while (to_create--) {
        u32 cursor = emitter->particle_cursor;
        emitter->particle_cursor = INC_MODULO(emitter->particle_cursor, ARRAY_SIZE(emitter->particles));
        Particle_4x *dest = emitter->particles + cursor;
        dest->p = Vec3_4x(emitter->spec.p);
        dest->e = F32_4x(4.0f);
    }
    
    vec3 cam_x = GetX(render_group->commands->last_setup->mvp);
    vec3 cam_y = GetY(render_group->commands->last_setup->mvp);
    f32_4x lower_e = F32_4x(0);
    f32_4x de = F32_4x(dt);
    vec3_4x dp = Vec3_4x(Vec3(0, 1.0, 0) * dt);
    u32 DEBUG_number_drawed = 0;
    for (size_t particle4_idx = 0; particle4_idx < ARRAY_SIZE(emitter->particles); ++particle4_idx) {
        Particle_4x *particle = emitter->particles + particle4_idx;
        if (any_true(particle->e > lower_e)) {
            particle->e -= de;
            particle->p += dp;
            
            for (u32 local_idx = 0; local_idx < 4; ++local_idx) {
                vec3 p = GetComponent(particle->p, local_idx);
                f32 e = particle->e.e[local_idx];
                
                if (e > 0.0) {
                    vec3 billboard[4];
                    get_billboard_positions(p, cam_x, cam_y, 0.05, 0.05, billboard);
                    push_quad(render_group, billboard[0], billboard[1], billboard[2], billboard[3], RED);
                    ++DEBUG_number_drawed;
                }
            }
        }
    }
    DEBUG_VALUE(DEBUG_number_drawed, "Particles drawed");
}
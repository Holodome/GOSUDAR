#ifdef VERTEX_SHADER

layout(location = 0) in vec4 position;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 uv;
layout(location = 3) in int texture_index;
    
out vec4 rect_color;
out vec2 frag_uv;
    
flat out int frag_texture_index;
uniform mat4 mvp_matrix = mat4(1);

void main() {
    vec4 world_space = position;
    vec4 clip_space = mvp_matrix * world_space;
    gl_Position = clip_space;
    
    rect_color = color;
    frag_uv = uv;
    frag_texture_index = texture_index;
}

#endif
#ifdef FRAGMENT_SHADER

in vec4 rect_color;
in vec2 frag_uv;
flat in int frag_texture_index;
uniform sampler2DArray texture_sampler;
out vec4 out_color;

void main() {
    vec3 array_uv = vec3(frag_uv.x, frag_uv.y, frag_texture_index);
    vec4 texture_sample = texture(texture_sampler, array_uv);
    out_color = texture_sample * rect_color;
}

#endif     
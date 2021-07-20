R"FOO(#ifdef VERTEX_SHADER       
layout(location = 0) in vec4 position;     
layout(location = 1) in vec2 uv;       
layout(location = 2) in vec3 n;        
layout(location = 3) in vec4 color;        
layout(location = 4) in int texture_index;     
       
out vec4 rect_color;       
out vec2 frag_uv;      
       
flat out int frag_texture_index;       

uniform mat4 view_matrix = mat4(1);     
uniform mat4 projection_matrix = mat4(1);

void main() {             
    vec4 world_space = position;       
    vec4 cam_space = view_matrix * world_space;
    vec4 clip_space = projection_matrix * cam_space;        
    gl_Position = clip_space;      
       
    rect_color = color;        
    frag_uv = uv;      
    frag_texture_index = texture_index;        
}      
#else 

in vec4 rect_color;        
in vec2 frag_uv;       
flat in int frag_texture_index;        

#if DEPTH_PEEL
uniform sampler2D depth;
#endif 
uniform sampler2DArray tex;        
out vec4 out_color;        

void main()        
{      
#if DEPTH_PEEL
    float clip_depth = texelFetch(depth, ivec2(gl_FragCoord.xy), 0).x;
    float frag_z = gl_FragCoord.z;
    if (frag_z <= clip_depth) {
        discard;
    }
#endif 
    
    vec3 array_uv = vec3(frag_uv.x, frag_uv.y, frag_texture_index);        
    vec4 texture_sample = texture(tex, array_uv, 0);      

    if (texture_sample.a > 0) {
        out_color = texture_sample * rect_color;       
    } else {
        discard;
    }

}    
  
#endif)FOO"
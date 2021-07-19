R"FOO(#ifdef VERTEX_SHADER
layout(location = 0) in vec2 position;
        
        out vec2 frag_uv;
        
        void main() {
            gl_Position = vec4(position.x, position.y, 0.0, 1.0);
            frag_uv = (position + vec2(1)) * 0.5;
        }
#else 

uniform sampler2D peel0_tex;
uniform sampler2D peel1_tex;

in vec2 frag_uv;
out vec4 out_color;

void main() {
vec4 peel0 = texture(peel0_tex, frag_uv);
vec4 peel1 = texture(peel1_tex, frag_uv);
out_color.rgb = peel0.rgb + (1.0 - peel0.a) * peel1.rgb;

}
#endif)FOO"
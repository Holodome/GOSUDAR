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
uniform sampler2D peel2_tex;
uniform sampler2D peel3_tex;

in vec2 frag_uv;
out vec4 out_color;

void main() {
vec4 peel0 = texture(peel0_tex, frag_uv);
vec4 peel1 = texture(peel1_tex, frag_uv);
vec4 peel2 = texture(peel2_tex, frag_uv);
vec4 peel3 = texture(peel3_tex, frag_uv);

peel0.rgb *= peel0.a;
peel1.rgb *= peel1.a;
peel2.rgb *= peel2.a;
peel3.rgb *= peel3.a;

out_color.rgb = peel3.rgb;
out_color.rgb = peel2.rgb + (1 - peel2.a) * out_color.rgb;
out_color.rgb = peel1.rgb + (1 - peel1.a) * out_color.rgb;
out_color.rgb = peel0.rgb + (1 - peel0.a) * out_color.rgb;
}
#endif)FOO"
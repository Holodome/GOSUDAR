R"FOO(#ifdef VERTEX_SHADER
layout(location = 0) in vec2 position;

out vec2 frag_uv;

void main() {
    gl_Position = vec4(position.x, position.y, 0.0, 1.0);
    frag_uv = (position + vec2(1)) * 0.5;
}
#else     

in vec2 frag_uv;
out vec4 out_color;

uniform sampler2D tex;

void main() {
    out_color = texture(tex, frag_uv);
}

#endif)FOO"
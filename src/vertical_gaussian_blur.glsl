R"FOO(#ifdef VERTEX_SHADER
layout(location = 0) in vec2 position;

out vec2 blur_uvs[11];
uniform float target_height;

void main() {
    gl_Position = vec4(position.x, position.y, 0.0, 1.0);
    vec2 center_uv = (position + vec2(1)) * 0.5;
    float px_size = 1.0 / target_height;
    for (int i = -5; i <= 5; ++i) {
        blur_uvs[i + 5] = center_uv + vec2(0, px_size * i);
    }
}

#else 

out vec4 out_color;
in vec2 blur_uvs[11];

uniform sampler2D tex;

void main() {
    out_color = vec4(0);
    out_color += texture(tex, blur_uvs[0]) * 0.0093;
    out_color += texture(tex, blur_uvs[1]) * 0.028002;
    out_color += texture(tex, blur_uvs[2]) * 0.065984;
    out_color += texture(tex, blur_uvs[3]) * 0.121703;
    out_color += texture(tex, blur_uvs[4]) * 0.175713;
    out_color += texture(tex, blur_uvs[5]) * 0.198596;
    out_color += texture(tex, blur_uvs[6]) * 0.175713;
    out_color += texture(tex, blur_uvs[7]) * 0.121703;
    out_color += texture(tex, blur_uvs[8]) * 0.065984;
    out_color += texture(tex, blur_uvs[9]) * 0.028002;
    out_color += texture(tex, blur_uvs[10]) * 0.0093;
}

#endif)FOO"
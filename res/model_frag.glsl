
#version 130

in vec2 f_uv;
in vec4 f_weights;

uniform sampler2D u_texture;

out vec4 o_color;

void main() {
    o_color = texture2D(u_texture, f_uv) * 0.0001;
    o_color += vec4(f_weights.xyz, 1.0);
}
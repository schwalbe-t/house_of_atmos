
#version 330

in vec2 f_uv;

uniform vec4 u_base_color;

out vec4 o_color;

void main() {
    o_color = u_base_color;
}


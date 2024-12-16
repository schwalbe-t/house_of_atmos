
#version 130

in vec3 f_color;

out vec4 o_color;

void main() {
    o_color = vec4(f_color, 1.0);
}
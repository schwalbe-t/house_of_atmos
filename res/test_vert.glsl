
#version 130

in vec3 v_pos;
in vec3 v_color;

out vec3 f_color;

void main() {
    gl_Position = vec4(v_pos, 1.0);
    f_color = v_color;
}

#version 130

in vec2 v_pos;
in uvec3 v_color;

out vec3 f_color;

void main() {
    gl_Position = vec4(v_pos, 0.0, 1.0);
    f_color = vec3(v_color.x, v_color.y, v_color.z);
}
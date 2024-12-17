
#version 130

in vec3 v_pos;
//in vec2 v_uv;
//in vec3 v_norm;

uniform mat4 u_projection;
uniform mat4 u_view;
uniform mat4 u_model;

//out vec2 f_uv;

void main() {
    gl_Position = u_projection * u_view * u_model * vec4(v_pos, 1.0);
    //f_uv = v_uv;
}
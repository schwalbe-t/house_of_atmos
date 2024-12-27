
#version 330

layout(location = 0) in vec3 v_pos;
layout(location = 1) in vec2 v_uv;

uniform mat4 u_view_projection;
uniform mat4 u_model_transf;

out vec2 f_uv;

void main() {
    gl_Position = u_view_projection * u_model_transf * vec4(v_pos, 1.0);
    f_uv = v_uv;
}
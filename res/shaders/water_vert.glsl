
#version 330

layout(location = 0) in vec3 v_pos;
layout(location = 1) in vec2 v_uv;

uniform mat4 u_view_projection;
uniform mat4 u_model_transf;
uniform mat4 u_local_transf;
uniform float u_time;

out vec2 f_uv;
out vec3 f_w_pos;

const float pi = 3.14159265;

const float wave_time = 10.0;
const vec3 wave_force = vec3(0, 0.05, 0);

void main() {
    vec3 pos = v_pos + wave_force * sin(u_time * 2 * pi / wave_time);
    vec4 w_pos = u_model_transf * u_local_transf * vec4(pos, 1);
    gl_Position = u_view_projection * w_pos;
    f_uv = v_uv;
    f_w_pos = w_pos.xyz;
}
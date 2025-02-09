
#version 330

#include "common/consts.glsl"

layout(location = 0) in vec3 v_pos;

uniform mat4 u_view_projection;
uniform mat4 u_model_transf;
uniform mat4 u_local_transf;
uniform float u_time;

out vec3 f_w_pos;

const float wave_time = 10.0;
const vec3 wave_force = vec3(0, 0.05, 0);

void main() {
    vec3 wave_offset = wave_force * sin(u_time * 2 * PI / wave_time);
    vec4 w_pos = u_model_transf * u_local_transf * vec4(v_pos, 1.0)
        + vec4(wave_offset, 0.0);
    gl_Position = u_view_projection * w_pos;
    f_w_pos = w_pos.xyz;
}
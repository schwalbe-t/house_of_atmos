
#version 330

#include "common/util.glsl"

layout(location = 0) in vec3 v_pos;
layout(location = 1) in vec2 v_uv;
layout(location = 2) in vec3 v_norm;
layout(location = 3) in uvec4 v_joints;
layout(location = 4) in vec4 v_weights;

uniform mat4 u_view_proj;
uniform mat4 u_transform;

out vec4 f_color;

const vec3 VALID_NORM = vec3(0, 1, 0);
const vec3 VALID_COLOR = COLOR_24(192, 199, 65);
const vec3 ERROR_COLOR = COLOR_24(157, 48, 59);

void main() {
    gl_Position = u_view_proj * u_transform * vec4(v_pos, 1.0);
    float diffuse = max(dot(VALID_NORM, normalize(v_norm)), 0.0);
    float is_error = step(0.01, 1.0 - diffuse);
    float is_valid = 1.0 - is_error;
    vec3 color = VALID_COLOR * is_valid + ERROR_COLOR * is_error;
    f_color = vec4(color, 0.1);
}
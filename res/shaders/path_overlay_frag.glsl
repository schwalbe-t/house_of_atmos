
#version 300 es
precision highp float;

#include "common/util.glsl"

in vec3 f_w_pos;

uniform float u_tile_size;

out vec4 o_color;

const vec3 BORDER_COLOR = COLOR_24(239, 239, 230);
const float BORDER_WIDTH = 0.1;

void main() {
    vec2 t_pos = f_w_pos.xz / u_tile_size;
    vec2 b_dist = abs(round(t_pos) - t_pos);
    float m_dist = min(b_dist.x, b_dist.y);
    o_color = step(m_dist, BORDER_WIDTH) * vec4(BORDER_COLOR, 1.0);
}


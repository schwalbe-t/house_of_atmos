
#version 330

#include "common/consts.glsl"

in vec2 f_uv;

uniform sampler2D u_texture;
uniform int u_texture_w;
uniform int u_texture_h;
uniform int u_blur_rad;

out vec4 o_color;

void main() {
    vec2 px = vec2(1.0 / float(u_texture_w), 1.0 / float(u_texture_h));
    float sigma = max(float(u_blur_rad) / 2.0, 1.0);
    int kernel_w = (u_blur_rad * 2) + 1;
    o_color = vec4(0.0, 0.0, 0.0, 1.0);
    for(int ox = -u_blur_rad; ox <= u_blur_rad; ox += 1) {
        for(int oy = -u_blur_rad; oy <= u_blur_rad; oy += 1) {
            vec2 point_uv = f_uv + px * vec2(float(ox), float(oy));
            point_uv.x = min(max(point_uv.x, 0.0), 1.0 - px.x);
            point_uv.y = min(max(point_uv.y, 0.0), 1.0 - px.y);
            float exponent = float(-(ox * ox + oy * oy)) / (2.0 * sigma * sigma);
            float kernel_val = pow(E, exponent) / (2.0 * PI * sigma * sigma);
            o_color += texture2D(u_texture, point_uv) * kernel_val;
        }
    }
}
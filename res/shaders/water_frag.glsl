
#version 330

#include "common/palettes.glsl"
#include "common/fog.glsl"
#include "common/shadows.glsl"

in vec3 f_w_pos;

uniform vec4 u_light_color;
uniform vec4 u_base_color;
uniform vec4 u_dark_color;
uniform sampler2D u_nmap;
uniform float u_nmap_scale;
uniform float u_time;

out vec4 o_color;

const float LIGHT_MIN_Y = 0.65;
const float DARK_MAX_Y = 0.075;

const vec2 NMAP_A_SCROLL = vec2(0.01, 0.0);
const vec2 NMAP_B_SCROLL = vec2(0.005, 0.02);

void main() {
    vec2 nmap_a_uv = f_w_pos.xz * u_nmap_scale + NMAP_A_SCROLL * u_time;
    vec2 nmap_b_uv = f_w_pos.xz * u_nmap_scale + NMAP_B_SCROLL * u_time;
    vec3 normal = texture(u_nmap, nmap_a_uv).rgb * texture(u_nmap, nmap_b_uv).rgb;
    bool in_shadow = is_in_shadow(f_w_pos);
    o_color = u_base_color;
    if(normal.y > LIGHT_MIN_Y && !in_shadow) { o_color = u_light_color; }
    if(normal.y < DARK_MAX_Y) { o_color = u_dark_color; }
    if(in_shadow) {
        o_color = vec4(
            map_palette(o_color.rgb, palette, shadow_palette), 
            o_color.a
        );
    }
    o_color = blend_fog(o_color, f_w_pos);
}


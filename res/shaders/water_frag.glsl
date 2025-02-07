
#version 330

#include "common/palettes.glsl"
#include "common/fog.glsl"
#include "common/shadows.glsl"

in vec2 f_uv;
in vec3 f_w_pos;

uniform vec4 u_light_color;
uniform vec4 u_base_color;
uniform vec4 u_dark_color;
uniform sampler2D u_nmap;
uniform float u_nmap_scale;
uniform vec2 u_nmap_offset;
uniform float u_time;

out vec4 o_color;

const float light_min_y = 0.65;
const float dark_max_y = 0.075;

const vec2 nmap_a_scroll = vec2(0.01, 0.0);
const vec2 nmap_b_scroll = vec2(0.005, 0.02);

void main() {
    vec2 nmap_a_uv = f_uv * u_nmap_scale + nmap_a_scroll * u_time + u_nmap_offset;
    vec2 nmap_b_uv = f_uv * u_nmap_scale + nmap_b_scroll * u_time + u_nmap_offset;
    vec3 normal = texture2D(u_nmap, nmap_a_uv).rgb * texture2D(u_nmap, nmap_b_uv).rgb;
    bool in_shadow = is_in_shadow(f_w_pos);
    o_color = u_base_color;
    if(normal.y > light_min_y && !in_shadow) { o_color = u_light_color; }
    if(normal.y < dark_max_y) { o_color = u_dark_color; }
    if(in_shadow) {
        o_color = vec4(
            map_palette(o_color.rgb, palette, shadow_palette), 
            o_color.a
        );
    }
    o_color = blend_fog(o_color, f_w_pos);
}



#version 330

in vec2 f_uv;
in vec3 f_w_pos;

uniform vec4 u_light_color;
uniform vec4 u_base_color;
uniform vec4 u_dark_color;
uniform sampler2D u_nmap;
uniform float u_nmap_scale;
uniform vec2 u_nmap_offset;
uniform float u_time;
uniform float u_fog_start_dist;
uniform float u_fog_gradiant_range;
uniform vec3 u_fog_origin;
uniform vec3 u_fog_dist_scale;
uniform vec4 u_fog_color;

out vec4 o_color;

const float light_min_y = 0.65;
const float dark_max_y = 0.075;

const vec2 nmap_a_scroll = vec2(0.01, 0.0);
const vec2 nmap_b_scroll = vec2(0.005, 0.02);

void main() {
    // compute actual surface color
    vec2 nmap_a_uv = f_uv * u_nmap_scale + nmap_a_scroll * u_time + u_nmap_offset;
    vec2 nmap_b_uv = f_uv * u_nmap_scale + nmap_b_scroll * u_time + u_nmap_offset;
    vec3 normal = texture2D(u_nmap, nmap_a_uv).rgb * texture2D(u_nmap, nmap_b_uv).rgb;
    o_color = u_base_color;
    if(normal.y > light_min_y) { o_color = u_light_color; }
    if(normal.y < dark_max_y) { o_color = u_dark_color; }
    // apply fog
    float fog_distance = length((f_w_pos - u_fog_origin) * u_fog_dist_scale);
    float fog_strength = clamp(
        (fog_distance - u_fog_start_dist) / u_fog_gradiant_range,
        0.0, 1.0
    );
    o_color = mix(o_color, u_fog_color, fog_strength);
}


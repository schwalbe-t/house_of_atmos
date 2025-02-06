
uniform float u_fog_start_dist;
uniform float u_fog_gradiant_range;
uniform vec3 u_fog_origin;
uniform vec3 u_fog_dist_scale;
uniform vec4 u_fog_color;

vec4 blend_fog(vec4 color, vec3 world_pos) {
    float fog_distance = length((world_pos - u_fog_origin) * u_fog_dist_scale);
    float fog_strength = clamp(
        (fog_distance - u_fog_start_dist) / u_fog_gradiant_range,
        0.0, 1.0
    );
    return mix(color, u_fog_color, fog_strength);
}

#include "packing.glsl"

const int MAX_LIGHT_COUNT = 16;

uniform int u_light_count;
uniform mat4 u_light_view_proj[MAX_LIGHT_COUNT];
uniform sampler2DArray u_shadow_maps;
uniform float u_shadow_bias;

bool is_in_shadow(vec3 world_pos) {
    for(int i = 0; i < u_light_count; i += 1) {
        vec4 l_clip_pos = u_light_view_proj[i] * vec4(world_pos, 1.0);
        vec3 l_ndc = l_clip_pos.xyz / l_clip_pos.w;
        vec2 shadow_uv = l_ndc.xy * 0.5 + 0.5;
        float frag_depth = l_ndc.z * 0.5 + 0.5;
        bool in_bounds = shadow_uv.x >= 0.0 && shadow_uv.x <= 1.0
            && shadow_uv.y >= 0.0 && shadow_uv.y <= 1.0;
        if(!in_bounds && u_light_count == 1) { return false; }
        if(!in_bounds) { continue; }
        vec3 packed_shadow_depth = texture(u_shadow_maps, vec3(shadow_uv, i)).rgb;
        float shadow_depth = unpack_value(packed_shadow_depth);
        if(frag_depth - u_shadow_bias <= shadow_depth) {
            return false;
        }
    }
    return true;
}
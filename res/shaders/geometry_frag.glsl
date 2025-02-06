
#version 330

#define NORM_COLOR(r, g, b) (vec3((r), (g), (b)) / 255.0)

const int PALETTE_SIZE = 19;
const vec3 palette[PALETTE_SIZE] = vec3[](
    NORM_COLOR(140, 143, 174),
    NORM_COLOR( 88,  69,  99),
    NORM_COLOR( 62,  33,  55),
    NORM_COLOR( 94,  64,  53),
    NORM_COLOR(154,  99,  72),
    NORM_COLOR(215, 155, 125),
    NORM_COLOR(255, 202, 168),
    NORM_COLOR(245, 237, 186),
    NORM_COLOR(192, 199,  65),
    NORM_COLOR(100, 125,  52),
    NORM_COLOR(228, 148,  58),
    NORM_COLOR(157,  48,  59),
    NORM_COLOR(210, 100, 113),
    NORM_COLOR(112,  55, 127),
    NORM_COLOR(126, 196, 193),
    NORM_COLOR( 52, 133, 157),
    NORM_COLOR( 23,  67,  75),
    NORM_COLOR( 31,  14,  28),
    NORM_COLOR(239, 239, 230)
);
const vec3 shadow_palette[PALETTE_SIZE] = vec3[](
    NORM_COLOR( 88,  69,  99),
    NORM_COLOR( 62,  33,  55),
    NORM_COLOR( 31,  14,  28),
    NORM_COLOR( 62,  33,  55),
    NORM_COLOR( 94,  64,  53),
    NORM_COLOR(154,  99,  72),
    NORM_COLOR(215, 155, 125),
    NORM_COLOR(255, 202, 168),
    NORM_COLOR(100, 125,  52),
    NORM_COLOR( 23,  67,  75),
    NORM_COLOR(154,  99,  72),
    NORM_COLOR( 62,  33,  55),
    NORM_COLOR(157,  48,  59),
    NORM_COLOR( 62,  33,  55),
    NORM_COLOR( 52, 133, 157),
    NORM_COLOR( 23,  67,  75),
    NORM_COLOR( 62,  33,  55),
    NORM_COLOR( 31,  14,  28),
    NORM_COLOR(140, 143, 174)
);
const vec3 no_mapping_color = vec3(1.0, 0.0, 1.0);

vec3 map_palette(
    vec3 color, vec3 in_palette[PALETTE_SIZE], vec3 out_palette[PALETTE_SIZE]
) {
    vec3 mapped_color = vec3(0.0, 0.0, 0.0);
    float found_match = 0.0;
    for(int i = 0; i < PALETTE_SIZE; i += 1) {
        float is_match = 1.0 - step(0.01, distance(color, in_palette[i]));
        mapped_color += is_match * out_palette[i];
        found_match += is_match;
    }
    found_match = step(0.0, found_match);
    return mix(no_mapping_color, mapped_color, found_match);
}


in vec2 f_uv;
in vec3 f_w_pos;

const int MAX_LIGHT_COUNT = 16;
uniform int u_light_count;
uniform mat4 u_light_view_proj[MAX_LIGHT_COUNT];
uniform sampler2DArray u_shadow_maps;
uniform sampler2D u_texture;
uniform float u_fog_start_dist;
uniform float u_fog_gradiant_range;
uniform vec3 u_fog_origin;
uniform vec3 u_fog_dist_scale;
uniform vec4 u_fog_color;

out vec4 o_color;

float decode_depth(vec3 packed_depth) {
    int r = int(round(packed_depth.r * 255.0));
    int g = int(round(packed_depth.g * 255.0));
    int b = int(round(packed_depth.b * 255.0));
    int scaled = (r << 16) | (g << 8) | b;
    return float(scaled) / 16777215.0;
}

const float shadow_bias = 0.0002;

bool is_in_shadow() {
    for(int i = 0; i < u_light_count; i += 1) {
        vec4 l_clip_pos = u_light_view_proj[i] * vec4(f_w_pos, 1.0);
        vec3 l_ndc = l_clip_pos.xyz / l_clip_pos.w;
        vec2 shadow_uv = l_ndc.xy * 0.5 + 0.5;
        float frag_depth = l_ndc.z * 0.5 + 0.5;
        bool in_bounds = shadow_uv.x >= 0.0 && shadow_uv.x <= 1.0
            && shadow_uv.y >= 0.0 && shadow_uv.y <= 1.0;
        if(!in_bounds) { continue; }
        vec3 packed_shadow_depth = texture(u_shadow_maps, vec3(shadow_uv, i)).rgb;
        float shadow_depth = decode_depth(packed_shadow_depth);
        if(frag_depth - shadow_bias > shadow_depth) {
            return true;
        }
    }
    return false;
}

void main() {
    o_color = texture2D(u_texture, f_uv);
    if(o_color.w == 0) { discard; } // don't render the pixel if alpha = 0
    if(is_in_shadow()) {
        o_color = vec4(map_palette(o_color.rgb, palette, shadow_palette), o_color.a);
    }
    float fog_distance = length((f_w_pos - u_fog_origin) * u_fog_dist_scale);
    float fog_strength = clamp(
        (fog_distance - u_fog_start_dist) / u_fog_gradiant_range,
        0.0, 1.0
    );
    o_color = mix(o_color, u_fog_color, fog_strength);
}


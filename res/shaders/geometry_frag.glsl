
#version 300 es
precision highp float;

#include "common/palettes.glsl"
#include "common/shadows.glsl"
#include "common/diffuse.glsl"
#include "common/fog.glsl"

in vec2 f_uv;
in vec3 f_w_pos;
in vec3 f_norm;

uniform sampler2D u_texture;
uniform sampler2D u_dither_pattern;

out vec4 o_color;

const float DITHER_PATTERNS = 7.0;
const float DITHER_PAT_UVW = 1.0 / DITHER_PATTERNS;
const float DITHER_PAT_PX = 16.0;

void main() {
    o_color = texture(u_texture, f_uv);
    if(o_color.a == 0.0) { discard; } // don't render the pixel if alpha = 0
    bool use_shadow_color = is_in_shadow(f_w_pos, f_norm);
    if(!use_shadow_color) {
        float diffuse_light = diffuse_intensity(f_norm);
        vec2 cell_pos = fract(gl_FragCoord.xy / DITHER_PAT_PX);
        float pat_i = floor(min(diffuse_light, 0.999) * DITHER_PATTERNS);
        vec2 pat_uv = vec2((pat_i + cell_pos.x) * DITHER_PAT_UVW, cell_pos.y);
        vec4 pat_color = texture(u_dither_pattern, pat_uv);
        use_shadow_color = pat_color.x < 0.5;
    }
    if(use_shadow_color) {
        vec3 shadow_color = map_palette(o_color.rgb, palette, shadow_palette);
        o_color = vec4(shadow_color, o_color.a);
    }
    o_color = blend_fog(o_color, f_w_pos);
}


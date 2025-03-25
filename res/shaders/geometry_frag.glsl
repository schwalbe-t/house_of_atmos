
#version 330

#include "common/palettes.glsl"
#include "common/shadows.glsl"
#include "common/diffuse.glsl"
#include "common/fog.glsl"

in vec2 f_uv;
in vec3 f_w_pos;
in vec3 f_norm;

uniform sampler2D u_texture;
uniform sampler2D u_dither_pattern;
uniform vec2 u_target_pixel_size;

out vec4 o_color;

const float DITHER_PATTERNS = 16;
const float DITHER_PAT_SIZE = 1.0 / DITHER_PATTERNS;
const float DITHER_PAT_PX = 16;

void main() {
    o_color = texture(u_texture, f_uv);
    if(o_color.w == 0) { discard; } // don't render the pixel if alpha = 0
    bool use_shadow_color = is_in_shadow(f_w_pos, f_norm);
    if(!use_shadow_color) {
        float diffuse_light = diffuse_intensity(f_norm);
        vec2 s_uv = gl_FragCoord.xy / u_target_pixel_size;
        float ar = u_target_pixel_size.x / u_target_pixel_size.y;
        float dit_freq = u_target_pixel_size.y / DITHER_PAT_PX;
        vec2 cell_pos = fract(vec2(s_uv.x * ar * dit_freq, s_uv.y * dit_freq));
        float pat_i = floor(min(diffuse_light, 0.999) * DITHER_PATTERNS);
        vec2 pat_uv = vec2((pat_i + cell_pos.x) * DITHER_PAT_SIZE, cell_pos.y);
        vec4 pat_color = texture(u_dither_pattern, pat_uv);
        use_shadow_color = pat_color.x < 0.5;
    }
    if(use_shadow_color) {
        vec3 shadow_color = map_palette(o_color.rgb, palette, shadow_palette);
        o_color = vec4(shadow_color, o_color.a);
    }
    o_color = blend_fog(o_color, f_w_pos);
}


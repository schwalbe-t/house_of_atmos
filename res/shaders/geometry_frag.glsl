
#version 330

#include "common/palettes.glsl"
#include "common/fog.glsl"
#include "common/shadows.glsl"

in vec2 f_uv;
in vec3 f_w_pos;

uniform sampler2D u_texture;

out vec4 o_color;

void main() {
    o_color = texture2D(u_texture, f_uv);
    if(o_color.w == 0) { discard; } // don't render the pixel if alpha = 0
    if(is_in_shadow(f_w_pos)) {
        o_color = vec4(
            map_palette(o_color.rgb, palette, shadow_palette), 
            o_color.a
        );
    }
    o_color = blend_fog(o_color, f_w_pos);
}


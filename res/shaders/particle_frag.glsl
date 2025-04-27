
#include "common/palettes.glsl"
#include "common/shadows.glsl"
#include "common/fog.glsl"

in vec2 f_uv;
in vec3 f_w_pos;

uniform vec3 u_camera_forward;
uniform sampler2D u_texture;
uniform vec2 u_uv_size;
uniform vec2 u_uv_offset;

out vec4 o_color;

void main() {
    o_color = texture(u_texture, f_uv * u_uv_size + u_uv_offset);
    if(o_color.a == 0.0) { discard; } // don't render the pixel if alpha = 0
    if(is_in_shadow(f_w_pos, -u_camera_forward)) {
        vec3 shadow_color = map_palette(o_color.rgb, palette, shadow_palette);
        o_color = vec4(shadow_color, o_color.a);
    }
    o_color = blend_fog(o_color, f_w_pos);
}
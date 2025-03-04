
#version 330

#include "common/packing.glsl"

in vec2 f_uv;
in vec3 f_w_pos;

uniform sampler2D u_texture;

out vec4 o_color;

void main() {
    vec4 tex = texture(u_texture, f_uv);
    if(tex.w == 0) { discard; } // don't render the pixel if alpha = 0
    gl_FragColor = vec4(pack_value(gl_FragCoord.z), 1.0);
}
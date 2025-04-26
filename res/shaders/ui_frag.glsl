
#version 300 es
precision highp float;

in vec2 f_uv;
flat in int f_instance_id;

uniform sampler2D u_texture;
uniform vec2 u_src_scales[128];
uniform vec2 u_src_offsets[128];

out vec4 o_color;

void main() {
    vec2 uv = f_uv * u_src_scales[f_instance_id] + u_src_offsets[f_instance_id];
    o_color = texture(u_texture, uv);
}
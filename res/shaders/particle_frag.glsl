
#version 330

in vec2 f_uv;

uniform sampler2D u_texture;
uniform vec2 u_uv_size;
uniform vec2 u_uv_offset;

out vec4 o_color;

void main() {
    o_color = texture(u_texture, f_uv * u_uv_size + u_uv_offset);
    if(o_color.a == 0) { discard; } // don't render the pixel if alpha = 0
}
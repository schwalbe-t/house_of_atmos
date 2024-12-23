
#version 330

in vec2 f_uv;

uniform sampler2D u_texture;

out vec4 o_color;

void main() {
    o_color = texture2D(u_texture, f_uv);
    if(o_color.w == 0) { discard; } // don't render the pixel if alpha = 0
}


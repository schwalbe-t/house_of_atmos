
#version 330

in vec2 f_uv;
in float f_light;

uniform sampler2D u_texture;

out vec4 o_color;

void main() {
    vec4 tex = texture2D(u_texture, f_uv);
    if(tex.w == 0) { discard; } // don't render the pixel if alpha = 0
    o_color = tex * f_light;
}


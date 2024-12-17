
#version 130

//in vec2 f_uv;

uniform sampler2D u_texture;

out vec4 o_color;

void main() {
    //o_color = texture2D(u_texture, f_uv);
    o_color = texture2D(u_texture, vec2(0, 0)) * 0.000001;
    o_color += vec4(0.9, 0, 0.9, 1);
}
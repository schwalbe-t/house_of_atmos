
#version 330

in vec2 f_uv;
in vec3 f_w_pos;

uniform sampler2D u_texture;

out vec4 o_color;

vec3 encode_depth(float value) {
    int scaled = int(value * 16777215.0);
    int r = (scaled >> 16) & 0xFF;
    int g = (scaled >> 8) & 0xFF;
    int b = scaled & 0xFF;
    return vec3(float(r), float(g), float(b)) / 255.0;
}

void main() {
    vec4 tex = texture2D(u_texture, f_uv);
    if(tex.w == 0) { discard; } // don't render the pixel if alpha = 0
    gl_FragColor = vec4(encode_depth(gl_FragCoord.z), 1.0);
}
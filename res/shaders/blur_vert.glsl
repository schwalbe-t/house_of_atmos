
#version 300 es
precision highp float;

layout(location = 0) in vec2 v_pos_uv;

out vec2 f_uv;

void main() {
    vec2 pos = v_pos_uv * 2.0 - vec2(1.0, 1.0);
    gl_Position = vec4(pos, 0.0, 1.0);
    f_uv = v_pos_uv;
}
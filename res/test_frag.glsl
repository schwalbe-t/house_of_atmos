
#version 130

in vec3 f_color;

uniform vec4 u_colors[3];

void main() {
    gl_FragColor =
        u_colors[0] * round(f_color.x * 2) / 2 + 
        u_colors[1] * round(f_color.y * 2) / 2 + 
        u_colors[2] * round(f_color.z * 2) / 2;
}
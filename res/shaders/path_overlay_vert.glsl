
layout(location = 0) in vec3 v_pos;

uniform mat4 u_view_proj;

out vec3 f_w_pos;

void main() {
    gl_Position = u_view_proj * vec4(v_pos, 1.0);
    f_w_pos = v_pos;
}
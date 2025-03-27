
#version 330

layout(location = 0) in vec2 v_pos_uv;

uniform mat4 u_view_proj;
uniform vec3 u_camera_right;
uniform vec3 u_camera_up;
uniform vec2 u_size[256];
uniform vec3 u_w_center_pos[256];

out vec2 f_uv;

void main() {
    vec2 rel_offset = v_pos_uv - vec2(0.5, 0.5);
    vec2 size = u_size[gl_InstanceID];
    vec3 w_pos = u_w_center_pos[gl_InstanceID]
        + u_camera_right * rel_offset.x * size.x
        +    u_camera_up * rel_offset.y * size.y;
    gl_Position = u_view_proj * vec4(w_pos, 1.0);
    f_uv = v_pos_uv;
}
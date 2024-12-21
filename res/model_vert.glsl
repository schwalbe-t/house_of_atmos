
#version 330

layout(location = 0) in vec3 v_pos;
layout(location = 1) in vec2 v_uv;
layout(location = 2) in vec3 v_norm;
layout(location = 3) in uvec4 v_joints;
layout(location = 4) in vec4 v_weights;

uniform mat4 u_projection;
uniform mat4 u_view;
uniform mat4 u_model;
uniform mat4 u_joint_transf[64];

out vec2 f_uv;

void main() {
    vec4 h_pos = vec4(v_pos, 1.0);
    vec4 s_pos = (u_joint_transf[v_joints.x] * h_pos) * v_weights.x
        + (u_joint_transf[v_joints.y] * h_pos) * v_weights.y
        + (u_joint_transf[v_joints.z] * h_pos) * v_weights.z
        + (u_joint_transf[v_joints.w] * h_pos) * v_weights.w;
    gl_Position = u_projection * u_view * u_model * s_pos;
    f_uv = v_uv;
}
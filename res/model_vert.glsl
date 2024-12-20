
#version 130

in vec3 v_pos;
in vec2 v_uv;
in vec3 v_norm;
// in uvec4 v_joints;
in vec4 v_weights;

uniform mat4 u_projection;
uniform mat4 u_view;
uniform mat4 u_model;
uniform mat4 u_joint_transf[64];

out vec2 f_uv;
out vec4 f_weights;

void main() {
    vec4 h_pos = vec4(v_pos, 1.0);
    // vec4 s_pos = (u_joint_transf[v_joints.x] * h_pos) * v_weights.x
    //     + (u_joint_transf[v_joints.y] * h_pos) * v_weights.y
    //     + (u_joint_transf[v_joints.z] * h_pos) * v_weights.z
    //     + (u_joint_transf[v_joints.w] * h_pos) * v_weights.w;
    vec4 s_pos = h_pos;
    gl_Position = u_projection * u_view * u_model * s_pos;
    f_uv = v_uv;
    f_weights = v_weights;
}

#version 330

layout(location = 0) in vec3 v_pos;
layout(location = 1) in vec2 v_uv;
layout(location = 2) in vec3 v_norm;
layout(location = 3) in uvec4 v_joints;
layout(location = 4) in vec4 v_weights;

uniform mat4 u_view_proj;
uniform mat4 u_model_transfs[128];
uniform mat4 u_local_transf;
uniform mat4 u_joint_transfs[32];

out vec2 f_uv;
out vec3 f_w_pos;

void main() {
    // apply skinning, transforms and projections to position
    vec4 h_pos = vec4(v_pos, 1.0);
    vec4 s_pos = (u_joint_transfs[v_joints.x] * h_pos) * v_weights.x  
        + (u_joint_transfs[v_joints.y] * h_pos) * v_weights.y
        + (u_joint_transfs[v_joints.z] * h_pos) * v_weights.z
        + (u_joint_transfs[v_joints.w] * h_pos) * v_weights.w;
    vec4 w_pos = u_model_transfs[gl_InstanceID] * u_local_transf * s_pos;
    gl_Position = u_view_proj * w_pos;
    f_uv = v_uv;
    f_w_pos = w_pos.xyz;
}

#version 330

layout(location = 0) in vec3 v_pos;
layout(location = 1) in vec2 v_uv;
layout(location = 2) in vec3 v_norm;
layout(location = 3) in uvec4 v_joints;
layout(location = 4) in vec4 v_weights;

uniform mat4 u_view_projection;
uniform mat4 u_model_transf;
uniform mat4 u_local_transf;
uniform mat4 u_joint_transf[32];

uniform vec3 u_light_pos;
uniform float u_ambient_light;

out vec2 f_uv;
out float f_light;

void main() {
    // apply skinning, transforms and projections to position
    vec4 h_pos = vec4(v_pos, 1.0);
    vec4 s_pos = (u_joint_transf[v_joints.x] * h_pos) * v_weights.x
        + (u_joint_transf[v_joints.y] * h_pos) * v_weights.y
        + (u_joint_transf[v_joints.z] * h_pos) * v_weights.z
        + (u_joint_transf[v_joints.w] * h_pos) * v_weights.w;
    vec4 w_pos = u_model_transf * u_local_transf * s_pos;
    vec4 f_pos = u_view_projection * w_pos;
    gl_Position = f_pos;
    // apply skinning and rotations to normal
    vec4 h_norm = vec4(v_norm, 1.0);
    vec4 s_norm = normalize(
        (inverse(transpose(u_joint_transf[v_joints.x])) * h_norm) * v_weights.x +
        (inverse(transpose(u_joint_transf[v_joints.y])) * h_norm) * v_weights.y +
        (inverse(transpose(u_joint_transf[v_joints.z])) * h_norm) * v_weights.z +
        (inverse(transpose(u_joint_transf[v_joints.w])) * h_norm) * v_weights.w
    );
    vec3 w_norm = normalize(
        inverse(transpose(u_local_transf)) * 
        inverse(transpose(u_model_transf)) * 
        s_norm
    ).xyz;
    // calculate diffuse lighting
    float diffuse = (dot(
        w_norm, normalize(u_light_pos - w_pos.xyz)
    ) + 1) / 2;
    f_light = diffuse * (1 - u_ambient_light) + u_ambient_light;
    // pass on UV mappings
    f_uv = v_uv;
}
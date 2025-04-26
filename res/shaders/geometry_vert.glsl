
#version 300 es
precision highp float;

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
out vec3 f_norm;

void main() {
    // apply skinning, transforms and projections to position
    vec4 h_pos = vec4(v_pos, 1.0);
    vec4 s_pos = (u_joint_transfs[v_joints.x] * h_pos) * v_weights.x  
        + (u_joint_transfs[v_joints.y] * h_pos) * v_weights.y
        + (u_joint_transfs[v_joints.z] * h_pos) * v_weights.z
        + (u_joint_transfs[v_joints.w] * h_pos) * v_weights.w;
    vec4 w_pos = u_model_transfs[gl_InstanceID] * u_local_transf * s_pos;
    gl_Position = u_view_proj * w_pos;
    // apply skinning to normals
    // IMPORTANT: THIS METHOD (extracting the top left of the matrix to remove
    //            translations) ONLY WORKS AS LONG AS THE MODEL AND JOINT
    //            TRANSFORMS ONLY CONSIST OF TRANSLATIONS, ROTATIONS AND
    //            **UNIFORM** SCALING!
    vec3 s_norm = (mat3(u_joint_transfs[v_joints.x]) * v_norm) * v_weights.x  
        + (mat3(u_joint_transfs[v_joints.y]) * v_norm) * v_weights.y
        + (mat3(u_joint_transfs[v_joints.z]) * v_norm) * v_weights.z
        + (mat3(u_joint_transfs[v_joints.w]) * v_norm) * v_weights.w;
    vec3 t_norm = mat3(u_model_transfs[gl_InstanceID]) 
        * mat3(u_local_transf) 
        * s_norm;
    // pass to fragment shader
    f_uv = v_uv;
    f_w_pos = w_pos.xyz;
    f_norm = normalize(t_norm);
}

layout(location = 0) in vec2 v_pos_uv;

uniform vec2 u_dest_scales[128];
uniform vec2 u_dest_offsets[128];

out vec2 f_uv;
flat out int f_instance_id;

void main() {
    vec2 pos = v_pos_uv * u_dest_scales[gl_InstanceID] 
        + u_dest_offsets[gl_InstanceID];
    gl_Position = vec4(pos, 0.0, 1.0);
    f_uv = v_pos_uv;
    f_instance_id = gl_InstanceID;
}

uniform vec3 u_sun_direction;
uniform float u_diffuse_min;
uniform float u_diffuse_max;

float diffuse_intensity(vec3 normal) {
    float diffuse_range = u_diffuse_max - u_diffuse_min;
    float diffuse_factor = max(0.0, dot(normal, -u_sun_direction));
    return min(1.0, u_diffuse_min + diffuse_range * diffuse_factor);
}
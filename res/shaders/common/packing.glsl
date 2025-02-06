
vec3 pack_value(float value) {
    int scaled = int(value * 16777215.0);
    int r = (scaled >> 16) & 0xFF;
    int g = (scaled >> 8) & 0xFF;
    int b = scaled & 0xFF;
    return vec3(float(r), float(g), float(b)) / 255.0;
}

float unpack_value(vec3 packed_value) {
    int r = int(round(packed_value.r * 255.0));
    int g = int(round(packed_value.g * 255.0));
    int b = int(round(packed_value.b * 255.0));
    int scaled = (r << 16) | (g << 8) | b;
    return float(scaled) / 16777215.0;
}
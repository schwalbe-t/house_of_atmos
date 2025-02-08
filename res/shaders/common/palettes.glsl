
#define NORM_COLOR(r, g, b) (vec3((r), (g), (b)) / 255.0)

const int PALETTE_SIZE = 19;
const vec3 palette[PALETTE_SIZE] = vec3[](
    NORM_COLOR(140, 143, 174),
    NORM_COLOR( 88,  69,  99),
    NORM_COLOR( 62,  33,  55),
    NORM_COLOR( 94,  64,  53),
    NORM_COLOR(154,  99,  72),
    NORM_COLOR(215, 155, 125),
    NORM_COLOR(255, 202, 168),
    NORM_COLOR(245, 237, 186),
    NORM_COLOR(192, 199,  65),
    NORM_COLOR(100, 125,  52),
    NORM_COLOR(228, 148,  58),
    NORM_COLOR(157,  48,  59),
    NORM_COLOR(210, 100, 113),
    NORM_COLOR(112,  55, 127),
    NORM_COLOR(126, 196, 193),
    NORM_COLOR( 52, 133, 157),
    NORM_COLOR( 23,  67,  75),
    NORM_COLOR( 31,  14,  28),
    NORM_COLOR(239, 239, 230)
);
const vec3 shadow_palette[PALETTE_SIZE] = vec3[](
    NORM_COLOR( 88,  69,  99),
    NORM_COLOR( 62,  33,  55),
    NORM_COLOR( 31,  14,  28),
    NORM_COLOR( 62,  33,  55),
    NORM_COLOR( 94,  64,  53),
    NORM_COLOR(154,  99,  72),
    NORM_COLOR(215, 155, 125),
    NORM_COLOR(255, 202, 168),
    NORM_COLOR(100, 125,  52),
    NORM_COLOR( 23,  67,  75),
    NORM_COLOR(154,  99,  72),
    NORM_COLOR( 62,  33,  55),
    NORM_COLOR(157,  48,  59),
    NORM_COLOR( 62,  33,  55),
    NORM_COLOR( 52, 133, 157),
    NORM_COLOR( 23,  67,  75),
    NORM_COLOR( 62,  33,  55),
    NORM_COLOR( 31,  14,  28),
    NORM_COLOR(140, 143, 174)
);
const vec3 no_mapping_color = vec3(1.0, 0.0, 1.0);

vec3 map_palette(
    vec3 color, vec3 in_palette[PALETTE_SIZE], vec3 out_palette[PALETTE_SIZE]
) {
    vec3 mapped_color = vec3(0.0, 0.0, 0.0);
    float found_match = 0.0;
    for(int i = 0; i < PALETTE_SIZE; i += 1) {
        float is_match = 1.0 - step(0.01, distance(color, in_palette[i]));
        mapped_color += is_match * out_palette[i];
        found_match += is_match;
    }
    found_match = step(0.0001, found_match);
    return mix(color, mapped_color, found_match);
}
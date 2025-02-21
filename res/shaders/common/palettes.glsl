
#include "util.glsl"

const int PALETTE_SIZE = 19;
const vec3 palette[PALETTE_SIZE] = vec3[](
    COLOR_24(140, 143, 174),
    COLOR_24( 88,  69,  99),
    COLOR_24( 62,  33,  55),
    COLOR_24( 94,  64,  53),
    COLOR_24(154,  99,  72),
    COLOR_24(215, 155, 125),
    COLOR_24(255, 202, 168),
    COLOR_24(245, 237, 186),
    COLOR_24(192, 199,  65),
    COLOR_24(100, 125,  52),
    COLOR_24(228, 148,  58),
    COLOR_24(157,  48,  59),
    COLOR_24(210, 100, 113),
    COLOR_24(112,  55, 127),
    COLOR_24(126, 196, 193),
    COLOR_24( 52, 133, 157),
    COLOR_24( 23,  67,  75),
    COLOR_24( 31,  14,  28),
    COLOR_24(239, 239, 230)
);
const vec3 shadow_palette[PALETTE_SIZE] = vec3[](
    COLOR_24( 88,  69,  99),
    COLOR_24( 62,  33,  55),
    COLOR_24( 31,  14,  28),
    COLOR_24( 62,  33,  55),
    COLOR_24( 94,  64,  53),
    COLOR_24(154,  99,  72),
    COLOR_24(215, 155, 125),
    COLOR_24(255, 202, 168),
    COLOR_24(100, 125,  52),
    COLOR_24( 23,  67,  75),
    COLOR_24(154,  99,  72),
    COLOR_24( 62,  33,  55),
    COLOR_24(157,  48,  59),
    COLOR_24( 62,  33,  55),
    COLOR_24( 52, 133, 157),
    COLOR_24( 23,  67,  75),
    COLOR_24( 62,  33,  55),
    COLOR_24( 31,  14,  28),
    COLOR_24(140, 143, 174)
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